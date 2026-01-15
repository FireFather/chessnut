#pragma once
#include <algorithm>
#include "position.h"
#include "types.h"

constexpr int pawn_value=100;
constexpr int knight_value=320;
constexpr int bishop_value=330;
constexpr int rook_value=500;
constexpr int queen_value=980;
constexpr int king_value=1500;

constexpr int advanced_pawn_bonus=4;
constexpr int rank4_pawn_bonus=10;
constexpr int rank5_pawn_bonus=20;
constexpr int rank6_pawn_bonus=50;
constexpr int rank7_pawn_bonus=90;

constexpr int center_dist_max=14;

constexpr int psts[num_piecetypes][num_squares]={
{
50,50,50,50,50,50,50,50,
50,50,50,50,50,50,50,50,
10,10,20,30,30,20,10,10,
5,5,10,25,25,10,5,5,
0,0,0,20,20,0,0,0,
5,-5,-10,0,0,-10,-5,5,
5,10,10,-20,-20,10,10,5,
0,0,0,0,0,0,0,0
},
{
-50,-40,-30,-30,-30,-30,-40,-50,
-40,-20,0,0,0,0,-20,-40,
-30,0,10,15,15,10,0,-30,
-30,5,15,20,20,15,5,-30,
-30,0,15,20,20,15,0,-30,
-30,5,10,15,15,10,5,-30,
-40,-20,0,5,5,0,-20,-40,
-50,-40,-30,-30,-30,-30,-40,-50,
},
{
-20,-10,-10,-10,-10,-10,-10,-20,
-10,0,0,0,0,0,0,-10,
-10,0,5,10,10,5,0,-10,
-10,5,5,10,10,5,5,-10,
-10,0,10,10,10,10,0,-10,
-10,10,10,10,10,10,10,-10,
-10,5,0,0,0,0,5,-10,
-20,-10,-10,-10,-10,-10,-10,-20,
},
{
0,0,0,0,0,0,0,0,
5,10,10,10,10,10,10,5,
-5,0,0,0,0,0,0,-5,
-5,0,0,0,0,0,0,-5,
-5,0,0,0,0,0,0,-5,
-5,0,0,0,0,0,0,-5,
-5,0,0,0,0,0,0,-5,
-5,0,0,5,5,0,0,-5
},
{
-20,-10,-10,-5,-5,-10,-10,-20,
-10,0,0,0,0,0,0,-10,
-10,0,5,5,5,5,0,-10,
-5,0,5,5,5,5,0,-5,
0,0,5,5,5,5,0,-5,
-10,5,5,5,5,5,0,-10,
-10,0,5,0,0,0,0,-10,
-20,-10,-10,-5,-5,-10,-10,-20
},
{
-6,-8,-8,-10,-10,-8,-8,-6,
-6,-8,-8,-10,-10,-8,-8,-6,
-6,-8,-8,-10,-10,-8,-8,-6,
-6,-8,-8,-10,-10,-8,-8,-6,
-4,-6,-6,-8,-8,-6,-6,-4,
-2,-4,-4,-4,-4,-4,-4,-2,
-2,-2,-1,-1,-1,-1,-2,-2,
0,0,0,0,0,0,0,0,
}
};

constexpr int end_king_squares[]={
-10,-8,-6,-4,-4,-6,-8,-10,
-6,-4,-2,0,0,-2,-4,-6,
-6,-2,4,6,6,4,-2,-6,
-6,-2,6,8,8,6,-2,-6,
-6,-2,6,8,8,6,-2,-6,
-6,-2,4,6,6,4,-2,-6,
-6,-6,0,0,0,0,-6,-6,
-10,-6,-6,-6,-6,-6,-6,-10,
};

constexpr int piece_value[king+1]={0,0,100,320,330,500,980,1500};

inline int piece_weight(const piece_type pt){
  return piece_value[pt];
}

template<color PointOfView> constexpr int square_score(const piece_type pt,const square sq){
  if constexpr(PointOfView==white) return psts[pt-2][sq^63];
  else return psts[pt-2][sq];
}

