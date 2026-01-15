#pragma once
#include <cassert>
#include <cstring>
#include <string>
#include "bitboard.h"
#include "hash.h"
#include "types.h"

constexpr int knight_weight=3;
constexpr int rook_weight=5;
constexpr int queen_weight=9;
constexpr int endgame_material_thresh=10;
constexpr int endgame_queenless_thresh=17;

enum game_phase : u8{
  mg,eg
};

typedef struct {
  bitboard key;
  color side_to_move;
  game_phase gamephase;
  piece captured;
  square ep_sq;
  u8 castling_rights;
} state_info;

extern bitboard bboards[16];
extern piece board[num_squares];
extern state_info state_stack[max_plies],* state_ptr;

namespace zobrist {
  constexpr bitboard color=0xeeb3b2fe864d41e5ull;
  extern bitboard hash[b_king + 1][num_squares];
}

template<piece P> bitboard bboard(){
  return bboards[P];
}

inline piece piece_on(const square sq){
  return board[sq];
}

inline piece_type piece_type_on(const square sq){
  return get_piecetype(board[sq]);
}

inline bool is_capture(const move m){
  return piece_on(to_sq(m));
}

inline bool is_quiet(const move m){
  return !piece_on(to_sq(m));
}

namespace position{
  void init();
  void set(const std::string& fen);
  void commit_move(move m);
  std::string fen();
  std::string to_string();

  inline bool white_to_move(){
    return state_ptr->side_to_move==white;
  }

  inline bool black_to_move(){
    return state_ptr->side_to_move==black;
  }

  inline bool midgame(){
    return state_ptr->gamephase==mg;
  }

  inline bool endgame(){
    return state_ptr->gamephase==eg;
  }

  inline bitboard occupied(){
    return bboards[white]|bboards[black];
  }

  inline bitboard ep_bb(){
    return square_bb(state_ptr->ep_sq);
  }

  inline bitboard key(){
    return state_ptr->key;
  }

  inline color side_to_move(){
    return state_ptr->side_to_move;
  }

  template<color SideToMove> bool in_check(){
    constexpr color them=!SideToMove;
    constexpr piece thempawn=get_piece(them,pawn);
    constexpr piece themknight=get_piece(them,knight);
    constexpr piece thembishop=get_piece(them,bishop);
    constexpr piece themrook=get_piece(them,rook);
    constexpr piece themqueen=get_piece(them,queen);
    const square ksq=lsb(bboard<get_piece(SideToMove,king)>());
    return
      pawn_attacks<SideToMove>((ksq)&bboard<thempawn>())
      |(knight_attacks(ksq)&bboard<themknight>())
      |(bishop_attacks(ksq,occupied()&(bboard<themqueen>()))|bboard<thembishop>())
      |(rook_attacks(ksq,occupied())&(bboard<themqueen>()|bboard<themrook>()));
  }
}

template<color JustMoved> void update_castling_rights(){
  constexpr bitboard mask=JustMoved==white?square_bb(a1,e1,h1,a8,h8):square_bb(a8,e8,h8,a1,h1);
  constexpr bitboard magic=JustMoved==white?0x4860104020003061ull:0x1080000400400c21ull;
  state_ptr->castling_rights&=get_castle_masks[JustMoved][(bboards[JustMoved]&mask)*magic>>59];
}

template<color Us> void do_capture(const move m){
  constexpr color them=!Us;
  constexpr piece p=get_piece(Us,pawn);
  const square fr=from_sq(m);
  const square to=to_sq(m);
  const bitboard to_bb=square_bb(to);
  const bitboard from_to=square_bb(fr,to);
  switch(get_movetype(m)){
  case normal:{
    bboards[board[to]]^=to_bb;
    bboards[them]^=to_bb;
    bboards[board[fr]]^=from_to;
    bboards[Us]^=from_to;
    board[to]=board[fr];
    board[fr]=no_piece;
    return;
  }
  case promote:{
    const piece promo=get_piece(Us,get_promotetype(m));
    bboards[board[to]]^=to_bb;
    bboards[them]^=to_bb;
    bboards[p]^=square_bb(fr);
    bboards[promo]^=to_bb;
    bboards[Us]^=from_to;
    board[to]=promo;
    board[fr]=no_piece;
    return;
  }
  default:
    assert(false && "Unhandled mv type in do_capture");
  }
}

template<color Us> void undo_capture(const move m,const piece captured){
  constexpr color them=!Us;
  constexpr piece p=get_piece(Us,pawn);
  const square fr=from_sq(m);
  const square to=to_sq(m);
  const bitboard to_bb=square_bb(to);
  const bitboard from_to=square_bb(fr,to);
  switch(get_movetype(m)){
  case normal:{
    bboards[board[to]]^=from_to;
    bboards[Us]^=from_to;
    bboards[captured]^=to_bb;
    bboards[them]^=to_bb;
    board[fr]=board[to];
    board[to]=captured;
    return;
  }
  case promote:{
    bboards[board[to]]^=to_bb;
    bboards[p]^=square_bb(fr);
    bboards[Us]^=from_to;
    bboards[captured]^=to_bb;
    bboards[them]^=to_bb;
    board[fr]=p;
    board[to]=captured;
    return;
  }
  default:
    assert(false && "Unhandled mv type in undo_capture");
  }
}

