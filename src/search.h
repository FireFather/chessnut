#pragma once
#include <chrono>
#include <thread>
#include "types.h"

constexpr int base_reduction_offset=1087;
constexpr int deepen_thresh_base=43;
constexpr int deepen_thresh_scale=2;
constexpr int delta_scale=764;

constexpr int futility_depth_limit=7;
constexpr int futility_depth_scale=116;
constexpr int futility_static_margin=200;

constexpr int improvement_scale_den=512;
constexpr int improvement_scale_num=191;

constexpr int lmr_divisor=1024;
constexpr int lmr_reduction_thresh_1=3495;
constexpr int lmr_reduction_thresh_2=5510;

constexpr int min_depth_for_extra_reduction=2;
constexpr int move_count_penalty=32;
constexpr int no_tt_move_penalty=1156;

constexpr int null_move_base_reduction=3;
constexpr int null_move_bonus1=1;
constexpr int null_move_bonus2=1;
constexpr int null_move_cap=6;
constexpr int null_move_min_depth=3;
constexpr int null_move_depth_div=4;
constexpr int null_move_divisor=232;
constexpr int null_move_margin1=150;
constexpr int null_move_margin2=300;

constexpr int search_window=50;
constexpr int shallow_thresh=9;

constexpr double log_reduction_base=2954.0;
constexpr double log_reduction_scale=128.0;

struct search_status{
  bitboard nodes;
  int root_delta;
  move best_move;
  volatile bool search_stop;
};

extern search_status status;
extern int reductions[max_plies];

enum node_type:u8{
  root_node,pv_node,nonpv_node
};

using search_info=struct{
  int static_ev;
  int ply;
  move killers[2];
};

inline bitboard get_ms(){
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

namespace search{
  void init();
  void go(bitboard thinktime=0,int depth=max_depth);
}
bitboard compute_think_time(int time_left,int increment,int moves_to_go=30);
