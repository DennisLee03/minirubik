#include "common.h"

#ifndef __riscv
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
#else

// inlined testcase
static const char input[] = "21345671111111";
static const uint8_t expected = 11;

// in freestanding C program, main is not spceical but I use ripes_main to prevent gcc do something
int ripes_main(void)
{
    state_t state;

    // 1. input
    if (!parse_state(input, &state)) return 1;

    // 2. rank
    uint16_t p = rank_perm(&state);
    uint16_t o = rank_orie(&state);

    // 3. search
    uint8_t len = solve_iteration(p, o);
    if (len == 255) return 2;

    // 4. verify (p, o) to (0, 0)
    if (!verify_path(p, o, path, len)) return 3;
    if (len != expected) return 4;

    return 0;
}

__asm__(
    ".globl _start\n"
    "_start:\n"
    "    .option push\n"               // save original option
    "    .option norelax\n"            // change to norelax
    "    la   gp, __global_pointer$\n" 
    "    .option pop\n"                // restore original option
    "    li   sp, 0x7ffffff0\n"
    "    call ripes_main\n"
    "    li   a7, 93\n"
    "    ecall\n"
    "1:  j    1b\n"
);


#endif