inline int calculate_phase(){
  int phase=0;
  phase+=1*popcnt(bboards[w_knight]|bboards[b_knight]);
  phase+=1*popcnt(bboards[w_bishop]|bboards[b_bishop]);
  phase+=2*popcnt(bboards[w_rook]|bboards[b_rook]);
  phase+=4*popcnt(bboards[w_queen]|bboards[b_queen]);
  return std::clamp(phase*256/24,0,256);
}

template<color Us> int material_count(){
  return
    pawn_value*(popcnt(bboard<get_piece(Us,pawn)>())-popcnt(bboard<get_piece(!Us,pawn)>()))+
    knight_value*(popcnt(bboard<get_piece(Us,knight)>())-popcnt(bboard<get_piece(!Us,knight)>()))+
    bishop_value*(popcnt(bboard<get_piece(Us,bishop)>())-popcnt(bboard<get_piece(!Us,bishop)>()))+
    rook_value*(popcnt(bboard<get_piece(Us,rook)>())-popcnt(bboard<get_piece(!Us,rook)>()))+
    queen_value*(popcnt(bboard<get_piece(Us,queen)>())-popcnt(bboard<get_piece(!Us,queen)>()));
}

template<color Us> int midgame(){
  int value=material_count<Us>();
  constexpr color them=!Us;
  constexpr piece our_pawn=get_piece(Us,pawn);
  constexpr piece our_king=get_piece(Us,king);
  constexpr piece their_pawn=get_piece(them,pawn);
  constexpr piece their_king=get_piece(them,king);
  value+=king_safety<Us>(lsb(bboard<our_king>()),bboard<our_pawn>())-king_safety<them>(lsb(bboard<their_king>()),bboard<their_pawn>());
  constexpr bitboard rank567=rel_rank(Us,rank_5,rank_6,rank_7);
  constexpr bitboard rank234=rel_rank(Us,rank_2,rank_3,rank_4);
  value+=advanced_pawn_bonus*(popcnt(bboard<our_pawn>()&rank567)-popcnt(bboard<their_pawn>()&rank234));
  for(piece_type pt:{pawn,knight,bishop,rook,queen,king}){
    for(bitboard b=bboards[get_piece(Us,pt)];b;clear_lsb(b)) value+=square_score<Us>(pt,lsb(b));
    for(bitboard b=bboards[get_piece(them,pt)];b;clear_lsb(b)) value-=square_score<them>(pt,lsb(b));
  }
  return value;
}

template<color Us> int endgame(){
  int value=material_count<Us>();
  constexpr color them=!Us;
  constexpr piece our_king=get_piece(Us,king);
  constexpr piece their_king=get_piece(them,king);
  value+=end_king_squares[lsb(bboard<our_king>())];
  value-=end_king_squares[lsb(bboard<their_king>())];
  constexpr bitboard rank2=rel_rank(Us,rank_2);
  constexpr bitboard rank3=rel_rank(Us,rank_3);
  constexpr bitboard rank4=rel_rank(Us,rank_4);
  constexpr bitboard rank5=rel_rank(Us,rank_5);
  constexpr bitboard rank6=rel_rank(Us,rank_6);
  constexpr bitboard rank7=rel_rank(Us,rank_7);
  const bitboard our_pawn=bboard<get_piece(Us,pawn)>();
  const bitboard their_pawn=bboard<get_piece(them,pawn)>();
  value+=rank4_pawn_bonus*popcnt(our_pawn&rank4);
  value+=rank5_pawn_bonus*popcnt(our_pawn&rank5);
  value+=rank6_pawn_bonus*popcnt(our_pawn&rank6);
  value+=rank7_pawn_bonus*popcnt(our_pawn&rank7);
  value-=rank4_pawn_bonus*popcnt(their_pawn&rank5);
  value-=rank5_pawn_bonus*popcnt(their_pawn&rank4);
  value-=rank6_pawn_bonus*popcnt(their_pawn&rank3);
  value-=rank7_pawn_bonus*popcnt(their_pawn&rank2);
  return value;
}

template<color Perspective> int static_eval(){
  const int mg=midgame<Perspective>();
  const int eg=endgame<Perspective>();
  const int phase=calculate_phase();
  return (mg*phase+eg*(256-phase))/256;
}
