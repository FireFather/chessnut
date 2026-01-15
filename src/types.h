#pragma once
#include <cstdint>
#include <string>

using bitboard=uint64_t;
using color=int;
using dir=int;
using file=int;
using move_type=uint16_t;
using move=uint16_t;
using movelist=uint32_t;
using piece_type=int;
using piece=int;
using rank=int;
using score=int;
using square=int;
using u8=uint8_t;

#define SC static_cast
#define SO std::cout
#define SE std::endl

constexpr int mate=32000;
constexpr int max_depth=64;
constexpr int max_plies=128;
constexpr int infinite=mate+1;
constexpr int mate_in_max_ply=mate-max_plies;
constexpr int mated_in_max_ply=-mate_in_max_ply;
constexpr int no_eval=0x7fffffff;
constexpr move no_move=0;
constexpr move nullmove=0;

inline bool is_win(const int eval){
  return eval>=mate_in_max_ply;
}

inline bool is_loss(const int eval){
  return eval<=mated_in_max_ply;
}

extern const std::string piece_to_char;

enum squares:u8{
  h1=0,g1=1,f1=2,e1=3,d1=4,c1=5,b1=6,a1=7,
  h2=8,g2=9,f2=10,e2=11,d2=12,c2=13,b2=14,a2=15,
  h3=16,g3=17,f3=18,e3=19,d3=20,c3=21,b3=22,a3=23,
  h4=24,g4=25,f4=26,e4=27,d4=28,c4=29,b4=30,a4=31,
  h5=32,g5=33,f5=34,e5=35,d5=36,c5=37,b5=38,a5=39,
  h6=40,g6=41,f6=42,e6=43,d6=44,c6=45,b6=46,a6=47,
  h7=48,g7=49,f7=50,e7=51,d7=52,c7=53,b7=54,a7=55,
  h8=56,g8=57,f8=58,e8=59,d8=60,c8=61,b8=62,a8=63,
  num_squares=64,no_sq=64
};

enum piecetypes:u8{
  no_piecetype=0,pawn=2,knight=3,bishop=4,rook=5,queen=6,king=7,num_piecetypes=6,
};

enum pieces:u8{
  no_piece=0,w_pawn=2,w_knight=3,w_bishop=4,w_rook=5,w_queen=6,w_king=7,b_pawn=10,
  b_knight=11,b_bishop=12,b_rook=13,b_queen=14,b_king=15,num_pieces=16,
};

enum colors:u8{
  white=0,black=1,num_colors=2
};

enum move_types:uint16_t{
  normal=0<<12,promote=1<<12,en_passant=2<<12,castle=3<<12,knight_promote=promote,bishop_promote=promote+((bishop-knight)<<14),rook_promote=promote+((rook-knight)<<14),queen_promote=promote+((queen-knight)<<14)
};

enum files:u8{
  file_h,file_g,file_f,file_e,file_d,file_c,file_b,file_a
};

enum ranks:u8{
  rank_1,rank_2,rank_3,rank_4,rank_5,rank_6,rank_7,rank_8
};

enum directions:int8_t{
  north=8,east=-1,south=-8,west=1,northeast=north+east,southeast=south+east,northwest=north+west,southwest=south+west,
};

constexpr void set_score(movelist& lm,const score value){
  lm+=SC<movelist>(value<<16);
}

constexpr score get_score(const movelist lm){
  return SC<score>(lm>>16);
}

constexpr dir rel_direction(const color c,const dir d){
  return c==white?d:-d;
}

constexpr piece get_piece(const color c,const piece_type pt){
  return pt+(c<<3);
}

constexpr piece_type get_piecetype(const piece p){
  return p&7;
}

constexpr color get_color(const piece p){
  return p>>3;
}

constexpr move make_move(const square from,const square to){
  return SC<move>(from+(to<<6));
}

template<move_type T> constexpr move make_move(const square from,const square to){
  return T+static_cast<move>(from)+static_cast<move>(to<<6);
}

inline bool is_ok(const square s){
  return s>=h1&&s<=a8;
}

constexpr square from_sq(const move m){
  return m&0x3f;
}

constexpr square to_sq(const move m){
  return m>>6&0x3f;
}

constexpr move from_to(const move m){
  return m&0xfff;
}

constexpr move_type get_movetype(const move m){
  return m&0x3000;
}

constexpr piece_type get_promotetype(const move m){
  return (m>>14)+knight;
}

inline rank get_rank(const square s){
  return s>>3;
}

inline square get_square(const rank r,const file f){
  return r*8+f;
}
