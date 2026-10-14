#include "tables.h"
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

uint8_t path[11];          // recorded path to solved
#define h(p, o) (hp[p]>ho[o]?hp[p]:ho[o])

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

static uint8_t solve_iteration(uint16_t p0, uint16_t o0) {
  
    // path
    memset(path, 0xFF, 11);

    uint16_t p[12], o[12];
    uint16_t next_p[12], next_o[12];
    uint8_t face[12], turn[12];

    // already solved
    if(p0 == 0 && o0 == 0) return 0;

    for(uint8_t bound = h(p0, o0); bound <= 11; bound++) {

        // dfs root
        uint8_t g = 0;
        p[0] = p0; o[0] = o0;
        face[0] = 0; turn[0] = 0;
        next_p[0] = p0; next_o[0] = o0;

        while(1) {
            uint8_t descended = 0;

            // explore nodes at level g:
            for(; face[g] < 3; face[g]++, turn[g] = 0, next_p[g] = p[g], next_o[g] = o[g]) {
                
                // if(last_face == face): skip this face
                if(g > 0 && face[g] == face[g-1]) continue;
                
                for(; turn[g] < 3; ) {
                    next_p[g] = permutation[face[g]][next_p[g]];
                    next_o[g] = orientation[face[g]][next_o[g]];
                    path[g] = face[g]*3 + turn[g];
                    turn[g]++;

                    // solved
                    if(next_p[g] == 0 && next_o[g] == 0) return g+1;

                    // prune
                    if((g+1) + h(next_p[g], next_o[g]) > bound) continue;
                    
                    p[g+1] = next_p[g]; o[g+1] = next_o[g];
                    g++;
                    face[g] = 0; turn[g] = 0;
                    next_p[g] = p[g]; next_o[g] = o[g];
                    descended = 1;
                    break;
                }
                if(descended) break;
            }
            if(descended) continue;

            if(g == 0) break;

            g--;
        }
    }

    return 255;// error. 0 moves with a unsolved state fails at apply-test
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

int main(int argc, char **argv)
{
    state_t state;

    // $ when not ./solver PPPPPPPOOOOOOO
    if (argc != 2 || !parse_state(argv[1], &state)) {
        /* C99 5.1.2.2.1 lets argv[0] be null when argc is 0. */
        fprintf(stderr, "usage: %s PPPPPPPOOOOOOO\n",
                argc > 0 && argv[0] ? argv[0] : "solver");
        return 2;
    }

    // $ ./solver PPPPPPPOOOOOOO by IDA*
    uint32_t unsolved = rank_state(&state);
    uint16_t p = (uint16_t)(unsolved/729);
    uint16_t o = (uint16_t)(unsolved%729);
    uint8_t cost = solve_iteration(p, o);
    if(cost == 255) {
      printf("FAIL TO SOLVE\n");
      return 1;
    }

    printf("distance = %d\n", cost);
    for(uint8_t step=0; step<cost; step++) {
        uint8_t move = path[step];
        if(move != 0xFF) {
            printf("%s ", move_names[move]);
        }
    }
    printf("\n");

    return output_failed();
}