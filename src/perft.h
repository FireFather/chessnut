#pragma once
#include <string>

#include "types.h"

move* get_moves(move* list);
std::string pv();
void handle_perft(std::istringstream& is);
