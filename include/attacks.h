#pragma once

#include <stdint.h>

#include "types.h"

//publics
void attacks_init(void);
uint64_t get_knight_attack(int square);
uint64_t get_king_attack(int square);
uint64_t get_pawn_attack(int square, int color);
uint64_t get_rook_attack(int square, uint64_t occupancy_bitboard);
uint64_t get_bishop_attack(int square, uint64_t occupancy_bitboard);
uint64_t get_queen_attack(int square, uint64_t occupancy_bitboard);
