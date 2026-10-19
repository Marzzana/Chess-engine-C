#pragma once

#include "types.h"
#include "board.h"

enum Max_Possible_Moves {MAX_POSSIBLE_MOVES = 218};

struct Moves {
    int new_move_index;
    MOVE move_list[MAX_POSSIBLE_MOVES];
};

void generate_moves(const struct Position* position, struct Moves* moves);
