#include "perft.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include "hash.h"
#include "movegen.h"
#include "position.h"
#include "uci.h"

template<color Stm> static std::string pv(){
  if(rep_table::draw()) return "";
  const move best=hash_table::get_move();
  if(best==no_move) return "";
  bool legal=false;
  for(move_list<Stm> moves;auto m:moves) if(SC<move>(m)==best) legal=true;
  if(!legal) return "";
  do_move<Stm>(best);
  std::string line=move_to_uci(best)+" "+pv<!Stm>();
  undo_move<Stm>(best);
  return line;
}

std::string pv(){
  return position::white_to_move()?::pv<white>(): ::pv<black>();
}

namespace{
  template<bool Root,color SideToMove> bitboard perft(const int depth){
    if(depth==0) return 1;
    move_list<SideToMove> moves;
    if(depth==1&&!Root) return moves.size();
    bitboard nodes=0;
    int num=0;
    for(auto m:moves){
      num++;
      do_move<SideToMove>(SC<move>(m));
      const bitboard count=perft<false,!SideToMove>(depth-1);
      undo_move<SideToMove>(SC<move>(m));
      nodes+=count;
      if(Root)
        SO<<num<<" "<<move_to_uci(SC<move>(m))<<" "<<count<<SE;
    }
    return nodes;
  }
}

void handle_perft(std::istringstream& is){
  if(int depth;is>>depth){
    const auto start=std::chrono::steady_clock::now();
    const bitboard result=position::white_to_move()
      ?perft<true,white>(depth)
      :perft<true,black>(depth);
    const auto end=std::chrono::steady_clock::now();
    SO<<"nodes "<<result<<SE;
    SO<<"time "
      <<std::chrono::duration_cast<std::chrono::microseconds>(end-start).count()/1000
      <<" ms\n"<<SE;
  }
}

move* get_moves(move* list){
  if(position::white_to_move()){
    for(move_list<white> moves;const auto m:moves) *list++=SC<move>(m);
  } else{
    for(move_list<black> moves;const auto m:moves) *list++=SC<move>(m);
  }
  return list;
}
