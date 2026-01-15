#pragma once
#include "bitboard.h"
#include "evaluation.h"
#include "position.h"
#include "search.h"
#include "types.h"

extern const move* table[num_colors][1<<4][1<<6];
constexpr uint16_t max_short=65535;
constexpr int max_moves=128;
constexpr int max_captures=32;

constexpr score bad_quiet_min=max_short/4*0;
constexpr score bad_capture_min=max_short/4*1;
constexpr score good_quiet_min=max_short/4*2;
constexpr score good_capture_min=max_short/4*3;
constexpr score bad_quiet_base=bad_quiet_min+max_short/8;
constexpr score bad_capture_base=bad_capture_min+max_short/8;
constexpr score good_quiet_base=good_quiet_min+max_short/8;
constexpr score good_capture_base=good_capture_min+max_short/8;
constexpr score max_score=max_short;
constexpr score seen_by_pawn_malus=50;

inline move extract_move(const movelist lm){
  return static_cast<move>(lm&0xFFFF);
}

namespace move_gen{
  void init();
}

inline constexpr move data[num_colors][5]=
{
{make_move<castle>(e1,g1),make_move<castle>(e1,c1),nullmove,make_move<castle>(e1,g1),nullmove},
{make_move<castle>(e8,g8),make_move<castle>(e8,c8),nullmove,make_move<castle>(e8,g8),nullmove}
};

inline void move_gen::init(){
  for(const color c:{white,black})
    for(int rights=0;rights<=0xf;rights++)
      for(bitboard hash=0;hash<=0b111111;hash++){
        const move* kcastle=&data[c][3];
        const move* qcastle=&data[c][1];
        const move* both=&data[c][0];
        const move* no_castle=&data[c][2];
        const move* src=no_castle;
        const bool rights_k=rights&(c==white?8:2);
        const bool rights_q=rights&(c==white?4:1);
        const bool rights_kq=rights_k&&rights_q;
        if(rights_k||rights_q){
          if(hash==0) src=rights_kq?both:rights_k?kcastle:qcastle;
          else if((hash&0b000111)==0) src=rights_k?kcastle:no_castle;
          else if((hash&0b111100)==0) src=rights_q?qcastle:no_castle;
        }
        table[c][rights][hash]=src;
      }
}

template<move_type Type,dir D> movelist* make_pawn_moves(movelist* list,bitboard attacks){
  for(;attacks;clear_lsb(attacks)){
    const square to=lsb(attacks);
    if constexpr(Type==normal){
      *list++=make_move(to-D,to);
    } else if constexpr(Type==promote){
      *list++=make_move<knight_promote>(to-D,to);
      *list++=make_move<bishop_promote>(to-D,to);
      *list++=make_move<rook_promote>(to-D,to);
      *list++=make_move<queen_promote>(to-D,to);
    }
  }
  return list;
}

inline movelist* make_moves(movelist* list,const square from,bitboard to){
  for(;to;clear_lsb(to)) *list++=make_move(from,lsb(to));
  return list;
}

template<color Us> class move_list{
public:
  move_list();

  movelist* begin(){
    return moves;
  }

  [[nodiscard]] movelist* end() const{
    return last;
  }

  [[nodiscard]] size_t size() const{
    return last-moves;
  }

  [[nodiscard]] bool in_check() const{
    return ~checkmask_;
  }

  void sort(move ttmove,const search_info* si);
  movelist moves[max_moves],* last=moves;
private:
  bitboard checkmask_;
  bitboard seen_by_them_;
  void quicksort(int low,int high);
  int partition(int low,int high);
};

template<color Us> class capture_list{
public:
  capture_list();

  movelist* begin(){
    return moves_;
  }

  [[nodiscard]] movelist* end() const{
    return last_;
  }

  [[nodiscard]] size_t size() const{
    return last_-moves_;
  }

  void sort();
private:
  void insertion_sort();
  movelist moves_[max_captures],* last_=moves_;
  bitboard seen_by_them_;
};

