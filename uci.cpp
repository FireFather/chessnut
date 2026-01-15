#include "uci.h"
#include <iostream>
#include <sstream>
#include "hash.h"
#include "movegen.h"
#include "perft.h"
#include "position.h"
#include "search.h"

const std::string start_fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

struct time_control{
  int wtime=0,btime=0;
  int winc=0,binc=0;
  int movestogo=30;
  int movetime=-1;
  int depth=-1;
  bool analyze=false;

  void parse(std::istringstream& is){
    std::string token;
    while(is>>token){
      if(token=="wtime") is>>wtime;
      else if(token=="btime") is>>btime;
      else if(token=="winc") is>>winc;
      else if(token=="binc") is>>binc;
      else if(token=="movestogo") is>>movestogo;
      else if(token=="movetime") is>>movetime;
      else if(token=="depth") is>>depth;
      else if(token=="infinite") analyze=true;
    }
  }

  [[nodiscard]] bitboard compute_think_time(const bool white_to_move) const{
    if(movetime>=0) return movetime;
    int time=white_to_move?wtime:btime;
    const int inc=white_to_move?winc:binc;
    if(analyze) time=INT_MAX;
    return ::compute_think_time(time,inc,movestogo);
  }
};

namespace{
  void handle_uci(){
    SO<<"id name "<<engine<<" "<<version<<SE;
    SO<<"id author "<<author<<SE;
    SO<<"option name Hash type spin default 64 min 1 max 16384"<<SE;
    SO<<"uciok"<<SE;
  }

  void handle_newgame(){
    hash_table::clear();
    rep_table::clear();
    rep_table::push();
  }

  void handle_position(std::istringstream& is){
    std::string fen,token;
    is>>token;
    if(token=="startpos"){
      fen=start_fen;
      is>>token;
    } else if(token=="fen") for(;is>>token&&token!="moves";fen+=token+" "){}
    position::set(fen);
    for(move m;is>>token&&((m=uci_to_move(token)));position::commit_move(m));
  }

  void handle_setoption(std::istringstream& is){
    std::string token,name,value;
    while(is>>token){
      if(token=="name"){
        while(is>>token&&token!="value") name+=token+" ";
        name=name.substr(0,name.size()-1);
      }
      if(token=="value") is>>value;
    }
    if(name=="Hash") resize_tt(std::stoi(value));
  }

  void handle_go(std::istringstream& is){
    time_control tc;
    tc.parse(is);
    if(tc.analyze) search::go(INT_MAX,-1);
    else if(tc.depth>0) search::go(0,tc.depth);
    else search::go(tc.compute_think_time(position::white_to_move()),-1);
  }
}

void uci::loop(){
  position::set(start_fen);
  std::string cmd,token;
  do{
    std::getline(std::cin,cmd);
    std::istringstream is(cmd);
    is>>token;
    if(token=="uci"){
      handle_uci();
    } else if(token=="isready")
      SO<<"readyok"<<SE;
    else if(token=="ucinewgame") handle_newgame();
    else if(token=="setoption") handle_setoption(is);
    else if(token=="position") handle_position(is);
    else if(token=="go") handle_go(is);
    else if(token=="stop") status.search_stop=true;
    else if(token=="perft") handle_perft(is);
    else if(token=="d")
      SO<<position::to_string()<<SE;
  } while(cmd!="quit");
}

move uci_to_move(const std::string& str){
  for(move list[max_moves],* m=list,* end=get_moves(list);m!=end;m++) if(move_to_uci(*m)==str) return *m;
  return nullmove;
}
