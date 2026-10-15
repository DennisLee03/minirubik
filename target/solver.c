#include "tables.h"
#include "common.h"

uint8_t path[11];          // recorded path to solved
#define h(p, o) (hp[p]>ho[o]?hp[p]:ho[o])

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

    return 255;// error.
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