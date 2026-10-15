#include "../common.h"

int main(int argc, char *argv[]) {

  // test sum%3 == sum_mod3[sum]
  for(uint8_t sum=0; sum<=14; sum++) {
    if(sum%3 != sum_mod3[sum]) {
      printf("Fail %u mod 3. Table result: %u\n", sum, sum_mod3[sum]);
      return 1;
    }
  }
  printf("All sum_mod[] passed!\n");

  // test i%7 == i_mod7[i]
  for(uint8_t i=0; i<14; i++) {
    if(i%7 != i_mod7[i]) {
      printf("Fail %u mod 7. Table result: %u\n", i, i_mod7[i]);
      return 1;
    }
  }
  printf("All i_mod[] passed!\n");

  return 0;
}