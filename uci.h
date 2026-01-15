#pragma once
#include <string>
#include "types.h"

constexpr auto engine="chessnut";
constexpr auto version="1.0";
constexpr auto author="me";

move uci_to_move(const std::string& str);
namespace uci{
  void loop();
}

inline std::string square_to_uci(const square sq){
  return std::string(1,"hgfedcba"[sq%8])+std::string(1,"12345678"[sq/8]);
}

inline std::string move_to_uci(const move m){
  return get_movetype(m)==promote?square_to_uci(from_sq(m))+square_to_uci(to_sq(m))+"   nbrq"[get_promotetype(m)]
    :square_to_uci(from_sq(m))+square_to_uci(to_sq(m));
}

inline square uci_to_square(const std::string& str){
  return 8*(str[1]-'1')+'h'-str[0];
}
