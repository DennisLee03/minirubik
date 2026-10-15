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
    uint16_t next_p = p, next_o = o;
    for(uint8_t i = 0; i < cost; i++) {
      uint8_t move = path[i];
      uint8_t face = move_face[move], turns = move_turns[move];
      for(uint8_t turn = 0; turn < turns; turn++) {
        next_p = permutation[face][next_p];
        next_o = orientation[face][next_o];
      }
    }

    // test (p, o) == (0, 0)
    if((next_p != 0) || (next_o != 0)) {
      printf("After solve_iteration and transition errors: (%hu, %hu)\n", next_p, next_o);
      return 1;
    }
  }

  printf("All transition passed\n");
}