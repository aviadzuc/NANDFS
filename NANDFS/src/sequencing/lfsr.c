/** @file lfsr.c
 * Functions for random bit generation
 */
#include <system.h>
#include <src/sequencing/lfsr.h>
/*specific definitions*/
/* the series repeats itself every (2^32-1) / NEEDED_PROB
 * this means that the expectency is that we generate 1 every 2^NEEDED_PROB
 * */

uint32_t gen_rand_bit(uint32_t needed_prob) {
  uint32_t ret_val, i;
	
  ret_val = (((lfsr_state & ((1<<needed_prob)-1)) == 0)? 1:0);
   	
  for (i=needed_prob; i >0;i--) {    
    TACT_LFSR();
  }
 
  return ret_val;
}

uint32_t get_random_num(uint32_t bits_num){	
	uint32_t num = 0;
	
	/* create random number by doing bits_num lfsr shifts */
	num = ((1 << bits_num) -1) & lfsr_state;
	/* move bits to make sure we do not use them again*/
	gen_rand_bit(bits_num);	
	
	return num;
}

//int self_test() {
//  long long int i;
//  long long int counter=0;
//  
//  lfsr_state = 0x12345678;
//  
//  printf("Before all 0x%x\n",lfsr_state);
//
//  for (i=0; i < ((long long)1<<31) ; i++) 
//    TACT_LFSR;
//
//  printf("After half 0x%x\n",lfsr_state);
//
//  for (i=0; i < (((long long)1<<31)-1) ; i++) 
//    TACT_LFSR;
//
//  printf("After all 0x%x\n",lfsr_state);
//  
//  for (i=0; i < (((long long)1<<32)-1) ; i++) {
//    TACT_LFSR;
//    if (lfsr_state == 0x12345678)
//      counter++;
//  }
//  printf("Is %d==1 ?!?\n",counter);
//  assert(counter == 1);
//
//}

//int main() {
//
//  long long int i=0;
//  long long int counter=0;
//  
//  lfsr_state = 0x8172abd1;
//
//  assert(sizeof(lfsr_state) == 4); // 32-bit LFSR
//  assert(NEEDED_PROB < 32); // NOT MUCH OF A TEST, we need "<<"
//  
//  /*
//  for (i=0;i < (1<<(NEEDED_PROB+7)); i++)
//    counter += gen_rand_bit();
//    
//  printf("Counter=%d\n",counter);
//  */
//  self_test();
//
//  return 0;
//}