template<color Us> move_list<Us>::move_list(): moves{}{
  constexpr color them=!Us;
  constexpr piece our_pawn=get_piece(Us,pawn);
  constexpr piece our_knight=get_piece(Us,knight);
  constexpr piece our_bishop=get_piece(Us,bishop);
  constexpr piece our_rook=get_piece(Us,rook);
  constexpr piece our_queen=get_piece(Us,queen);
  constexpr piece our_king=get_piece(Us,king);
  constexpr piece their_pawn=get_piece(them,pawn);
  constexpr piece their_knight=get_piece(them,knight);
  constexpr piece their_bishop=get_piece(them,bishop);
  constexpr piece their_rook=get_piece(them,rook);
  constexpr piece their_queen=get_piece(them,queen);
  constexpr piece their_king=get_piece(them,king);
  const bitboard their_rook_queen=bboard<their_queen>()|bboard<their_rook>();
  const bitboard their_bishop_queen=bboard<their_queen>()|bboard<their_bishop>();
  const square ksq=lsb(bboard<our_king>());
  bitboard occupied=position::occupied()^square_bb(ksq);
  seen_by_them_=pawn_attacks<them>(bboard<their_pawn>())|king_attacks(lsb(bboard<their_king>()));
  for(bitboard b=bboard<their_knight>();b;clear_lsb(b)) seen_by_them_|=knight_attacks(lsb(b));
  for(bitboard b=their_bishop_queen;b;clear_lsb(b)) seen_by_them_|=bishop_attacks(lsb(b),occupied);
  for(bitboard b=their_rook_queen;b;clear_lsb(b)) seen_by_them_|=rook_attacks(lsb(b),occupied);
  toggle_square(occupied,ksq);
  checkmask_=(knight_attacks(ksq)&bboard<their_knight>())|(pawn_attacks<Us>(ksq)&bboard<their_pawn>());
  for(bitboard checkers=(bishop_attacks(ksq,occupied)&their_bishop_queen)|(rook_attacks(ksq,occupied)&their_rook_queen);
      checkers;clear_lsb(checkers))
    checkmask_|=check_ray(ksq,lsb(checkers));
  if(more_than_one(checkmask_&double_check(ksq))){
    last=make_moves(last,ksq,king_attacks(ksq)&~(seen_by_them_|bboard<Us>()));
    return;
  }
  checkmask_|=-!checkmask_;
  bitboard pinned=0;
  for(bitboard pinners=(bishop_xray(ksq,occupied)&their_bishop_queen)|(rook_xray(ksq,occupied)&their_rook_queen);
      pinners;clear_lsb(pinners))
    pinned|=check_ray(ksq,lsb(pinners));
  constexpr dir up=Us==white?north:south;
  constexpr dir up_right=Us==white?northeast:southwest;
  constexpr dir up_left=Us==white?northwest:southeast;
  constexpr bitboard rank3=Us==white?rank_3_bb:rank_6_bb;
  constexpr bitboard rank6=Us==white?rank_6_bb:rank_3_bb;
  constexpr bitboard rank7=Us==white?rank_7_bb:rank_2_bb;
  bitboard empty=~occupied;
  bitboard e=shift<up>(rank3&empty)&empty;
  const bitboard pawns=bboard<our_pawn>()&~rank7;
  last=make_pawn_moves<normal,up_right>(last,shift<up_right>(pawns&(~pinned|anti_diag(ksq)))&bboard<them>()&checkmask_);
  last=make_pawn_moves<normal,up_left>(last,shift<up_left>(pawns&(~pinned|main_diag(ksq)))&bboard<them>()&checkmask_);
  last=make_pawn_moves<normal,up>(last,shift<up>(pawns&(~pinned|file_bb(ksq)))&empty&checkmask_);
  last=make_pawn_moves<normal,up*2>(last,shift<up*2>(pawns&(~pinned|file_bb(ksq)))&e&checkmask_);
  if(const bitboard promotable=bboard<our_pawn>()&rank7){
    last=make_pawn_moves<promote,up_right>(last,shift<up_right>(promotable&(~pinned|anti_diag(ksq)))&bboard<them>()&checkmask_);
    last=make_pawn_moves<promote,up_left>(last,shift<up_left>(promotable&(~pinned|main_diag(ksq)))&bboard<them>()&checkmask_);
    last=make_pawn_moves<promote,up>(last,shift<up>(promotable&~pinned)&empty&checkmask_);
  }
  if(bitboard b=shift<up_right>(bboard<our_pawn>())&position::ep_bb()&rank6){
    *last=make_move<en_passant>(state_ptr->ep_sq-up_right,state_ptr->ep_sq);
    const bitboard after_ep=occupied^(b|shift_unsafe<-up_right>(b)|shift_unsafe<-up>(b));
    last+=!((bishop_attacks(ksq,after_ep)&their_bishop_queen)|(rook_attacks(ksq,after_ep)&their_rook_queen));
  }
  if(bitboard b=shift<up_left>(bboard<our_pawn>())&position::ep_bb()&rank6){
    *last=make_move<en_passant>(state_ptr->ep_sq-up_left,state_ptr->ep_sq);
    const bitboard after_ep=occupied^(b|shift_unsafe<-up_left>(b)|shift_unsafe<-up>(b));
    last+=!((bishop_attacks(ksq,after_ep)&their_bishop_queen)|(rook_attacks(ksq,after_ep)&their_rook_queen));
  }
  const bitboard legal=checkmask_&~bboard<Us>();
  for(bitboard b=bboard<our_knight>()&~pinned;b;clear_lsb(b)){
    const square from=lsb(b);
    last=make_moves(last,from,knight_attacks(from)&legal);
  }
  const bitboard bishop_queen=bboard<our_bishop>()|bboard<our_queen>();
  const bitboard rook_queen=bboard<our_rook>()|bboard<our_queen>();
  for(bitboard b=bishop_queen&~pinned;b;clear_lsb(b)){
    const square from=lsb(b);
    last=make_moves(last,from,bishop_attacks(from,occupied)&legal);
  }
  for(bitboard b=bishop_queen&pinned;b;clear_lsb(b)){
    const square from=lsb(b);
    last=make_moves(last,from,bishop_attacks(from,occupied)&legal&align_mask(ksq,from));
  }
  for(bitboard b=rook_queen&~pinned;b;clear_lsb(b)){
    const square from=lsb(b);
    last=make_moves(last,from,rook_attacks(from,occupied)&legal);
  }
  for(bitboard b=rook_queen&pinned;b;clear_lsb(b)){
    const square from=lsb(b);
    last=make_moves(last,from,rook_attacks(from,occupied)&legal&align_mask(ksq,from));
  }
  last=make_moves(last,ksq,king_attacks(ksq)&~(seen_by_them_|bboard<Us>()));
  constexpr int shift=Us==white?1:57;
  constexpr bitboard no_atk=Us==white?square_bb(c1,d1,e1,f1,g1):square_bb(c8,d8,e8,f8,g8);
  constexpr bitboard no_occ=Us==white?square_bb(b1,c1,d1,f1,g1):square_bb(b8,c8,d8,f8,g8);
  for(const move* src=table[Us][state_ptr->castling_rights][((no_atk&seen_by_them_)|(no_occ&occupied))>>shift];*src;*last++=*src++);
}

