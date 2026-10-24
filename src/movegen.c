#include "movegen.h"
#include "attacks.h"

//CONSTANTS-DEFINITIONS-DATA STRUCTURES
static const uint64_t rank_1 = 0x00000000000000FFULL;
static const uint64_t rank_2 = 0x000000000000FF00ULL;
static const uint64_t rank_4 = 0x00000000FF000000ULL;
static const uint64_t rank_5 = 0x000000FF00000000ULL;
static const uint64_t rank_7 = 0x00FF000000000000ULL;
static const uint64_t rank_8 = 0xFF00000000000000ULL;


//PRIVATE FUNCTIONS
//king castling-handling helpers
static int can_castle_kingside(const struct position* position)
{
    int my_color = position->side_to_play;
    int opposite_color = (my_color + 1) % 2;
    int is_kingside_castling_allowed = position->castling_rights & (1 << (2 * my_color + 0));
    if (!is_kingside_castling_allowed) return 0;

    int is_f1_or_f8_clear = !(position->all & (1ULL << (F1 + (my_color * 56))));
    int is_g1_or_g8_clear = !(position->all & (1ULL << (G1 + (my_color * 56))));
    int is_path_clear = is_f1_or_f8_clear && is_g1_or_g8_clear;
    if (!is_path_clear) return 0;
    
    uint64_t is_king_in_check = sq_attackers(position, E1 + (my_color * 56), opposite_color);
    if (is_king_in_check) return 0;

    uint64_t f1_or_f8_attackers = sq_attackers(position, F1 + (my_color * 56), opposite_color);
    uint64_t g1_or_g8_attackers = sq_attackers(position, G1 + (my_color * 56), opposite_color);
    int is_going_through_check = f1_or_f8_attackers || g1_or_g8_attackers;
    if (is_going_through_check) return 0;

    return 1;
}
static int can_castle_queenside(const struct position* position)
{
    int my_color = position->side_to_play;
    int opposite_color = (my_color + 1) % 2;
    int is_queenside_castling_allowed = position->castling_rights & (1 << (2 * my_color + 1));
    if (!is_queenside_castling_allowed) return 0;

    int is_d1_or_d8_clear = !(position->all & (1ULL << (D1 + (my_color * 56))));
    int is_c1_or_c8_clear = !(position->all & (1ULL << (C1 + (my_color * 56))));
    int is_b1_or_b8_clear = !(position->all & (1ULL << (B1 + (my_color * 56))));
    int is_path_clear = is_d1_or_d8_clear && is_c1_or_c8_clear && is_b1_or_b8_clear;
    if (!is_path_clear) return 0;
    
    uint64_t is_king_in_check = sq_attackers(position, E1 + (my_color * 56), opposite_color);
    if (is_king_in_check) return 0;

    uint64_t d1_or_d8_attackers = sq_attackers(position, D1 + (my_color * 56), opposite_color);
    uint64_t c1_or_c8_attackers = sq_attackers(position, C1 + (my_color * 56), opposite_color);
    int is_going_through_check = d1_or_d8_attackers || c1_or_c8_attackers;
    if (is_going_through_check) return 0;

    return 1;
}

