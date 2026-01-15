#include "search.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <thread>
#include "evaluation.h"
#include "movegen.h"
#include "perft.h"
#include "position.h"
#include "types.h"
#include "uci.h"

int reductions[max_plies];
search_status status;

void search::init(){
  for(int i=0;i<max_plies;i++) reductions[i]=SC<int>(log_reduction_base/log_reduction_scale*std::log(i));
}

namespace{
  std::string uci_score_string(const int value){
    if(value>=mate_in_max_ply) return "mate "+std::to_string((mate-value+1)/deepen_thresh_scale);
    if(value<=mated_in_max_ply) return "mate "+std::to_string(-(mate+value-1)/deepen_thresh_scale);
    return "cp "+std::to_string(value);
  }

  int reduction(const bool i,const int depth,const int mn,const int delta){
    const int r=reductions[depth]*reductions[mn];
    return r-delta*delta_scale/status.root_delta+!i*r*improvement_scale_num/improvement_scale_den+base_reduction_offset-move_count_penalty*mn;
  }

  template<color SideToMove> int qsearch(int alpha,const int beta){
    status.nodes++;
    int eval=static_eval<SideToMove>();
    if(eval>=beta) return eval;
    alpha=std::max(alpha,eval);
    capture_list<SideToMove> captures;
    if(captures.size()==0) return eval;
    captures.sort();
    for(auto mv:captures){
      move m=extract_move(mv);
      piece captured=piece_on(to_sq(m));
      do_capture<SideToMove>(m);
      eval=-qsearch<!SideToMove>(-beta,-alpha);
      undo_capture<SideToMove>(m,captured);
      if(eval>=beta) return eval;
      alpha=std::max(alpha,eval);
    }
    return alpha;
  }

  template<node_type NodeT,color SideToMove> int search_ab(int alpha,const int beta,const int depth,const bool null_ok,search_info* si){
    constexpr bool rootnode=NodeT==root_node;
    constexpr bool pvnode=rootnode||NodeT==pv_node;
    status.nodes++;
    if(status.search_stop) return 0;
    if constexpr(!rootnode){
      if(rep_table::draw()) return 0;
    }
    if(depth<=0) return qsearch<SideToMove>(alpha,beta);
    if constexpr(!rootnode){
      if(const int lookup=hash_table::probe(depth,alpha,beta,si->ply);lookup!=no_eval) return lookup;
    }
    si->static_ev=static_eval<SideToMove>();
    if(null_ok&&position::midgame()&&depth>=null_move_min_depth&&!position::in_check<SideToMove>()){
      const int eval_margin=si->static_ev-beta;
      int r=null_move_base_reduction+(depth/null_move_depth_div);
      if(eval_margin>null_move_margin1) r+=null_move_bonus1;
      if(eval_margin>null_move_margin2) r+=null_move_bonus2;
      const int reduced_depth=std::max(1,depth-r);
      state_ptr->key^=zobrist::color;
      const int eval=-search_ab<nonpv_node,!SideToMove>(-beta,-beta+1,reduced_depth,false,si+1);
      state_ptr->key^=zobrist::color;
      if(status.search_stop) return 0;
      if(eval>=beta) return eval;
    }
    move_list<SideToMove> moves;
    if(moves.size()==0) return moves.in_check()?-mate+si->ply:0;
    int best_eval=-infinite;
    move best_move=no_move;
    move ttmove=hash_table::get_move();
    const bool improving=si->static_ev>(si-deepen_thresh_scale)->static_ev;
    const int extension=rootnode?0:moves.in_check();
    int move_count=0;
    bound_type bound_type=upper;
    moves.sort(ttmove,si);
    for(auto mv:moves){
      move m=extract_move(mv);
      move_count++;
      if(rootnode) status.root_delta=beta-alpha;
      int eval=-infinite;
      int new_depth=depth-1+extension;
      int r=reduction(improving,depth,move_count,beta-alpha);
      int lmr_depth=new_depth-r/1024;
      if constexpr(!rootnode){
        if(lmr_depth<futility_depth_limit&&get_movetype(m)==normal){
          if(const int futility_value=si->static_ev+piece_weight(piece_type_on(to_sq(m)))+futility_static_margin+futility_depth_scale*lmr_depth;futility_value<=alpha) continue;
        }
      }
      do_move<SideToMove>(m);
      if(depth>=deepen_thresh_scale&&move_count>1){
        int reduced=std::clamp(lmr_depth,1,new_depth);
        eval=-search_ab<nonpv_node,!SideToMove>(-(alpha+1),-alpha,reduced,true,si+1);
        if(eval>alpha&&reduced<new_depth){
          const bool do_deeper_search=eval>best_eval+deepen_thresh_base+deepen_thresh_scale*new_depth;
          const bool do_shallower_search=eval<best_eval+shallow_thresh;
          new_depth+=do_deeper_search-do_shallower_search;
          if(new_depth>reduced) eval=-search_ab<nonpv_node,!SideToMove>(-(alpha+1),-alpha,new_depth,!null_ok,si+1);
        } else if(eval>alpha&&eval<best_eval+shallow_thresh) --new_depth;
      } else{
        if constexpr(!pvnode){
          if(!ttmove) r+=no_tt_move_penalty;
          const int depth_reduction=(r>lmr_reduction_thresh_1)+
            (r>lmr_reduction_thresh_2&&new_depth>deepen_thresh_scale);
          eval=-search_ab<nonpv_node,!SideToMove>(
            -(alpha+1),-alpha,
            new_depth-depth_reduction,
            !null_ok,si+1);
        } else if(move_count>1){
          if(!ttmove) r+=no_tt_move_penalty;
          const int depth_reduction=(r>lmr_reduction_thresh_1)+
            (r>lmr_reduction_thresh_2&&new_depth>deepen_thresh_scale);
          eval=-search_ab<nonpv_node,!SideToMove>(
            -(alpha+1),-alpha,
            new_depth-depth_reduction,
            !null_ok,si+1);
        }
      }
      if constexpr(pvnode){
        if(move_count==1||eval>alpha){
          eval=-search_ab<pv_node,!SideToMove>(-beta,-alpha,new_depth,false,si+1);
        }
      }
      undo_move<SideToMove>(m);
      if(status.search_stop) return 0;
      if(eval>=beta){
        hash_table::store(SC<u8>(depth),lower,eval,m,si->ply);
        if(is_quiet(m)&&m!=si->killers[0]){
          si->killers[1]=si->killers[0];
          si->killers[0]=m;
        }
        return eval;
      }
      if(eval>alpha){
        best_move=m;
        alpha=eval;
        bound_type=exact;
        if(rootnode) status.best_move=best_move;
      }
      best_eval=std::max(eval,best_eval);
    }
    hash_table::store(SC<u8>(depth),bound_type,alpha,best_move,si->ply);
    return alpha;
  }