template<color Us> void do_move(const move m){
  constexpr color them=!Us;
  constexpr piece p=get_piece(Us,pawn);
  constexpr piece r=get_piece(Us,rook);
  constexpr piece k=get_piece(Us,king);
  constexpr dir up=Us==white?north:south;
  const square fr=from_sq(m);
  const square to=to_sq(m);
  memcpy(state_ptr+1,state_ptr,sizeof(state_info));
  state_ptr++;
  state_ptr->captured=piece_on(to);
  state_ptr->ep_sq=(fr+up)*!((fr^to^16)|(piece_on(fr)^p));
  const bitboard zero_to=~square_bb(to);
  const bitboard from_to=square_bb(fr,to);
  switch(get_movetype(m)){
  case normal:{
    state_ptr->key^=zobrist::hash[board[fr]][fr]
      ^zobrist::hash[board[fr]][to]
      ^zobrist::hash[board[to]][to]
      ^zobrist::color;
    bboards[board[to]]&=zero_to;
    bboards[them]&=zero_to;
    bboards[board[fr]]^=from_to;
    bboards[Us]^=from_to;
    board[to]=board[fr];
    board[fr]=no_piece;
    update_castling_rights<Us>();
    rep_table::push();
    return;
  }
  case promote:{
    const piece promo=get_piece(Us,get_promotetype(m));
    state_ptr->key^=zobrist::hash[p][fr]
      ^zobrist::hash[promo][to]
      ^zobrist::hash[board[to]][to]
      ^zobrist::color;
    bboards[board[to]]&=zero_to;
    bboards[them]&=zero_to;
    bboards[p]^=square_bb(fr);
    bboards[promo]^=~zero_to;
    bboards[Us]^=from_to;
    board[to]=promo;
    board[fr]=no_piece;
    update_castling_rights<Us>();
    rep_table::push();
    return;
  }
  case castle:{
    const move rook_move=Us==white?(to==g1?make_move(h1,f1):make_move(a1,d1))
      :to==g8?make_move(h8,f8):make_move(a8,d8);
    const square rfrom=from_sq(rook_move);
    const square rto=to_sq(rook_move);
    const bitboard rfrom_to=square_bb(rfrom,rto);
    state_ptr->key^=zobrist::hash[k][fr]
      ^zobrist::hash[k][to]
      ^zobrist::hash[r][rfrom]
      ^zobrist::hash[r][rto]
      ^zobrist::color;
    bboards[k]^=from_to;
    bboards[r]^=rfrom_to;
    bboards[Us]^=from_to^rfrom_to;
    board[fr]=no_piece;
    board[rfrom]=no_piece;
    board[to]=k;
    board[rto]=r;
    constexpr u8 mask=Us==white?0b0011:0b1100;
    state_ptr->castling_rights&=mask;
    rep_table::push();
    return;
  }
  case en_passant:{
    constexpr piece thempawn=get_piece(them,pawn);
    const square capsq=to+(Us==white?south:north);
    state_ptr->key^=zobrist::hash[p][fr]
      ^zobrist::hash[p][to]
      ^zobrist::hash[thempawn][capsq]
      ^zobrist::color;
    bboards[p]^=from_to;
    bboards[thempawn]^=square_bb(capsq);
    bboards[Us]^=from_to;
    bboards[them]^=square_bb(capsq);
    board[fr]=no_piece;
    board[to]=p;
    board[capsq]=no_piece;
    rep_table::push();
    return;
  }
  default:{
    assert(false && "Unhandled mv type in do_move");
  }
  }
}

template<color Us> void undo_move(const move m){
  rep_table::pop();
  constexpr color them=!Us;
  constexpr piece p=get_piece(Us,pawn);
  constexpr piece r=get_piece(Us,rook);
  constexpr piece k=get_piece(Us,king);
  const piece captured=state_ptr--->captured;
  const square from=from_sq(m);
  const square to=to_sq(m);
  const bitboard to_bb=square_bb(to);
  const bitboard fromto_bb=square_bb(from,to);
  const bitboard capture_bb=to_bb*SC<bool>(captured);
  switch(get_movetype(m)){
  case normal:{
    bboards[board[to]]^=fromto_bb;
    bboards[Us]^=fromto_bb;
    bboards[captured]^=capture_bb;
    bboards[them]^=capture_bb;
    board[from]=board[to];
    board[to]=captured;
    return;
  }
  case promote:{
    bboards[board[to]]^=to_bb;
    bboards[p]^=square_bb(from);
    bboards[Us]^=fromto_bb;
    bboards[captured]^=capture_bb;
    bboards[them]^=capture_bb;
    board[to]=captured;
    board[from]=p;
    return;
  }
  case castle:{
    const move rook_move=Us==white?(to==g1?make_move(h1,f1):make_move(a1,d1))
      :to==g8?make_move(h8,f8):make_move(a8,d8);
    const square rfrom=from_sq(rook_move);
    const square rto=to_sq(rook_move);
    const bitboard rfrom_to=square_bb(rfrom,rto);
    bboards[k]^=fromto_bb;
    bboards[r]^=rfrom_to;
    bboards[Us]^=fromto_bb^rfrom_to;
    board[to]=no_piece;
    board[rto]=no_piece;
    board[from]=k;
    board[rfrom]=r;
    return;
  }
  case en_passant:{
    constexpr piece thempawn=get_piece(them,pawn);
    const square capsq=to+(Us==white?south:north);
    bboards[p]^=fromto_bb;
    bboards[Us]^=fromto_bb;
    bboards[thempawn]^=square_bb(capsq);
    bboards[them]^=square_bb(capsq);
    board[to]=no_piece;
    board[from]=p;
    board[capsq]=thempawn;
    return;
  }
  default:{
    assert(false && "Unhandled mv type in undo_move");
  }
  }
}

void set_gamephase();