template<color Us> capture_list<Us>::capture_list(): moves_{}{
  constexpr color them=!Us;
  constexpr piece our_pawn=get_piece(Us,pawn);
  constexpr piece our_knight=get_piece(Us,knight);
  constexpr piece our_bishop=get_piece(Us,bishop);
  constexpr piece our_rook=get_piece(Us,rook);
  constexpr piece our_queen=get_piece(Us,queen);
  constexpr piece our_king=get_piece(Us,king);
  constexpr piece their_pawn=get_piece(them,pawn);
  constexpr piece their_knight=get_piece(them,knight);
  constexpr piece their_bishop=get_piece(them,bishop);
  constexpr piece their_rook=get_piece(them,rook);
  constexpr piece their_queen=get_piece(them,queen);
  constexpr piece their_king=get_piece(them,king);
  const bitboard their_rook_queen=bboard<their_queen>()|bboard<their_rook>();
  const bitboard their_bishop_queen=bboard<their_queen>()|bboard<their_bishop>();
  const square ksq=lsb(bboard<our_king>());
  bitboard occupied=position::occupied()^square_bb(ksq);
  seen_by_them_=pawn_attacks<them>(bboard<their_pawn>())|king_attacks(lsb(bboard<their_king>()));
  for(bitboard b=bboard<their_knight>();b;clear_lsb(b)) seen_by_them_|=knight_attacks(lsb(b));
  for(bitboard b=their_bishop_queen;b;clear_lsb(b)) seen_by_them_|=bishop_attacks(lsb(b),occupied);
  for(bitboard b=their_rook_queen;b;clear_lsb(b)) seen_by_them_|=rook_attacks(lsb(b),occupied);
  toggle_square(occupied,ksq);
  bitboard checkmask=(knight_attacks(ksq)&bboard<their_knight>())|(pawn_attacks<Us>(ksq)&bboard<their_pawn>());
  for(bitboard checkers=(bishop_attacks(ksq,occupied)&their_bishop_queen)|(rook_attacks(ksq,occupied)&their_rook_queen);
      checkers;clear_lsb(checkers))
    checkmask|=check_ray(ksq,lsb(checkers));
  if(more_than_one(checkmask&double_check(ksq))){
    last_=make_moves(last_,ksq,king_attacks(ksq)&bboard<them>()&~seen_by_them_);
    return;
  }
  checkmask=(checkmask|-!checkmask)&bboard<them>();
  bitboard pinned=0;
  for(bitboard pinners=(bishop_xray(ksq,occupied)&their_bishop_queen)|(rook_xray(ksq,occupied)&their_rook_queen);
      pinners;clear_lsb(pinners))
    pinned|=check_ray(ksq,lsb(pinners));
  constexpr dir up_right=Us==white?northeast:southwest;
  constexpr dir up_left=Us==white?northwest:southeast;
  constexpr bitboard rank7=Us==white?rank_7_bb:rank_2_bb;
  if(const bitboard promotable=bboard<our_pawn>()&rank7){
    last_=make_pawn_moves<promote,up_right>(last_,shift<up_right>(promotable&(~pinned|anti_diag(ksq)))&checkmask);
    last_=make_pawn_moves<promote,up_left>(last_,shift<up_left>(promotable&(~pinned|main_diag(ksq)))&checkmask);
  }
  const bitboard pawns=bboard<our_pawn>()&~rank7;
  last_=make_pawn_moves<normal,up_right>(last_,shift<up_right>(pawns&(~pinned|anti_diag(ksq)))&checkmask);
  last_=make_pawn_moves<normal,up_left>(last_,shift<up_left>(pawns&(~pinned|main_diag(ksq)))&checkmask);
  for(bitboard b=bboard<our_knight>()&~pinned;b;clear_lsb(b)){
    const square from=lsb(b);
    last_=make_moves(last_,from,knight_attacks(from)&checkmask);
  }
  const bitboard bishop_queen=bboard<our_bishop>()|bboard<our_queen>();
  const bitboard rook_queen=bboard<our_rook>()|bboard<our_queen>();
  for(bitboard b=bishop_queen&~pinned;b;clear_lsb(b)){
    const square from=lsb(b);
    last_=make_moves(last_,from,bishop_attacks(from,occupied)&checkmask);
  }
  for(bitboard b=bishop_queen&pinned;b;clear_lsb(b)){
    const square from=lsb(b);
    last_=make_moves(last_,from,bishop_attacks(from,occupied)&checkmask&align_mask(ksq,from));
  }
  for(bitboard b=rook_queen&~pinned;b;clear_lsb(b)){
    const square from=lsb(b);
    last_=make_moves(last_,from,rook_attacks(from,occupied)&checkmask);
  }
  for(bitboard b=rook_queen&pinned;b;clear_lsb(b)){
    const square from=lsb(b);
    last_=make_moves(last_,from,rook_attacks(from,occupied)&checkmask&align_mask(ksq,from));
  }
  last_=make_moves(last_,ksq,king_attacks(ksq)&bboard<them>()&~seen_by_them_);
}

