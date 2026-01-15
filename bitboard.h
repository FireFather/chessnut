#pragma once
#include <random>
#include <string>
#include "types.h"

constexpr bitboard k1=0x5555555555555555;
constexpr bitboard k2=0x3333333333333333;
constexpr bitboard k4=0x0f0f0f0f0f0f0f0f;
constexpr bitboard kf=0x0101010101010101;

inline int popcnt(bitboard b){
  b=b-((b>>1)&k1);
  b=(b&k2)+((b>>2)&k2);
  b=(b+(b>>4))&k4;
  b=(b*kf)>>56;
  return SC<int>(b);
}

constexpr int debruin[64]={
0,47,1,56,48,27,2,60,
57,49,41,37,28,16,3,61,
54,58,35,52,50,42,21,44,
38,32,29,23,17,11,4,62,
46,55,26,59,40,36,15,53,
34,51,20,43,31,22,10,45,
25,39,14,33,19,30,9,24,
13,18,8,12,7,6,5,63
};

inline int lsb(const bitboard b){
  return debruin[(0x03f79d71b4cb0a89*(b^(b-1)))>>58];
}

typedef struct{
  bitboard* ptr;
  bitboard mask;
  bitboard magic;
  int shift;
} magic_bb;

extern bitboard get_align_mask[num_squares][num_squares];
extern bitboard get_anti_diag[num_squares];
extern bitboard get_check_ray[num_squares][num_squares];
extern bitboard get_double_check[num_squares];
extern bitboard get_file_bb[num_squares];
extern bitboard get_king_attacks[num_squares];
extern bitboard get_king_shield[num_colors][num_squares];
extern bitboard get_knight_attacks[num_squares];
extern bitboard get_main_diag[num_squares];
extern bitboard get_passers[num_colors][4][1<<10];
extern bitboard get_pawn_attacks[num_colors][num_squares];
extern bitboard king_shield_magics[num_colors][num_squares];
extern bitboard pext_table[0x1a480];
extern int get_king_shield_scores[num_colors][num_squares][1<<6];
extern magic_bb magics[num_squares][2];
extern u8 get_castle_masks[num_colors][1<<5];
extern u8 get_center_dist[num_squares];
extern u8 get_dist[num_squares][num_squares];

std::string to_string(bitboard b);

namespace bitboards{
  void init();
}

constexpr bitboard file_abb=0x8080808080808080ull;
constexpr bitboard file_bbb=file_abb>>1;
constexpr bitboard file_cbb=file_abb>>2;
constexpr bitboard file_dbb=file_abb>>3;
constexpr bitboard file_ebb=file_abb>>4;
constexpr bitboard file_fbb=file_abb>>5;
constexpr bitboard file_gbb=file_abb>>6;
constexpr bitboard file_hbb=file_abb>>7;

constexpr bitboard rank_1_bb=0xffull;
constexpr bitboard rank_2_bb=rank_1_bb<<8;
constexpr bitboard rank_3_bb=rank_1_bb<<16;
constexpr bitboard rank_4_bb=rank_1_bb<<24;
constexpr bitboard rank_5_bb=rank_1_bb<<32;
constexpr bitboard rank_6_bb=rank_1_bb<<40;
constexpr bitboard rank_7_bb=rank_1_bb<<48;
constexpr bitboard rank_8_bb=rank_1_bb<<56;

template<dir D> constexpr bitboard shift(const bitboard bb){
  if constexpr(D==north) return bb<<8;
  else if constexpr(D==northeast) return (bb&~file_hbb)<<7;
  else if constexpr(D==east) return bb>>1;
  else if constexpr(D==southeast) return (bb&~file_hbb)>>9;
  else if constexpr(D==south) return bb>>8;
  else if constexpr(D==southwest) return (bb&~file_abb)>>7;
  else if constexpr(D==west) return bb<<1;
  else if constexpr(D==northwest) return (bb&~file_abb)<<9;
  else if constexpr(D==north+north) return bb<<16;
  else if constexpr(D==south+south) return bb>>16;
  else return bb;
}

template<dir D> constexpr bitboard shift_unsafe(const bitboard bb){
  if constexpr(D==north) return bb<<8;
  else if constexpr(D==northeast) return bb<<7;
  else if constexpr(D==east) return bb>>1;
  else if constexpr(D==southeast) return bb>>9;
  else if constexpr(D==south) return bb>>8;
  else if constexpr(D==southwest) return bb>>7;
  else if constexpr(D==west) return bb<<1;
  else if constexpr(D==northwest) return bb<<9;
  else if constexpr(D==north+north) return bb<<16;
  else if constexpr(D==south+south) return bb>>16;
  else return bb;
}

