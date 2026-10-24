#include <stdio.h>

#include "types.h"
#include "board.h"
#include "attacks.h"
#include "movegen.h"

//CONSTANTS-DEFINITIONS-DATA STRUCTURES
enum max_depths_to_check {MAX_DEPTHS_TO_CHECK = 7};

struct pos_perft {
    const char *pos_fen;
    int depths_to_test;
    unsigned long long expected_number_per_depth[MAX_DEPTHS_TO_CHECK];
};

static const struct pos_perft positions_to_test[] = {
    //starting position
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7,
        {20, 400, 8902, 197281, 4865609, 119060324, 3195901860}
    },
    //Kiwipete position
    {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 6,
        {48, 2039, 97862, 4085603, 193690690, 8031647685}
    },
    //position 3
    {
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 7,
        {14, 191, 2812, 43238, 674624, 11030083, 178633661}
    },
    //position 4
    {
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6,
        {6, 264, 9467, 422333, 15833292, 706045033}
    },
    //position 4 mirrored
    {
        "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 6,
        {6, 264, 9467, 422333, 15833292, 706045033}
    },
    //position 5
    {
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 
        {44, 1486, 62379, 2103487, 89941194}
    },
    //position 6
    {
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 6,
        {46, 2079, 89890, 3894594, 164075551, 6923051137}
    }
};


//FUNCTIONS
static unsigned long long perft(struct position *position, int n)
{
    if (n == 0)
        return 1;

    unsigned long long acc = 0;

    struct moves curr_lvl_moves = {0};
    generate_moves(position, &curr_lvl_moves);
    struct prev_pos_info curr_pos_prev_info = {0};

    //reach here if n > 0
    for (int i = 0; i < curr_lvl_moves.new_move_index; i++) {
        make(position, curr_lvl_moves.move_list[i], &curr_pos_prev_info);
            
        int last_color_to_play = 1 - position->side_to_play;
            
        if (!is_king_in_check(position, last_color_to_play))
            acc += perft(position, n - 1);
        
        unmake(position, curr_lvl_moves.move_list[i], &curr_pos_prev_info);
    }

    return acc;
}


//MAIN
int main(void) 
{
    attacks_init();
    init_castling_rights_by_sq();
    
    for (int i = 0; i < 7; i++) {
        struct position position_i = FEN_to_position(positions_to_test[i].pos_fen);
        int depths_to_test = positions_to_test[i].depths_to_test;
        int flag_pos_test_failed = 0;

        for (int d = 1; d <= depths_to_test; d++) {
            if (perft(&position_i, d) != positions_to_test[i].expected_number_per_depth[d - 1]) {
                printf("POSITION %d, AT DEPTH %d, TEST FAILED\n", i, d);
                flag_pos_test_failed = 1;
            }
        }
        if (!flag_pos_test_failed)
            printf("POSITION %d, ALL DEPTHS, TEST SUCCEEDED\n\n", i);
    }
    
    return 0;
}