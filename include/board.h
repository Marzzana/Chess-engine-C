#pragma once

#include <stdint.h>

#include "types.h"

static const char piece_dict[13] = "PNBRQKpnbrqk";

struct Position {
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

struct Position FEN_to_Position(char* fen);
uint64_t sq_attackers(const struct Position* position, int sq, int attacking_color);

void make(struct Position* position, MOVE move);
void unmake(struct Position* position, MOVE move);

void print_position(const struct Position* position);
void print_bitboard(uint64_t bitboard);
void print_position_metadata(const struct Position* position);