inline bitboard dist_from_center(const square sq){
  return get_center_dist[sq];
}

inline int square_dist(const square a,const square b){
  return get_dist[a][b];
}

inline bitboard align_mask(const square a,const square b){
  return get_align_mask[a][b];
}

inline bitboard main_diag(const square sq){
  return get_main_diag[sq];
}

inline bitboard anti_diag(const square sq){
  return get_anti_diag[sq];
}

inline bitboard file_bb(const square sq){
  return get_file_bb[sq];
}

inline bitboard double_check(const square sq){
  return get_double_check[sq];
}

inline bitboard check_ray(const square sq,const square checker){
  return get_check_ray[sq][checker];
}

constexpr bitboard square_bb(const square sq){
  return 1ull<<sq;
}

template<typename...Squares> constexpr bitboard square_bb(const square sq,Squares...sqs){
  return square_bb(sq)|square_bb(sqs...);
}

constexpr bitboard rel_rank(const color c,const rank r){
  return rank_1_bb<<8*(r^c*7);
}

template<typename...Ranks> constexpr bitboard rel_rank(color c,const rank r,Ranks...ranks){
  return rel_rank(c,r)|rel_rank(c,ranks...);
}

constexpr bitboard rel_file(const color c,const file f){
  return file_hbb<<(f^c*7);
}

template<typename...Files> constexpr bitboard rel_file(color c,const file f,Files...files){
  return rel_file(c,f)|rel_file(c,files...);
}

inline bitboard rank_bb(const square sq){
  return rank_1_bb<<8*(sq/8);
}

inline bitboard safe_step(const square sq,const int step){
  const square to=sq+step;
  return is_ok(to)&&square_dist(sq,to)<=2?square_bb(to):0;
}

inline bitboard mask(square sq,const dir d){
  switch(d){
  case northeast: return mask(sq,north)&mask(sq,east);
  case southeast: return mask(sq,south)&mask(sq,east);
  case southwest: return mask(sq,south)&mask(sq,west);
  case northwest: return mask(sq,north)&mask(sq,west);
  default:;
  }
  bitboard m=0;
  while(safe_step(sq,d)&&is_ok(sq+=d)) m|=d==north||d==south?rank_bb(sq):file_hbb<<sq%8;
  return m;
}

inline void clear_lsb(bitboard& bb){
  bb=bb&(bb-1);
}

inline bitboard lsb_mask(const bitboard bb){
  return bb&-static_cast<int64_t>(bb);
}

inline bool more_than_one(const bitboard bb){
  return (bb&(bb-1));
}

inline bitboard knight_attacks(const square sq){
  return get_knight_attacks[sq];
}

inline bitboard bishop_attacks(const square sq,bitboard bb){
  bb&=magics[sq][0].mask;
  bb*=magics[sq][0].magic;
  bb>>=magics[sq][0].shift;
  return magics[sq][0].ptr[bb];
}

inline bitboard bishop_xray(const square sq,const bitboard bb){
  return bishop_attacks(sq,bb^(bishop_attacks(sq,bb)&bb));
}

inline bitboard rook_attacks(const square sq,bitboard bb){
  bb&=magics[sq][1].mask;
  bb*=magics[sq][1].magic;
  bb>>=magics[sq][1].shift;
  return magics[sq][1].ptr[bb];
}

inline bitboard rook_xray(const square sq,const bitboard occupied){
  return rook_attacks(sq,occupied^(rook_attacks(sq,occupied)&occupied));
}

inline bitboard queen_attacks(const square sq,const bitboard occupied){
  return bishop_attacks(sq,occupied)|rook_attacks(sq,occupied);
}

inline bitboard king_attacks(const square sq){
  return get_king_attacks[sq];
}

template<color C> constexpr bitboard pawn_attacks(const square sq){
  return get_pawn_attacks[C][sq];
}

template<color C> constexpr bitboard pawn_attacks(const bitboard bb){
  return shift<rel_direction(C,northeast)>(bb)|shift<rel_direction(C,northwest)>(bb);
}

inline void toggle_square(bitboard& bb,const square sq){
  bb^=1ull<<sq;
}

template<color C> int king_safety(const square sq,bitboard bb){
  bb&=get_king_shield[C][sq];
  bb*=king_shield_magics[C][sq];
  bb>>=58;
  return get_king_shield_scores[C][sq][bb];
}

inline int file_dist(const square sq,const square bb){
  return std::abs(sq%8-bb%8);
}

inline int rank_dist(const square sq,const square bb){
  return std::abs(sq/8-bb/8);
}

extern bitboard rand64();
