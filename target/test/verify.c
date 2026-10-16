#include "../common.h"

int main(int argc, char *argv[]) {
  
  uint32_t states = PERMUTATIONS*ORIENTATIONS;

  for(uint32_t state=0; state<states; state++) {
    uint16_t p = (uint16_t)(state / 729);
    uint16_t o = (uint16_t)(state % 729);

    uint8_t cost = solve_iteration(p, o);

    // solve errors!
    if(cost == 255) {
      printf("solve_iteration(%hu, %hu) error\n", p, o);
      return 1;
    }

    // apply face turns by transition table
    uint8_t valid = verify_path(p, o, path, cost);
    if(!valid) {
      printf("solve_iteration(%hu, %hu) errors.\n", p, o);
      return 1;
    }
  }

  printf("All transition passed\n");
}