  template<color SideToMove> void iterative_deepening(const int max_depth){
    status.best_move=no_move;
    status.nodes=0;
    search_info search_stack[max_plies]={},* si=search_stack+futility_depth_limit;
    for(search_info* s=si+1;s!=search_stack+max_plies;s++) s->ply=(s-1)->ply+1;
    const bitboard start=get_ms();
    int guess=0,alpha=-infinite,beta=infinite;
    for(int depth=1;depth<=max_depth;depth++){
      constexpr int window=search_window;
fail: const int eval=search_ab<root_node,SideToMove>(alpha,beta,depth,false,si);
      if(status.search_stop) break;
      if(eval<=alpha){
        const int margin=guess-alpha;
        alpha=guess-margin*deepen_thresh_scale;
        goto fail;
      }
      if(eval>=beta){
        const int margin=beta-guess;
        beta=guess+margin*deepen_thresh_scale;
        goto fail;
      }
      guess=eval;
      alpha=eval-window;
      beta=eval+window;
      SO<<"info depth "<<depth
        <<" score "<<uci_score_string(eval)
        <<" nodes "<<status.nodes
        <<" nps "<<status.nodes*1000/std::max(1,SC<int>(get_ms()-start))
        <<" pv "<<pv()<<SE;
    }
  }
}

void search::go(bitboard thinktime,const int depth){
  status.search_stop=false;
  status.nodes=0;
  std::thread stop_thread;
  const bool use_timer=thinktime>0;
  if(use_timer){
    stop_thread=std::thread([thinktime]{
      std::this_thread::sleep_for(std::chrono::milliseconds(thinktime));
      status.search_stop=true;
    });
  }
  if(depth>0) position::white_to_move()?iterative_deepening<white>(depth):iterative_deepening<black>(depth);
  else position::white_to_move()?iterative_deepening<white>(max_depth):iterative_deepening<black>(max_depth);
  if(use_timer) stop_thread.join();
  SO<<"bestmove "<<move_to_uci(status.best_move)<<SE;
}

bitboard compute_think_time(const int time_left,const int increment,const int moves_to_go){
  constexpr int overhead=50;
  const int usable_time=std::max(0,time_left-overhead);
  const int base_time=usable_time/std::max(1,moves_to_go);
  const int bonus=increment*3/4;
  return std::max<bitboard>(10,base_time+bonus);
}
