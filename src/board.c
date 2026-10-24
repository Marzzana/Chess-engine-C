#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "board.h"
#include "attacks.h"

//CONSTANTS-DEFINITIONS-DATA STRUCTURES
static int castling_rights_by_sq[SQUARES] = {0};


//PRIVATE FUNCTIONS
//position setters
static char* set_boards(struct position* position, char* fen)
{
    int rank = 7;
    int file = 0;

    for (int i = 0; i < SQUARES ; i++)
        position->mailbox[i] = EMPTY;

    while (*fen != ' ') {
        if (*fen == '/') {
            fen++;
            rank--;
            file = 0;
        }
        
        int pos = (8 * rank) + file;
        
        int color = 0;
        if (isupper(*fen)) {
            position->occupancy[WHITE] |= (1ULL << pos);
            color = WHITE;
        }
        if (islower(*fen)) {
            position->occupancy[BLACK] |= (1ULL << pos);
            color = BLACK;
        }
        
        switch (tolower(*fen)) {
            case 'k':
                position->pieces[color][KING] |= (1ULL << pos);
                position->mailbox[pos] = (PIECE_TYPES * color) + KING;
                break;
            case 'q': 
                position->pieces[color][QUEEN] |= (1ULL << pos);
                position->mailbox[pos] = (PIECE_TYPES * color) + QUEEN;
                break;
            case 'r': 
                position->pieces[color][ROOK] |= (1ULL << pos);
                position->mailbox[pos] = (PIECE_TYPES * color) + ROOK;
                break;
            case 'n': 
                position->pieces[color][KNIGHT] |= (1ULL << pos);
                position->mailbox[pos] = (PIECE_TYPES * color) + KNIGHT;
                break;
            case 'b': 
                position->pieces[color][BISHOP] |= (1ULL << pos);
                position->mailbox[pos] = (PIECE_TYPES * color) + BISHOP;
                break;
            case 'p': 
                position->pieces[color][PAWN] |= (1ULL << pos);
                position->mailbox[pos] = (PIECE_TYPES * color) + PAWN;
                break;
            default: file += (*fen - '0') - 1;
        }

        file++;
        fen++;
    }
    position->all = position->occupancy[WHITE] | position->occupancy[BLACK];
    
    return ++fen;
}
static char* set_side_to_play(struct position* position, char* fen)
{
    if (*fen == 'w') 
        position->side_to_play = WHITE;
    if (*fen == 'b')
        position->side_to_play = BLACK;

    return fen += 2;
}
static char* set_castling_rights(struct position* position, char* fen)
{
    position->castling_rights = 0;

    while (*fen != ' ') {
        switch (*fen) {
        case 'K': position->castling_rights |= WHITE_KINGSIDE; break;
        case 'Q': position->castling_rights |= WHITE_QUEENSIDE; break;
        case 'k': position->castling_rights |= BLACK_KINGSIDE; break;
        case 'q': position->castling_rights |= BLACK_QUEENSIDE; break;
        default: break;
        }
        fen++;
    }
    return ++fen;
}
static char* set_en_passant(struct position* position, char* fen)
{
    if (*fen == '-') {
        position->en_passant = -1;
    }
    else {
        int file = *fen - 'a';
        fen++;
        int rank = *fen - '0' - 1;
        position->en_passant = 8 * rank + file;
    }
    
    return fen += 2;
}
static char* set_halfmove_clock(struct position* position, char* fen)
{
    position->halfmove_clock = atoi(fen);

    while (*fen != ' ')
        fen++;
    return ++fen;
}
static void set_fullmove_clock(struct position* position, char* fen)
{
    position->fullmove_clock = atoi(fen);
}

