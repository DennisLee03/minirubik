#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common.h"

int main(int argc, char *argv[]) {
  
  uint32_t states = PERMUTATIONS*ORIENTATIONS;

  state_t s;
  for(uint32_t state=0; state<states; state++) {
    unrank_state(state, &s);
    uint16_t p = rank_perm(&s);
    uint16_t o = rank_orie(&s);
    if(p != (uint16_t)(state/729) || o != (uint16_t)(state%729)) {
      printf("Fail at state: %u\n", state);
      printf("\tstate div 729 = %hu\n", (uint16_t)(state/729));
      printf("\tstate mod 729 = %hu\n", (uint16_t)(state%729));
      return 1;
    }
  }
  printf("All ranking passed!\n");
  
  return 0;
}