#include "common.h"

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
    // turn by permutation and orientation tables
    uint16_t p = rank_perm(&state);
    uint16_t o = rank_orie(&state);
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