//make and unmake helper functions
static void remove_piece_from_sq(struct position* position, int piece, int piece_color, int sq)
{
    position->pieces[piece_color][piece] &= ~(1ULL << sq);
    position->occupancy[piece_color] &= ~(1ULL << sq);
    position->all &= ~(1ULL << sq);
    position->mailbox[sq] = EMPTY;
}
static void put_piece_on_sq(struct position* position, int piece, int piece_color, int sq)
{
    position->pieces[piece_color][piece] |= (1ULL << sq);
    position->occupancy[piece_color] |= (1ULL << sq);
    position->all |= (1ULL << sq);
    position->mailbox[sq] = (PIECE_TYPES * piece_color) + piece;
}


//PUBLIC FUNCTIONS
//utility functions
void init_castling_rights_by_sq(void)
{
    for (int sq = A1; sq <= H8; sq++)
        switch (sq) {
        case E1: castling_rights_by_sq[sq] |= BLACK_KINGSIDE | BLACK_QUEENSIDE; break;
        case H1: castling_rights_by_sq[sq] |= WHITE_QUEENSIDE | BLACK_KINGSIDE | BLACK_QUEENSIDE; break;
        case A1: castling_rights_by_sq[sq] |= WHITE_KINGSIDE | BLACK_KINGSIDE | BLACK_QUEENSIDE; break;
        case E8: castling_rights_by_sq[sq] |= WHITE_KINGSIDE | WHITE_QUEENSIDE; break;
        case H8: castling_rights_by_sq[sq] |= BLACK_QUEENSIDE | WHITE_KINGSIDE | WHITE_QUEENSIDE; break;
        case A8: castling_rights_by_sq[sq] |= BLACK_KINGSIDE | WHITE_KINGSIDE | WHITE_QUEENSIDE; break;
        default: castling_rights_by_sq[sq] |= WHITE_KINGSIDE | WHITE_QUEENSIDE 
                                            | BLACK_KINGSIDE | BLACK_QUEENSIDE; break;
        }  
}
uint64_t sq_attackers(const struct position* position, int sq, int attacking_color)
{
    uint64_t sq_attackers_bitboard = 0;

    uint64_t potential_knight_attacking_squares = get_knight_attack(sq);
    uint64_t attacker_knights = position->pieces[attacking_color][KNIGHT];
    uint64_t attacking_knights = potential_knight_attacking_squares & attacker_knights;
    
    uint64_t potential_rook_attacking_squares = get_rook_attack(sq, position->all);
    uint64_t attacker_rooks = position->pieces[attacking_color][ROOK];
    uint64_t attacking_rooks = potential_rook_attacking_squares & attacker_rooks;

    uint64_t potential_bishop_attacking_squares = get_bishop_attack(sq, position-> all);
    uint64_t attacker_bishops = position->pieces[attacking_color][BISHOP];
    uint64_t attacking_bishops = potential_bishop_attacking_squares & attacker_bishops;

    uint64_t potential_queen_attacking_squares = potential_bishop_attacking_squares 
                                                | potential_rook_attacking_squares;
    uint64_t attacker_queens = position->pieces[attacking_color][QUEEN];
    uint64_t attacking_queens = potential_queen_attacking_squares & attacker_queens;

    int opposite_color = (attacking_color + 1) % 2;
    uint64_t potential_pawn_attacking_squares = get_pawn_attack(sq, opposite_color);
    uint64_t attacker_pawns = position->pieces[attacking_color][PAWN];
    uint64_t attacking_pawns = potential_pawn_attacking_squares & attacker_pawns;

    uint64_t potential_king_attacking_squares = get_king_attack(sq);
    uint64_t attacker_king = position->pieces[attacking_color][KING];
    uint64_t attacking_king = potential_king_attacking_squares & attacker_king;

    sq_attackers_bitboard = attacking_knights | attacking_bishops | attacking_rooks
                | attacking_queens | attacking_pawns | attacking_king;

    return sq_attackers_bitboard;
}
int is_king_in_check(const struct position* position, int color)
{
    int opposite_color = (color + 1) % 2;
    int king_sq = __builtin_ctzll(position->pieces[color][KING]);
    return sq_attackers(position, king_sq, opposite_color) != 0;
}

