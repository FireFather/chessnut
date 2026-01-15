#include "position.h"
#include <iomanip>
#include <random>
#include <sstream>
#include "hash.h"
#include "uci.h"

bitboard bboards[16];
piece board[num_squares];
state_info state_stack[max_plies];
state_info* state_ptr=state_stack;
const std::string piece_to_char="  PNBRQK  pnbrqk";

namespace{
  [[maybe_unused]] bitboard hash[b_king+1][num_squares];
}

namespace zobrist {
  bitboard hash[b_king + 1][num_squares];
}

void position::init(){
  for(const piece pc:{w_pawn,w_knight,w_bishop,w_rook,w_queen,w_king,
      b_pawn,b_knight,b_bishop,b_rook,b_queen,b_king
      }){
    for(square sq=h1;sq<=a8;sq++) zobrist::hash[pc][sq]=rand64();
  }
}

void position::set(const std::string& fen){
  memset(board,no_piece,64*sizeof(piece));
  memset(bboards,0,16*sizeof(bitboard));
  square s=a8;
  std::istringstream is(fen);
  std::string pieces,color,castling,enpassant;
  is>>pieces>>color>>castling>>enpassant;
  for(const char token:pieces){
    if(std::isdigit(token)){
      s-=token-'0';
    } else if(const size_t pc=piece_to_char.find(token);pc!=std::string::npos){
      board[s]=SC<piece>(pc);
      bboards[pc]|=square_bb(s);
      bboards[get_color(SC<piece>(pc))]|=square_bb(s);
      s--;
    }
  }
  state_ptr->side_to_move=color=="w"?white:black;
  state_ptr->castling_rights=static_cast<u8>(state_ptr->ep_sq=0);
  for(const char token:castling) if(const size_t idx=std::string("qkQK").find(token);idx!=std::string::npos) state_ptr->castling_rights|=1<<idx;
  if(enpassant!="-") state_ptr->ep_sq=uci_to_square(enpassant);
  state_ptr->key=state_ptr->side_to_move==white?0:zobrist::color;
  for(square sq=h1;sq<=a8;sq++) state_ptr->key^=zobrist::hash[piece_on(sq)][sq];
  rep_table::clear();
  rep_table::push();
  set_gamephase();
}

std::string position::to_string(){
  std::stringstream ss;
  ss<<"\n  +---+---+---+---+---+---+---+---+\n";
  for(int r=8;r>=1;--r){
    ss<<r<<" ";
    for(int f=7;f>=0;--f){
      const square sq=(r-1)*8+f;
      ss<<"| "<<piece_to_char[board[sq]]<<" ";
    }
    ss<<"|\n";
    ss<<"  +---+---+---+---+---+---+---+---+\n";
  }
  ss<<"    a   b   c   d   e   f   g   h\n\n";
  ss<<"Fen: "<<fen()<<"\n"
    <<"Key: "<<std::setw(16)<<std::setfill('0')
    <<std::hex<<std::uppercase<<key()<<"\n";
  return ss.str();
}

std::string position::fen(){
  std::stringstream fen;
  for(rank r=rank_8;r>=rank_1;r--){
    for(file f=file_a;f>=file_h;f--){
      if(const piece pc=piece_on(r*8+f)){
        fen<<piece_to_char[pc];
      } else{
        int empty=0;
        for(file fi=f+1;fi>=file_h&&!piece_on(r*8+fi);fi--){
          empty++;
        }
        fen<<empty;
      }
    }
    if(r) fen<<"/";
  }
  fen<<" "<<"wb"[side_to_move()]<<" ";
  if(!state_ptr->castling_rights) fen<<"-";
  else{
    if(state_ptr->castling_rights&8) fen<<"K";
    if(state_ptr->castling_rights&4) fen<<"Q";
    if(state_ptr->castling_rights&2) fen<<"k";
    if(state_ptr->castling_rights&1) fen<<"q";
  }
  fen<<" "<<(state_ptr->ep_sq?square_to_uci(state_ptr->ep_sq):"-");
  return fen.str();
}

void position::commit_move(const move m){
  if(is_capture(m)||piece_type_on(from_sq(m))==pawn||get_movetype(m)!=normal) rep_table::clear();
  white_to_move()?do_move<white>(m)
    :do_move<black>(m);
  memcpy(state_stack,state_ptr,sizeof(state_info));
  state_ptr=state_stack;
  state_ptr->side_to_move=!state_ptr->side_to_move;
  set_gamephase();
}

void set_gamephase(){
  const color us=state_ptr->side_to_move;
  const int our_material=
    knight_weight*popcnt(bboards[get_piece(us,knight)])+
    knight_weight*popcnt(bboards[get_piece(us,bishop)])+
    rook_weight*popcnt(bboards[get_piece(us,rook)])+
    queen_weight*popcnt(bboards[get_piece(us,queen)]);
  if(our_material<endgame_material_thresh||
    (our_material<endgame_queenless_thresh&&!bboards[get_piece(us,queen)])){
    state_ptr->gamephase=eg;
  } else{
    state_ptr->gamephase=mg;
  }
}