//pawn helpers
static void gen_white_pawns_simple_moves(const struct position* position, struct moves* moves)
{
    uint64_t pawns_bitboard = position->pieces[WHITE][PAWN];

    uint64_t pawns_1_foward_targets = (pawns_bitboard << 8) & ~(position->all); //convert to pawns 1-step targets
    pawns_1_foward_targets &= ~rank_8; //filter to pawns targets not on rank 8 (not promoting)

    uint64_t pawns_2_foward_targets = pawns_bitboard & rank_2; //filter to pawns on rank 2
    pawns_2_foward_targets = (pawns_2_foward_targets << 8) & ~position->all; //convert to pawns 1-step targets
    pawns_2_foward_targets = (pawns_2_foward_targets << 8) & ~position->all; //convert to pawns 2-step targets

    while (pawns_1_foward_targets) {
        int target_sq = __builtin_ctzll(pawns_1_foward_targets);
        int start_sq = target_sq - 8;

        MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_SIMPLE << 12);
        moves->move_list[moves->new_move_index] = new_move;
        moves->new_move_index++;

        pawns_1_foward_targets &= (pawns_1_foward_targets - 1);
    }

    while (pawns_2_foward_targets) {
        int target_sq = __builtin_ctzll(pawns_2_foward_targets);
        int start_sq = target_sq - 16;

        MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_SIMPLE << 12);
        moves->move_list[moves->new_move_index] = new_move;
        moves->new_move_index++;

        pawns_2_foward_targets &= (pawns_2_foward_targets - 1);
    }
}
static void gen_white_pawns_promotion_moves(const struct position* position, struct moves* moves)
{
    uint64_t pawns_bitboard = position->pieces[WHITE][PAWN];

    uint64_t pawns_on_rank_7 = pawns_bitboard & rank_7;
    uint64_t pawns_targets_for_promotion = (pawns_on_rank_7 << 8) & ~position->all;
    while (pawns_targets_for_promotion) {
        int target_sq = __builtin_ctzll(pawns_targets_for_promotion);
        int start_sq = target_sq - 8;

        for (int piece = KNIGHT; piece <= QUEEN; piece++) {
            MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_PROMOTION << 12) 
                            | ((piece - KNIGHT) << 14);
            moves->move_list[moves->new_move_index] = new_move;
            moves->new_move_index++;
        }
        pawns_targets_for_promotion &= (pawns_targets_for_promotion - 1);
    }
}
static void gen_black_pawns_simple_moves(const struct position* position, struct moves* moves)
{
    uint64_t pawns_bitboard = position->pieces[BLACK][PAWN];

    uint64_t pawns_1_foward_targets = (pawns_bitboard >> 8) & ~(position->all); //convert to pawns 1-step targets
    pawns_1_foward_targets &= ~rank_1; //filter to pawns targets not on rank 1 (not promoting)

    uint64_t pawns_2_foward_targets = pawns_bitboard & rank_7; //filter to pawns on rank 7
    pawns_2_foward_targets = (pawns_2_foward_targets >> 8) & ~position->all; //convert to pawns 1-step targets
    pawns_2_foward_targets = (pawns_2_foward_targets >> 8) & ~position->all; //convert to pawns 2-step targets

    while (pawns_1_foward_targets) {
        int target_sq = __builtin_ctzll(pawns_1_foward_targets);
        int start_sq = target_sq + 8;

        MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_SIMPLE << 12);
        moves->move_list[moves->new_move_index] = new_move;
        moves->new_move_index++;

        pawns_1_foward_targets &= (pawns_1_foward_targets - 1);
    }

    while (pawns_2_foward_targets) {
        int target_sq = __builtin_ctzll(pawns_2_foward_targets);
        int start_sq = target_sq + 16;

        MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_SIMPLE << 12);
        moves->move_list[moves->new_move_index] = new_move;
        moves->new_move_index++;

        pawns_2_foward_targets &= (pawns_2_foward_targets - 1);
    }
}
static void gen_black_pawns_promotion_moves(const struct position* position, struct moves* moves)
{
    uint64_t pawns_bitboard = position->pieces[BLACK][PAWN];

    uint64_t pawns_on_rank_2 = pawns_bitboard & rank_2;
    uint64_t pawns_targets_for_promotion = (pawns_on_rank_2 >> 8) & ~position->all;
    while (pawns_targets_for_promotion) {
        int target_sq = __builtin_ctzll(pawns_targets_for_promotion);
        int start_sq = target_sq + 8;

        for (int piece = KNIGHT; piece <= QUEEN; piece++) {
            MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_PROMOTION << 12) 
                            | ((piece - KNIGHT) << 14);
            moves->move_list[moves->new_move_index] = new_move;
            moves->new_move_index++;
        }

        pawns_targets_for_promotion &= (pawns_targets_for_promotion - 1);
    }
}
static void gen_pawns_simple_captures(const struct position* position, struct moves* moves)
{
    int my_color = position->side_to_play;
    int opposite_color = (my_color + 1) % 2;

    uint64_t pawns_bitboard = position->pieces[my_color][PAWN];
    while (pawns_bitboard) {
        int start_sq = __builtin_ctzll(pawns_bitboard);

        //pawn targets that arent en_passant nor capture and promotion
        uint64_t pawn_targets = get_pawn_attack(start_sq, my_color) & position->occupancy[opposite_color]
                                & ~rank_1 & ~rank_8;
        while (pawn_targets) {
            int target_sq = __builtin_ctzll(pawn_targets);
                    
            MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_SIMPLE << 12);
            moves->move_list[moves->new_move_index] = new_move;
            moves->new_move_index++;

            pawn_targets &= (pawn_targets - 1);
        }
        pawns_bitboard &= (pawns_bitboard - 1);
    }
}
static void gen_pawns_capture_promotions(const struct position* position, struct moves* moves)
{
    int my_color = position->side_to_play;
    int opposite_color = (my_color + 1) % 2;

    uint64_t pawns_bitboard = position->pieces[my_color][PAWN];
    while (pawns_bitboard) {
        int start_sq = __builtin_ctzll(pawns_bitboard);

        uint64_t pawn_targets = get_pawn_attack(start_sq, my_color) & position->occupancy[opposite_color]
                                & (rank_1 | rank_8);
        while (pawn_targets) {
            int target_sq = __builtin_ctzll(pawn_targets);

            for (int piece = KNIGHT; piece <= QUEEN; piece++) {
                MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_PROMOTION << 12) 
                                | ((piece - KNIGHT) << 14);
                moves->move_list[moves->new_move_index] = new_move;
                moves->new_move_index++;
            }

            pawn_targets &= (pawn_targets - 1);
        }

        pawns_bitboard &= (pawns_bitboard - 1);
    }
}
static void gen_pawns_en_passant_moves(const struct position* position, struct moves* moves)
{
    int my_color = position->side_to_play;

    int en_passant_sq = position->en_passant;
    if (en_passant_sq == -1) return;

    //filter to pawns that are candidates to capture en passant
    uint64_t pawns_bitboard = position->pieces[my_color][PAWN] & (rank_4 | rank_5);
    while (pawns_bitboard) {
        int start_sq = __builtin_ctzll(pawns_bitboard);

        uint64_t pawn_target = get_pawn_attack(start_sq, my_color) & (1ULL << en_passant_sq);
        if (pawn_target) {
            int target_sq = __builtin_ctzll(pawn_target);

            MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_EN_PASSANT << 12);
            moves->move_list[moves->new_move_index] = new_move;
            moves->new_move_index++;
        }
        
        pawns_bitboard &= (pawns_bitboard - 1);
    }
}

