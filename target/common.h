#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#ifndef __riscv
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "tables.h"

uint8_t path[11];          // recorded path to solved
#define h(p, o) (hp[p]>ho[o]?hp[p]:ho[o])

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

// sum in [0, 14]: sum%3 == sum_mod3[sum]
static const uint8_t sum_mod3[15] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};

// i in [0, 13]: i%7 == i_mod7[i]
static const uint8_t i_mod7[14] = {0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6};

// face and turns for moves
static const uint8_t move_face[9]  = {0,0,0, 1,1,1, 2,2,2};
static const uint8_t move_turns[9] = {1,2,3, 1,2,3, 1,2,3};

/**
 * \brief Rank `state` for Lehmer rank of its perm
 * \param state a representation of minirubik
 * \return permutation of `state`
 */
static uint16_t rank_perm(const state_t *state) {
    uint16_t p = 0;
    uint8_t smaller[7] = {0}; // 7 bytes
    for (uint8_t i = 0; i < CUBIES; ++i) {
        for (uint8_t j = (uint8_t) (i + 1U); j < CUBIES; ++j)
            if (state->p[j] < state->p[i])
                smaller[i] += 1;
        /**
         * p = p * (CUBIES - i) + smaller[i];
         * -----------------------------------
         * i=0, p*7: p = (p << 3) - p + smaller[0]
         * i=1, p*6: p = (p << 2) + (p << 1) + smaller[1]
         * i=2, p*5: p = (p << 2) + p + smaller[2]
         * i=3, p*4: p = (p << 2) + smaller[3]
         * i=4, p*3: p = (p << 2) - p + smaller[4]
         * i=5, p*2: p = (p << 1) + smaller[5]
         * i=6, p*1: p = p + smaller[6]
         */
    }

    // unroll:
    p = smaller[0];
    p = (p << 2) + (p << 1) + smaller[1];
    p = (p << 2) + p + smaller[2];
    p = (p << 2) + smaller[3];
    p = (p << 2) - p + smaller[4];
    p = (p << 1) + smaller[5];
    
    return p;
}

/**
 * \brief Rank `state` as a base-3 number
 * \param state a representation of minirubik
 * \return orientation of `state`
 */
static uint16_t rank_orie(const state_t *state) {
    uint16_t o = 0;
    for (uint8_t i = 0; i < 6; ++i)
        o = (o << 1) +  o /* x3 */ + state->o[i];
    return o;
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
    return sum_mod3[sum] == 0;
}

static int parse_state(const char *input, state_t *state)
{
    for (int i = 0; i < 14; ++i) {
        int limit = i < 7 ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit)
            return 0;
        (i < 7 ? state->p : state->o)[i_mod7[i]] = (uint8_t) (input[i] - '1');
    }
    return input[14] == '\0' && valid(state);
}

#ifndef __riscv
/* stdout is fully buffered off a terminal, so a write error surfaces at the
 * flush, not at the printf that queued the bytes. Every exit path that has
 * produced output goes through here.
 */
static int output_failed(void)
{
    return fflush(stdout) != 0 || ferror(stdout);
}
#endif

/**
 * \brief Verify a single path of a given state
 * \param p0 permutation
 * \param o0 orientation
 * \param pth from (`p`, `o`) to (0, 0)
 * \param len length of the path
 * \return 1 valid path, 0 invalid path
 */
static uint8_t verify_path(uint16_t p0, uint16_t o0, const uint8_t *pth, uint8_t len) {
    uint16_t next_p = p0, next_o = o0;
    for(uint8_t i = 0; i < len; i++) {
        uint8_t move = pth[i];
        uint8_t face = move_face[move], turns = move_turns[move];
        for(uint8_t turn = 0; turn < turns; turn++) {
            next_p = permutation[face][next_p];
            next_o = orientation[face][next_o];
        }
    }

    return ((next_p == 0) && (next_o == 0));
}

static uint8_t solve_iteration(uint16_t p0, uint16_t o0) {

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
                    path[g] = ((face[g]<<1) + face[g]) + turn[g];
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

    return 255;// error.
}

#endif