/*************************************************
 * Copyright (C) 2009 Aviad Zuck & Sivan Toledo
 * This file is part of NANDFS.
 *
 * To license NANDFS under a different license, please
 * contact the authors.
 *
 * NANDFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NANDFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with NANDFS.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file bit_generator_mechanismTests.c  */
#include <test/sequencing/bit_generator_mechanismTests.h>
#include <test/sequencing/testsHeader.h>

extern uint32_t lfsr_state;

/**
 * run all bit_generator_mechanism tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllbit_generator_mechanismTests(){
	RUN_TEST(bit_generator_mechanism,1);
	
	return 0;
}

error_t init_bit_generator_mechanismTest(){
	lfsr_state = LFSR_STATE_INITIAL;
	
	return 1;
}

error_t tearDown_bit_generator_mechanismTest(){	
	
	return 1;
}

/**
 * @brief 
 * test that our random bit generator indeed repeats otself only after 2^32-1 times
 * @return 1 if successful, 0 otherwise
 */
uint32_t bit_generator_mechanismTest1() {
  int64_t i,j, count = 0;
  int64_t counter=0;
  int64_t expected_counter=5;    
  
  for(j=0 ; j<expected_counter; j++){
	  for (i=0; i < (int64_t)(((int64_t)1<<32)-1) ; i++) {
	    TACT_LFSR();
	    
	    if(i% 100000000==0){
	    	count++;
		    print(uart0SendByte,0,"\nanother 100 million tests - ");
	        printNum(uart0SendByte,0, count);
	    }
	    
	    if (lfsr_state == 0x12345678)
	      counter++;
	  }  
  }
  
  return (counter == expected_counter);
}

