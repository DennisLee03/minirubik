#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    CUBIES = 7,
    PERMUTATIONS = 5040,
    ORIENTATIONS = 729,
    STATES = PERMUTATIONS * ORIENTATIONS,
    MOVES = 9
};

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;


static const char *const move_names[MOVES] = {"R",  "R2", "R'", "B", "B2",
                                              "B'", "D",  "D2", "D'"};
static const uint8_t inverse_move[MOVES] = {2, 1, 0, 5, 4, 3, 8, 7, 6};
/* Each destination takes a cubie from source[face][destination]. */
static const uint8_t source[3][CUBIES] = {
    {1, 4, 2, 0, 3, 5, 6}, // R
    {0, 1, 2, 4, 5, 6, 3}, // B
    {0, 2, 5, 3, 1, 4, 6}, // D
};
static const uint8_t twist[3][CUBIES] = {
    {1, 2, 0, 2, 1, 0, 0}, // R
    {0, 0, 0, 1, 2, 1, 2}, // B
    {0, 0, 0, 0, 0, 0, 0}, // D
};
//static uint8_t hp[PERMUTATIONS];
//static uint8_t ho[ORIENTATIONS];

static state_t quarter_turn(state_t state, uint8_t face)
{
    state_t result;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t from = source[face][i];
        result.p[i] = state.p[from];
        result.o[i] = (uint8_t) ((state.o[from] + twist[face][i]) % 3U);
    }
    return result;
}

static state_t apply_move(state_t state, uint8_t move)
{
    uint8_t turns = (uint8_t) (move % 3U + 1U);
    for (uint8_t i = 0; i < turns; ++i)
        state = quarter_turn(state, (uint8_t) (move / 3U));
    return state;
}

static uint32_t rank_state(const state_t *state)
{
    uint32_t p = 0, o = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t smaller = 0;
        for (uint8_t j = (uint8_t) (i + 1U); j < CUBIES; ++j)
            if (state->p[j] < state->p[i])
                ++smaller;
        p = p * (CUBIES - i) + smaller;
    }
    for (uint8_t i = 0; i < 6; ++i)
        o = o * 3U + state->o[i];
    return p * ORIENTATIONS + o;
}

static void unrank_state(uint32_t rank, state_t *state)
{
    uint8_t available[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    uint32_t p = rank / ORIENTATIONS, o = rank % ORIENTATIONS, f = 720;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t q = (uint8_t) (p / f);
        p %= f;
        state->p[i] = available[q];
        for (uint8_t j = q; j + 1U < CUBIES - i; ++j)
            available[j] = available[j + 1U];
        if (i < 5)
            f /= 6U - i;
    }
    for (uint8_t i = 6; i-- > 0;) {
        state->o[i] = (uint8_t) (o % 3U);
        sum = (uint8_t) (sum + state->o[i]);
        o /= 3U;
    }
    state->o[6] = (uint8_t) ((3U - sum % 3U) % 3U);
}

static int valid(const state_t *state)
{
    uint8_t sum = 0;

    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (state->p[i] >= CUBIES || state->o[i] >= 3)
            return 0;
        for (uint8_t j = 0; j < i; ++j)
            if (state->p[j] == state->p[i])
                return 0;
        sum = (uint8_t) (sum + state->o[i]);
    }
    return sum % 3U == 0;
}

static uint8_t *build_table(uint8_t *diameter)
{
    /**
     * toward_solved stores 3,674,160(~367w) move numbers
     * toward_solved[state] = a move back to solved state along shortest path
     * Due to move numbers in range of [0, 8], we store each in byte
     */
    uint8_t *toward_solved = malloc(STATES);
    
    // BFS's queue, stores compact number of states
    uint32_t *queue = malloc((size_t) STATES * sizeof *queue);

    /**
     * look-up tables: 0~5039 and 0~728
     * next_permutation_after_turn = permutation[R_B_D][current_permutation]
     * next_orientation_after_turn = orientation[R_B_D][current_orientation]
     * To get next state quickly, otherwise it takes a lot to rank and unrank
     */
    uint16_t permutation[3][PERMUTATIONS], orientation[3][ORIENTATIONS];

    uint32_t head = 0, tail = 1, level_end = 1;
    state_t state;
    if (!toward_solved || !queue) {
        // if one of them allocating fails
        free(toward_solved);
        free(queue);
        return NULL;
    }

    // ========== building 2 LUTs ==========
    // Lehmer rank of each permutation
    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {

        // state = PPPPPPP 1111111 from unrank_state()
        unrank_state((uint32_t) rank * ORIENTATIONS, &state);

        // R(0), B(1), D(2) faces
        for (uint8_t face = 0; face < 3; ++face) {
            // apply all atomic turns, others turns: X', X2 can be a sequence of Xs
            state_t next = quarter_turn(state, face);
            permutation[face][rank] =
                (uint16_t) (rank_state(&next) / ORIENTATIONS);
        }
    }
    // Base-3 rank of each orientation
    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {
        unrank_state(rank, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            orientation[face][rank] =
                (uint16_t) (rank_state(&next) % ORIENTATIONS);
        }
    }
    // This 2 LUTs building takes (5040 + 729) × 3 ≈ 17,000 iterations(of unrank, turn, rank).


    // init heuristic tables
    //memset(hp, UINT8_MAX, PERMUTATIONS);
    //memset(ho, UINT8_MAX, ORIENTATIONS);

    memset(toward_solved, UINT8_MAX, STATES); // UINT8_MAX means not settled
    queue[0] = 0;
    toward_solved[0] = 0;
    *diameter = 0;
    while (head < tail) {
        if (head == level_end) {
            level_end = tail;
            ++*diameter;
        }
        uint32_t current_state = queue[head++]; // pop
        uint16_t p = (uint16_t) (current_state / ORIENTATIONS);
        uint16_t o = (uint16_t) (current_state % ORIENTATIONS);

        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t next_p = p, next_o = o;

            // hot loop: table lookups only, no rank_state()/unrank_state()
            for (uint8_t turn = 0; turn < 3; ++turn) {

                // apply R_B_D on current state to get next state
                next_p = permutation[face][next_p]; // 33,067,440 transitions, ~15 instructions per transition
                next_o = orientation[face][next_o]; // 33,067,440 transitions, ~15 instructions per transition
                // 33,067,440 x 2 x ~15 = ~992023200, 10^9 order
                // this is an estimation, using Ripes to run such instrutions takes too long

                uint32_t next_state = (uint32_t) next_p * ORIENTATIONS + next_o;
                
                // if this state back to previous state is not specified
                // the first visit to a state is along a shortest path
                if (toward_solved[next_state] == UINT8_MAX) {
                    uint8_t move = (uint8_t) (face * 3U + turn);

                    // assign the inverse move and distance
                    toward_solved[next_state] = (*diameter+1)<<4 | inverse_move[move];

                    
                    // push new state to next level
                    queue[tail++] = next_state;
                }
            }
        }
    }
    free(queue);
    if (tail != STATES) {
        // tail = #enqueue = #states
        free(toward_solved);
        return NULL;
    }
    return toward_solved;
}


