/** @file lfsr.h
 * Header file for random bit generation functions
 */
#ifndef LFSR_H_
#define LFSR_H_

#include <system.h>

/* general defenitions */
#define LFSR_MASK           ((uint32_t)0x80200003)
#define LFSR_STATE_INITIAL  0x63478395
#define TACT_LFSR() lfsr_state = ((lfsr_state >> 1) ^ ((lfsr_state & 1)? LFSR_MASK : 0 ))

extern uint32_t lfsr_state;

/**
 *@brief 
 * generate bit randomly according to setting 
 * @param needed probability for the generator
 * @return the bit 
 */
uint32_t gen_rand_bit(uint32_t needed_prob);

/**
 *@brief 
 * egt random number with bits_num bits
 * @param number of bits to retrieve
 * @return the nubmer
 */
uint32_t get_random_num(uint32_t bits_num);
#endif /*LFSR_H_*/
