#include <stdint.h>
#include <string.h>

#include "attacks.h"

//CONSTANTS-DEFINITIONS-DATA STRUCTURES
static const uint64_t a_file = 0x0101010101010101ULL;
static const uint64_t h_file = 0x8080808080808080ULL;
static const uint64_t a_b_files = 0x0303030303030303ULL; 
static const uint64_t g_h_files = 0xc0c0c0c0c0c0c0c0ULL;
static const uint64_t rank_1 = 0x00000000000000FFULL;
static const uint64_t rank_8 = 0xFF00000000000000ULL;
static const uint64_t a_file_rank_1 = 0x01010101010101FFULL;
static const uint64_t a_file_rank_8 = 0xFF01010101010101ULL;
static const uint64_t h_file_rank_1 = 0x80808080808080FFULL;
static const uint64_t h_file_rank_8 = 0xFF80808080808080ULL;

typedef uint64_t (*SINGLE_ATTACK_FUNC)(int sq, uint64_t blockers_config);
enum Max_Bits_In_Mask {MAX_BITS_IN_MASK = 12};
enum Squares_x_Blocker_Configs {SQUARES_x_BLOCKER_CONFIGS_ROOK = 102400, 
                                SQUARES_x_BLOCKER_CONFIGS_BISHOP = 5248};

static uint64_t rand_state = 123456789ULL;

static uint64_t knight_attacks_arr[SQUARES] = {0};
static uint64_t king_attacks_arr[SQUARES] = {0};
static uint64_t white_pawn_attacks_arr[SQUARES] = {0};
static uint64_t black_pawn_attacks_arr[SQUARES] = {0};

//rook data structures
static uint64_t rook_attacks_arr[SQUARES_x_BLOCKER_CONFIGS_ROOK] = {0};
static uint64_t rook_masks[SQUARES] = {0};
static int rook_attacks_block_beginnings[SQUARES] = {0};
static uint64_t rook_magic_numbers[SQUARES] = {0};

//bishop data structures
static uint64_t bishop_attacks_arr[SQUARES_x_BLOCKER_CONFIGS_BISHOP] = {0};
static uint64_t bishop_masks[SQUARES] = {0};
static int bishop_attacks_block_beginnings[SQUARES] = {0};
static uint64_t bishop_magic_numbers[SQUARES] = {0};


//PRIVATE FUNCTIONS
//utility functions
static uint64_t xorshift64(void)
{
    rand_state ^= (rand_state << 13);
    rand_state ^= (rand_state >> 7);
    rand_state ^= (rand_state << 17);
    return rand_state;
}
static uint64_t magic_candidate(void)
{
    uint64_t rand_num1 = xorshift64();
    uint64_t rand_num2 = xorshift64();
    uint64_t rand_num3 = xorshift64();
    uint64_t magic_cand = rand_num1 & rand_num2 & rand_num3; 
    return magic_cand;
}
static void mask_powerset_gen(uint64_t mask, uint64_t powerset[])
{
    int on_bits[MAX_BITS_IN_MASK] = {0};
    int j = 0;
    for (int i = 0; i < 64; i++) {
        if ((1ULL << i) & mask) {
            on_bits[j] = i;
            j++;
        }
    }

    int mask_size = j;
    int powerset_size = (1ULL << mask_size);

    for (int counter = 0; counter < powerset_size; counter++) {
        uint64_t mask_subset = 0;
        for (int i = 0; i < mask_size; i++)
            if (counter & (1U << i)) 
                mask_subset |= (1ULL << on_bits[i]);
        
        powerset[counter] = mask_subset;
    }
}
static void sliding_attacks_sq_block_filler(int sq, int mask_bits, uint64_t attacks[], 
                                            uint64_t blocker_configs[], 
                                            SINGLE_ATTACK_FUNC single_attack_gen,
                                            uint64_t piece_magic_numbers[])
{
    int block_size = 1ULL << mask_bits; //block_size = 2^mask_bits

    while (1) {
        uint64_t magic_cand = magic_candidate();
        int collision_found = 0;
        
        for (int i = 0; i < block_size; i++) {
            uint64_t ith_config_attack = single_attack_gen(sq, blocker_configs[i]);
            
            //hashing blocker_config to compressed index
            int index = (magic_cand * blocker_configs[i]) >> (64 - mask_bits);

            //collision handling
            if (attacks[index] != 0 && attacks[index] != ith_config_attack) {
                memset(attacks, 0, block_size * sizeof(uint64_t));
                collision_found = 1;
                break; //num is'nt magic -> test a new num
            }
            
            attacks[index] = ith_config_attack;
        }
        if (!collision_found) {
            piece_magic_numbers[sq] = magic_cand;
            break;
        }
    }
}