template<color Us> void capture_list<Us>::insertion_sort(){
  for(size_t i=1;i<size();i++){
    const movelist key=moves_[i];
    int j=static_cast<int>(i)-1;
    while(j>=0&&get_score(moves_[j])<get_score(key)){
      moves_[j+1]=moves_[j];
      j--;
    }
    moves_[j+1]=key;
  }
}

template<color Us> void capture_list<Us>::sort(){
  const bitboard seen_by_pawn=pawn_attacks<!Us>(bboard<get_piece(!Us,pawn)>());
  for(movelist& m:*this){
    score value=max_short/2;
    const square from=from_sq(SC<move>(m));
    const square to=to_sq(SC<move>(m));
    const piece_type pt=piece_type_on(from);
    const piece_type captured=piece_type_on(to);
    if(square_bb(to)&seen_by_pawn) value-=500;
    value+=piece_weight(captured)-piece_weight(pt)*SC<bool>(square_bb(to)&seen_by_them_);
    set_score(m,value);
  }
  insertion_sort();
}

template<color Us> int move_list<Us>::partition(const int low,const int high){
  const score pivot=get_score(moves[high]);
  int i=low-1;
  for(int j=low;j<high;j++){
    if(get_score(moves[j])>=pivot){
      i++;
      std::swap(moves[i],moves[j]);
    }
  }
  std::swap(moves[i+1],moves[high]);
  return i+1;
}

