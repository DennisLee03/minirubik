#ifndef COMMON_H
#define COMMON_H

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

// sum in [0, 14]: sum%3 == sum_mod3[sum]
static const uint8_t sum_mod3[15] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};

// i in [0, 13]: i%7 == i_mod7[i]
static const uint8_t i_mod7[14] = {0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6};

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

/* stdout is fully buffered off a terminal, so a write error surfaces at the
 * flush, not at the printf that queued the bytes. Every exit path that has
 * produced output goes through here.
 */
static int output_failed(void)
{
    return fflush(stdout) != 0 || ferror(stdout);
}

#endif