//rook helper functions
static uint64_t rook_mask_from_sq(int sq)
{
    uint64_t rook_mask = 0;
    
    uint64_t edge_by_direction[4] = {h_file, a_file, rank_8, rank_1};
    int step_by_direction[4] = {1, -1, 8, -8}; //right, left, up, down

    for (int direction = 0; direction < 4; direction++) {
        int walker = sq;
        int step = step_by_direction[direction];
                
        while (!((1ULL << walker) & edge_by_direction[direction])) {
            rook_mask |= (1ULL << walker);
            walker += step;
        }
    }
    rook_mask &= ~(1ULL << sq);
    return rook_mask;
}
static void rook_masks_gen(void)
{
    for (int sq = A1; sq <= H8; sq++) 
        rook_masks[sq] = rook_mask_from_sq(sq);
}
static uint64_t single_rook_attack_gen(int start_sq, uint64_t blockers_config)
{
    uint64_t res_bitboard = 0;

    uint64_t edge_by_direction[4] = {h_file, a_file, rank_8, rank_1};
    int step_by_direction[4] = {1, -1, 8, -8}; //right, left, up, down
    
    for (int direction = 0; direction < 4; direction++) {
        int walker = start_sq;
        int step = step_by_direction[direction];
                
        while (!((1ULL << walker) & edge_by_direction[direction]) && !((1ULL << walker) & blockers_config)) {
            walker += step;
            res_bitboard |= (1ULL << walker);
        }
    }
    return res_bitboard;
}

//bishop helper functions
static uint64_t bishop_mask_from_sq(int sq)
{
    uint64_t bishop_mask = 0;

    uint64_t edge_by_direction[4] = {a_file_rank_8, h_file_rank_8, a_file_rank_1, h_file_rank_1};
    int step_by_direction[4] = {7, 9, -9, -7}; //left-up, right-up, left-down, right-down

    for (int direction = 0; direction < 4; direction++) {
        int walker = sq;
        int step = step_by_direction[direction];
        while (!((1ULL << walker) & edge_by_direction[direction])) {
            bishop_mask |= (1ULL << walker);
            walker += step;
        }
    }
    bishop_mask &= ~(1ULL << sq);
    return bishop_mask;
}
static void bishop_masks_gen(void)
{
    for (int sq = A1; sq <= H8; sq++) 
        bishop_masks[sq] = bishop_mask_from_sq(sq);
}
static uint64_t single_bishop_attack_gen(int start_sq, uint64_t blockers_config)
{
    uint64_t res_bitboard = 0;

    uint64_t edge_by_direction[4] = {a_file_rank_8, h_file_rank_8, a_file_rank_1, h_file_rank_1};
    int step_by_direction[4] = {7, 9, -9, -7}; //left-up, right-up, left-down, right-down

    for (int direction = 0; direction < 4; direction++) {
        int walker = start_sq;
        int step = step_by_direction[direction];

        while (!((1ULL << walker) & edge_by_direction[direction]) && !((1ULL << walker) & blockers_config)) {
            walker += step;
            res_bitboard |= (1ULL << walker);
        }
    }
    return res_bitboard;
}

