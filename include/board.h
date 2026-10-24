#pragma once

#include <stdint.h>

#include "types.h"

static const char piece_dict[13] = "PNBRQKpnbrqk";

struct position {
    uint64_t pieces[COLOR_TYPES][PIECE_TYPES];
    uint64_t occupancy[COLOR_TYPES];
    uint64_t all;
    int mailbox[SQUARES];
    int side_to_play;
    int castling_rights;
    int en_passant;
    int halfmove_clock;
    int fullmove_clock;
};

struct prev_pos_info {
    int piece_last_captured;
    int prev_castling_rights;
    int prev_en_passant;
    int prev_halfmove_clock;
};

struct position FEN_to_position(const char* fen);
void init_castling_rights_by_sq(void);
uint64_t sq_attackers(const struct position* position, int sq, int attacking_color);
int is_king_in_check(const struct position* position, int color);

void make(struct position* position, MOVE move, struct prev_pos_info* prev_pos_info);
void unmake(struct position* position, MOVE last_move, const struct prev_pos_info* prev_pos_info);

void print_position(const struct position* position);
void print_bitboard(uint64_t bitboard);
void print_position_metadata(const struct position* position);