//move generators
static void generate_king_moves(const struct position* position, struct moves* moves)
{
    int my_color = position->side_to_play;
    int start_sq = __builtin_ctzll(position->pieces[my_color][KING]);

    uint64_t king_targets = get_king_attack(start_sq) & ~position->occupancy[my_color];
    while (king_targets) {
        int target_sq = __builtin_ctzll(king_targets);

        MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_SIMPLE << 12);
        moves->move_list[moves->new_move_index] = new_move;
        moves->new_move_index++;

        king_targets &= (king_targets - 1);
    }
    
    if (can_castle_kingside(position)) {
        MOVE new_move = (start_sq << 0) | ((start_sq + 2) << 6) | (MOVE_CASTLING << 12);
        moves->move_list[moves->new_move_index] = new_move;
        moves->new_move_index++;
    }
    if (can_castle_queenside(position)) {
        MOVE new_move = (start_sq << 0) | ((start_sq - 2) << 6) | (MOVE_CASTLING << 12);
        moves->move_list[moves->new_move_index] = new_move;
        moves->new_move_index++;
    }
}
static void generate_knights_moves(const struct position* position, struct moves* moves)
{
    int my_color = position->side_to_play;
    
    uint64_t knights_bitboard = position->pieces[my_color][KNIGHT];
    while (knights_bitboard) {
        int start_sq = __builtin_ctzll(knights_bitboard);

        uint64_t knight_targets = get_knight_attack(start_sq) & ~position->occupancy[my_color];
        while (knight_targets) {
            int target_sq = __builtin_ctzll(knight_targets);

            MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_SIMPLE << 12);
            moves->move_list[moves->new_move_index] = new_move;
            moves->new_move_index++;
            
            knight_targets &= (knight_targets - 1);
        }
        knights_bitboard &= (knights_bitboard - 1);
    }
}
static void generate_rooks_moves(const struct position* position, struct moves* moves)
{
    int my_color = position->side_to_play;

    uint64_t rooks_bitboard = position->pieces[my_color][ROOK];
    while (rooks_bitboard) {
        int start_sq = __builtin_ctzll(rooks_bitboard);

        uint64_t rook_targets = get_rook_attack(start_sq, position->all) & ~(position->occupancy[my_color]);
        while (rook_targets) {
            int target_sq = __builtin_ctzll(rook_targets);

            MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_SIMPLE << 12);
            moves->move_list[moves->new_move_index] = new_move;
            moves->new_move_index++;

            rook_targets &= (rook_targets - 1);
        }
        rooks_bitboard &= (rooks_bitboard - 1);
    }
}
static void generate_bishops_moves(const struct position* position, struct moves* moves)
{
    int my_color = position->side_to_play;

    uint64_t bishops_bitboard = position->pieces[my_color][BISHOP];
    while (bishops_bitboard) {
        int start_sq = __builtin_ctzll(bishops_bitboard);

        uint64_t bishop_targets = get_bishop_attack(start_sq, position->all) & ~(position->occupancy[my_color]);
        while (bishop_targets) {
            int target_sq = __builtin_ctzll(bishop_targets);

            MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_SIMPLE << 12);
            moves->move_list[moves->new_move_index] = new_move;
            moves->new_move_index++;

            bishop_targets &= (bishop_targets - 1);
        }
        bishops_bitboard &= (bishops_bitboard - 1);
    }
}
static void generate_queens_moves(const struct position* position, struct moves* moves)
{
    int my_color = position->side_to_play;

    uint64_t queens_bitboard = position->pieces[my_color][QUEEN];
    while (queens_bitboard) {
        int start_sq = __builtin_ctzll(queens_bitboard);

        uint64_t queen_targets = get_queen_attack(start_sq, position->all) & ~(position->occupancy[my_color]);
        while (queen_targets) {
            int target_sq = __builtin_ctzll(queen_targets);

            MOVE new_move = (start_sq << 0) | (target_sq << 6) | (MOVE_SIMPLE << 12);
            moves->move_list[moves->new_move_index] = new_move;
            moves->new_move_index++;

            queen_targets &= (queen_targets - 1);
        }
        queens_bitboard &= (queens_bitboard - 1);
    }
}
static void generate_pawns_moves(const struct position* position, struct moves* moves)
{
    int my_color = position->side_to_play;

    switch (my_color) {
        case WHITE: 
            gen_white_pawns_simple_moves(position, moves);
            gen_white_pawns_promotion_moves(position, moves);
            break;
        case BLACK:
            gen_black_pawns_simple_moves(position, moves);
            gen_black_pawns_promotion_moves(position, moves);
            break;
        default: break;
    }

    gen_pawns_simple_captures(position, moves);
    gen_pawns_capture_promotions(position, moves);
    gen_pawns_en_passant_moves(position, moves);
}


//PUBLIC FUNCTIONS
void generate_moves(const struct position* position, struct moves* moves)
{
    moves->new_move_index = 0;

    generate_pawns_moves(position, moves);
    generate_knights_moves(position, moves);
    generate_bishops_moves(position, moves);
    generate_rooks_moves(position, moves);
    generate_queens_moves(position, moves);
    generate_king_moves(position, moves);
}