//attack table initializers
static void knight_attacks_gen(void)
{
    //a_file to fix: <<17, >>15
    //h_file to fix: <<15, >>17
    //a_b_files to fix: <<10, >>6
    //g_h_files to fix: <<6, >>10

    for (int i = A1; i <= H8; i++) {
        uint64_t start = 1ULL << i;

        knight_attacks_arr[i] = ((start << 17) & ~a_file) | ((start >> 15) & ~a_file)
                        | ((start << 15) & ~h_file) | ((start >> 17) & ~h_file)
                        | ((start << 10) & ~a_b_files) | ((start >> 6) & ~a_b_files)
                        | ((start << 6) & ~g_h_files) | ((start >> 10) & ~g_h_files);
    }
}
static void king_attacks_gen(void)
{
    //a_file to fix: <<9, <<1, >>7
    //h_file to fix: <<7, >>1, >>9

    for (int i = A1; i <= H8; i++) {
        uint64_t start = 1ULL << i;

        king_attacks_arr[i] = ((start << 9) & ~a_file) | ((start << 1) & ~a_file) | ((start >> 7) & ~a_file) 
                      | ((start << 7) & ~h_file) | ((start >> 1) & ~h_file) | ((start >> 9) & ~h_file)
                      | (start << 8) | (start >> 8);
    }
}
static void white_pawn_attacks_gen(void)
{
    //a_file to fix: <<9
    //h_file to fix: <<7

    for (int i = A1; i <= H8; i++) {
        uint64_t start = 1ULL << i;

        white_pawn_attacks_arr[i] = ((start << 9) & ~a_file) | ((start << 7) & ~h_file);
    }
}
static void black_pawn_attacks_gen(void)
{
    //a_file to fix: >>7
    //h_file to fix: >>9

    for (int i = H8; i >= A1; i--) {
        uint64_t start = 1ULL << i;

        black_pawn_attacks_arr[i] = ((start >> 7) & ~a_file) | ((start >> 9) & ~h_file);
    }
}
static void rook_attacks_gen(void)
{
    rook_masks_gen();

    int offset = 0;
    for (int i = A1; i <= H8; i++) {
        uint64_t ith_sq_blocker_configs[1ULL << MAX_BITS_IN_MASK] = {0};
        mask_powerset_gen(rook_masks[i], ith_sq_blocker_configs);

        int mask_bits = __builtin_popcountll(rook_masks[i]);
        rook_attacks_block_beginnings[i] = offset;
        
        sliding_attacks_sq_block_filler(i, mask_bits, rook_attacks_arr + offset, ith_sq_blocker_configs, 
                                        single_rook_attack_gen, rook_magic_numbers);
        
        offset += (1ULL << mask_bits);
    }
}
static void bishop_attacks_gen(void)
{
    bishop_masks_gen();

    int offset = 0;
    for (int i = A1; i <= H8; i++) {
        uint64_t ith_sq_blocker_configs[1ULL << MAX_BITS_IN_MASK] = {0};
        mask_powerset_gen(bishop_masks[i], ith_sq_blocker_configs);

        int mask_bits = __builtin_popcountll(bishop_masks[i]);
        bishop_attacks_block_beginnings[i] = offset;
        
        sliding_attacks_sq_block_filler(i, mask_bits, bishop_attacks_arr + offset, ith_sq_blocker_configs,
                                        single_bishop_attack_gen, bishop_magic_numbers);
        
        offset += (1ULL << mask_bits);
    }
}


//PUBLIC FUNCTIONS
void attacks_init(void)
{
    knight_attacks_gen();
    king_attacks_gen();
    white_pawn_attacks_gen();
    black_pawn_attacks_gen();
    rook_attacks_gen();
    bishop_attacks_gen();
}
uint64_t get_knight_attack(int square)
{
    return knight_attacks_arr[square];
}
uint64_t get_king_attack(int square)
{
    return king_attacks_arr[square];
}
uint64_t get_pawn_attack(int square, int color)
{
    uint64_t res = 0;
    switch (color) {
        case WHITE: res = white_pawn_attacks_arr[square]; break;
        case BLACK: res = black_pawn_attacks_arr[square]; break;
        default: break;
    }
    return res;
}
uint64_t get_rook_attack(int square, uint64_t occupancy_bitboard)
{
    uint64_t sq_magic_num = rook_magic_numbers[square];
    uint64_t rook_sq_mask = rook_masks[square];
    uint64_t blocker_config = rook_sq_mask & occupancy_bitboard;

    int index_in_sq_block = (sq_magic_num * blocker_config) >> (64 - __builtin_popcountll(rook_sq_mask));

    return rook_attacks_arr[rook_attacks_block_beginnings[square] + index_in_sq_block];
}
uint64_t get_bishop_attack(int square, uint64_t occupancy_bitboard)
{
    uint64_t sq_magic_num = bishop_magic_numbers[square];
    uint64_t bishop_sq_mask = bishop_masks[square];
    uint64_t blocker_config = bishop_sq_mask & occupancy_bitboard;
    
    int index_in_sq_block = (sq_magic_num * blocker_config) >> (64 - __builtin_popcountll(bishop_sq_mask));

    return bishop_attacks_arr[bishop_attacks_block_beginnings[square] + index_in_sq_block];
}
uint64_t get_queen_attack(int square, uint64_t occupancy_bitboard)
{
    uint64_t diagonals_attack = get_bishop_attack(square, occupancy_bitboard);
    uint64_t lines_attack = get_rook_attack(square, occupancy_bitboard);
    return diagonals_attack | lines_attack;
}