template<color Us> void move_list<Us>::quicksort(const int low,const int high){
  if(low<high){
    const int pivot_index=partition(low,high);
    quicksort(low,pivot_index-1);
    quicksort(pivot_index+1,high);
  }
}

template<color Us> void move_list<Us>::sort(const move ttmove,const search_info* si){
  const bitboard seen_by_pawn=pawn_attacks<!Us>(bboard<get_piece(!Us,pawn)>());
  for(movelist& m:*this){
    if(m==ttmove){
      set_score(m,max_score);
      continue;
    }
    score value;
    square from=from_sq(SC<move>(m));
    square to=to_sq(SC<move>(m));
    piece_type pt=piece_type_on(from);
    if(const piece_type captured=piece_type_on(to)){
      const int material_delta=piece_weight(captured)-piece_weight(pt);
      if(square_bb(to)&seen_by_them_) value=(material_delta>=0?good_capture_base:bad_capture_base)+material_delta;
      else value=good_capture_base+material_delta;
    } else{
      if(m==si->killers[0]
        ||m==si->killers[1])
        value=good_quiet_base;
      else value=bad_quiet_base;
      value+=position::endgame()&&pt==king
        ?(end_king_squares[to]-end_king_squares[from])/2
        :(square_score<Us>(pt,to)-square_score<Us>(pt,from))/2;
    }
    if(get_movetype(SC<move>(m))==promote) value+=get_promotetype(SC<move>(m));
    if(square_bb(to)&seen_by_pawn) value-=seen_by_pawn_malus;
    set_score(m,value);
  }
  quicksort(0,static_cast<int>(size())-1);
}