static int parse_state(const char *input, state_t *state)
{
    for (int i = 0; i < 14; ++i) {
        int limit = i < 7 ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit)
            return 0;
        (i < 7 ? state->p : state->o)[i % 7] = (uint8_t) (input[i] - '1');
    }
    return input[14] == '\0' && valid(state);
}

/* stdout is fully buffered off a terminal, so a write error surfaces at the
 * flush, not at the printf that queued the bytes. Every exit path that has
 * produced output goes through here.
 */
static int output_failed(void)
{
    return fflush(stdout) != 0 || ferror(stdout);
}

static int self_test(void)
{
    // 0123456 0000000
    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};
    state_t state;

    // move inversion: verify `source` and `twist`
    for (uint8_t move = 0; move < MOVES; ++move) {
        state = solved;
        state = apply_move(state, move);
        state = apply_move(state, inverse_move[move]);
        if (memcmp(&solved, &state, sizeof solved))
            return 0;
    }

    // interate every compact number of states
    // bijection invariant: verify one-to-one mapping
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    state_t state;
    uint8_t diameter;

    // $ ./solver --self-test
    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        if (!self_test()) {
            fputs("self-test failed\n", stderr);
            return 1;
        }

        // graph exploration, verify BFS can reach every state
        uint8_t *table = build_table(&diameter);
        if (!table) {
            fputs("could not build complete state table\n", stderr);
            return 1;
        }
        free(table);
        if (diameter != 11) {
            fputs("BFS check failed\n", stderr);
            return 1;
        }
        puts("3674160 states; diameter 11");
        return output_failed();
    }

    // $ ./solver --distance-test
    if (argc == 2 && !strcmp(argv[1], "--distance-test")) {

        // build table and check
        uint8_t *table = build_table(&diameter);
        if (!table) {
            fputs("could not build complete state table\n", stderr);
            return 1;
        }

        uint32_t *distances = calloc(diameter, sizeof *distances);
        uint32_t states = PERMUTATIONS*ORIENTATIONS;

        for(uint32_t state = 1; state < states; state++) {
            distances[((table[state] & 0xF0) >> 4) - 1]++;
        }

        for(uint32_t i=0; i<diameter; i++){
            printf("%2d| %d\n", i+1, distances[i]);
        }
        
        free(table);

        return 0;
    }
    
    // $ when not ./solver PPPPPPPOOOOOOO
    if (argc != 2 || !parse_state(argv[1], &state)) {
        /* C99 5.1.2.2.1 lets argv[0] be null when argc is 0. */
        fprintf(stderr, "usage: %s PPPPPPPOOOOOOO\n",
                argc > 0 && argv[0] ? argv[0] : "solver");
        return 2;
    }
    uint8_t *table = build_table(&diameter);
    if (!table) {
        fputs("could not build complete state table\n", stderr);
        return 1;
    }
    const char *separator = "";
    uint8_t distance = (table[rank_state(&state)] & 0xF0) >> 4;
    printf("distance: %d\n", distance);
    for (uint32_t rank = rank_state(&state); rank; rank = rank_state(&state)) {
        uint8_t move = table[rank] & 0x0F;          // keep lower nibble which is a move
        printf("%s%s", separator, move_names[move]);
        separator = " ";
        state = apply_move(state, move);
    }
    putchar('\n');
    free(table);
    return output_failed();
}