//position initializer and manipulators
struct position FEN_to_position(const char* fen)
{
    struct position position = {0};

    fen = set_boards(&position, fen);
    fen = set_side_to_play(&position, fen);
    fen = set_castling_rights(&position, fen);
    fen = set_en_passant(&position, fen);
    fen = set_halfmove_clock(&position, fen);
    set_fullmove_clock(&position, fen);

    return position;
}
void make(struct position* position, MOVE move, struct prev_pos_info* prev_pos_info)
{
    //parsing move
    int start_sq = (move >> 0) & ((1 << 6) - 1);
    int target_sq = (move >> 6) & ((1 << 6) - 1);
    int move_type = (move >> 12) & ((1 << 2) - 1);
    int promotion_piece = ((move >> 14) & ((1 << 2) - 1)) + KNIGHT;
    
    //useful variables + saving lost metadata (for unmake)
    int my_color = position->side_to_play;
    int opposite_color = (my_color + 1) % 2;
    int pawn_forward_step = 8 * (1 + (my_color * -2)); //relevant for en passant maintaining
    int piece_to_move = position->mailbox[start_sq] - (PIECE_TYPES * my_color);
    int piece_to_capture = position->mailbox[target_sq]; //raw - without adjusting to PIECE_TYPES
    prev_pos_info->piece_last_captured = EMPTY;
    prev_pos_info->prev_castling_rights = position->castling_rights;
    prev_pos_info->prev_en_passant = position->en_passant;
    prev_pos_info->prev_halfmove_clock = position->halfmove_clock;

    //updating position
    //(1) updating 50-move-draw clocks
    if (piece_to_move != PAWN && piece_to_capture == EMPTY)
        position->halfmove_clock++;
    else
        position->halfmove_clock = 0;
    
    if (my_color == BLACK)
        position->fullmove_clock++;

    //(2) updating piece_to_move and piece_to_capture bitboards
    remove_piece_from_sq(position, piece_to_move, my_color, start_sq);
    
    if (move_type == MOVE_PROMOTION)
        piece_to_move = promotion_piece;
    
    if (piece_to_capture != EMPTY) { //if piece_to_capture exists handling
        piece_to_capture -= (PIECE_TYPES * opposite_color);
        prev_pos_info->piece_last_captured = piece_to_capture; //saving lost info
        remove_piece_from_sq(position, piece_to_capture, opposite_color, target_sq);
    }
    put_piece_on_sq(position, piece_to_move, my_color, target_sq);

    if (move_type == MOVE_CASTLING) { //update rook bitboards
        int rook_start_sq = 0;
        int rook_target_sq = 0;

        switch (target_sq) {
        case G1: rook_start_sq = H1; rook_target_sq = F1; break;
        case C1: rook_start_sq = A1; rook_target_sq = D1; break;
        case G8: rook_start_sq = H8; rook_target_sq = F8; break;
        case C8: rook_start_sq = A8; rook_target_sq = D8; break;
        default: break;
        }

        remove_piece_from_sq(position, ROOK, my_color, rook_start_sq);
        put_piece_on_sq(position, ROOK, my_color, rook_target_sq);
    }
    if (move_type == MOVE_EN_PASSANT) { //update the en-passant-captured-pawn bitboards
        int en_passant_sq = position->en_passant;
        int pawn_to_capture_sq = en_passant_sq - pawn_forward_step;

        remove_piece_from_sq(position, PAWN, opposite_color, pawn_to_capture_sq);
    }

    //(3) updating other position metadata
    //updating side_to_play
    position->side_to_play = opposite_color;
    
    //updating en_passant
    if (piece_to_move == PAWN && ((target_sq - start_sq) == 2 * pawn_forward_step))
        position->en_passant = target_sq - pawn_forward_step;
    else
        position->en_passant = -1;

    //updating castling_rights
    position->castling_rights &= castling_rights_by_sq[start_sq] & castling_rights_by_sq[target_sq];
}
void unmake(struct position* position, MOVE last_move, const struct prev_pos_info* prev_pos_info)
{
    //parsing move
    int origin_sq = (last_move >> 0) & ((1 << 6) - 1);
    int curr_sq = (last_move >> 6) & ((1 << 6) - 1);
    int move_type = (last_move >> 12) & ((1 << 2) - 1);

    //useful variables
    int curr_color = position->side_to_play;
    int last_color = (curr_color + 1) % 2;
    int pawn_forward_step = 8 * (1 + (last_color * -2)); //relevant for en passant maintaining
    int piece_last_moved = position->mailbox[curr_sq] - (PIECE_TYPES * last_color);
    //prev position lost info
    int piece_last_captured = prev_pos_info->piece_last_captured; //raw - without adjusting to PIECE_TYPES
    int prev_castling_rights = prev_pos_info->prev_castling_rights;
    int prev_en_passant_sq = prev_pos_info->prev_en_passant;
    int prev_halfmove_clock = prev_pos_info->prev_halfmove_clock;

    //updating position
    //(1) reverting 50-move-draw clocks
    position->halfmove_clock = prev_halfmove_clock;
    if (last_color == BLACK)
        position->fullmove_clock--;

    //(2) reverting to previous position bitboards
    remove_piece_from_sq(position, piece_last_moved, last_color, curr_sq);
    if (piece_last_captured != EMPTY)
        put_piece_on_sq(position, piece_last_captured, curr_color, curr_sq);
    
    if (move_type == MOVE_EN_PASSANT) {
        int original_pawn_captured_en_passant_sq = curr_sq - pawn_forward_step;
        put_piece_on_sq(position, PAWN, curr_color, original_pawn_captured_en_passant_sq);
    }
    
    if (move_type == MOVE_PROMOTION)
        piece_last_moved = PAWN;
    put_piece_on_sq(position, piece_last_moved, last_color, origin_sq);

    if (move_type == MOVE_CASTLING) { //revert rook bitboards
        int rook_origin_sq = 0;
        int rook_curr_sq = 0;

        switch (curr_sq) {
        case G1: rook_origin_sq = H1; rook_curr_sq = F1; break;
        case C1: rook_origin_sq = A1; rook_curr_sq = D1; break;
        case G8: rook_origin_sq = H8; rook_curr_sq = F8; break;
        case C8: rook_origin_sq = A8; rook_curr_sq = D8; break;
        default: break;
        }

        remove_piece_from_sq(position, ROOK, last_color, rook_curr_sq);
        put_piece_on_sq(position, ROOK, last_color, rook_origin_sq);
    }

    //(3) reverting other position metadata
    //reverting side_to_play
    position->side_to_play = last_color;
    
    //updating en_passant
    position->en_passant = prev_en_passant_sq;

    //updating castling_rights
    position->castling_rights = prev_castling_rights;
}

//board printing functions
void print_bitboard(uint64_t bitboard)
{
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file <= 7; file++) {
            int pos = (8 * rank) + file;
    
            char to_print = '.';

            if (bitboard & (1ULL << pos))
                to_print = '1';
            
            printf("%c", to_print);
        }
        printf("\n");
    }
}
void print_position_metadata(const struct position* position)
{
    printf("side to play - %d \n", position->side_to_play);
    printf("castling rights - %d \n", position->castling_rights);
    printf("en passant - %d \n",position->en_passant);
    printf("halfmove clock - %d \n", position->halfmove_clock);
    printf("fullmove clock - %d \n", position->fullmove_clock);
}
void print_position(const struct position* position)
{
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file <= 7; file++) {
            int pos = (8 * rank) + file;
            int piece_code = position->mailbox[pos];
    
            char to_print = '.';

            if (piece_code != EMPTY)
                to_print = piece_dict[piece_code];
            
            printf("%c", to_print);
        }
        printf("\n");
    }
}
