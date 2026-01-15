#pragma once
#include "types.h"

enum bound_type:u8{
  exact,upper,lower
};

extern size_t tt_size;
constexpr int rt_size=1<<10;

using hash_entry=struct{
  bitboard key;
  int eval;
  move best_move;
  u8 depth;
  bound_type flag;
};
using rep_entry=struct{
  bitboard key;
  u8 occurrences;
};
namespace hash_table{
  int probe(int depth,int alpha,int beta,int ply_from_root);
  void store(u8 depth,bound_type flag,int eval,move best_move,int ply_from_root);
  move get_move();
  void clear();
}
namespace rep_table{
  bool draw();
  void push();
  void pop();
  void clear();
}

extern hash_entry* hashtable;
extern rep_entry reptable[rt_size];
void resize_tt(size_t mb);
