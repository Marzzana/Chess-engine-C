#pragma once

#include "types.h"
#include "board.h"

enum max_possible_moves {MAX_POSSIBLE_MOVES = 218};

struct moves {
    int new_move_index;
    MOVE move_list[MAX_POSSIBLE_MOVES];
};

void generate_moves(const struct position* position, struct moves* moves);
