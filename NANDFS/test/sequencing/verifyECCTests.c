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

/** @file verifyECCTests.c  */
#include <test/sequencing/verifyECCTests.h>
#include <test/sequencing/testsHeader.h>

#ifdef DO_ECC
/**
 * run all verifyECC tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllVerifyECCTests(){
	RUN_TEST(verifyECC, 1);
	RUN_TEST(verifyECC, 2);			
	RUN_TEST(verifyECC, 3);			
	
	return 0;
}

/**
 * @brief 
 * init verifyECC test
 */
error_t init_verifyECCTest(){	
	init_buf(sequencing_buffer);
	
	return 1;
}									

/**
 * @brief 
 * tear down verifyECC test
 */
error_t tearDown_verifyECCTest(){			
	init_buf(sequencing_buffer);
	
	return 1;
}

/**
 * @brief 
 * test verifyECC - calculate ecc for data buffer, verify recalculation succeeds,
 * @return 1 if successful, 0 otherwise
 */
error_t verifyECCTest1(){
	uint32_t i;
	
	/* set buffer*/
	for(i=0; i<NAND_PAGE_SIZE;i++){		
		sequencing_buffer[i] = get_random_num(8);
	}
//	sequencing_buffer[5] = 0x69;
	
	setECC(sequencing_buffer);
	VERIFY(!verifyECC(sequencing_buffer));	
	
	return 1; 
}

/**
 * @brief 
 * test verifyECC - calculate ecc for data buffer, make it erroneous with 2 bits (not recoverable).
 * verify recalculation fails
 * @return 1 if successful, 0 otherwise
 */
error_t verifyECCTest2(){
	uint32_t i;
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
//	PRINT("\n");
	
	/* set buffer*/
	for(i=0; i<NAND_PAGE_SIZE;i++){		
		sequencing_buffer[i] = get_random_num(8);
//		PRINT_MSG_AND_HEX("",sequencing_buffer[i]);		
	}	

	setECC(sequencing_buffer);	
	/* insert several bit error*/
	seq_flags->bytes[ECC_LOCATION] -= 3;	
	VERIFY(verifyECC(sequencing_buffer));	
	
	return 1; 
}

/**
 * @brief 
 * test verifyECC - calculate ecc for data buffer, make it erroneous with 1 bit (recoverable).
 * verify recalculation succeeds
 * @return 1 if successful, 0 otherwise
 */
error_t verifyECCTest3(){
	uint32_t i;
	uint8_t read_ecc[3];	
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	
//	PRINT("\n");
	/* set buffer*/
	for(i=0; i<NAND_PAGE_SIZE;i++){		
		sequencing_buffer[i] = get_random_num(8);
//		PRINT_MSG_AND_HEX("",sequencing_buffer[i]);		
	}	
	
	sequencing_buffer[242] = 0xff;

	setECC(sequencing_buffer);
	read_ecc[0] = seq_flags->bytes[ECC_LOCATION];
	read_ecc[1] = seq_flags->bytes[1+ECC_LOCATION];
	read_ecc[2] = seq_flags->bytes[2+ECC_LOCATION];
//	for(j=0;j<3;j++){
//		PRINT_MSG_AND_HEX("\nverifyECCTest3() - read_ecc[",j);
//		PRINT_MSG_AND_HEX("]=",read_ecc[j]);
//	}
		
	/* insert one bit error*/
//	seq_flags->bytes[ECC_LOCATION] -= 1;
	sequencing_buffer[242] = 0xf7;
	VERIFY(!verifyECC(sequencing_buffer));	
	
	return 1; 
}
#endif
