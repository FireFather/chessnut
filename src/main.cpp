#include "bitboard.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "uci.h"

int main(){
  bitboards::init();
  search::init();
  position::init();
  move_gen::init();
  resize_tt(64);
  uci::loop();
}
