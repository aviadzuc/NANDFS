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

/** @file variousTests.c  */
#include <test/sequencing/variousTests.h>
#include <test/sequencing/testsHeader.h>

/**
 * run all various tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllVariousTests(){
	RUN_TEST(nandReadPageSpare, 1);
//	PRINT("\nsuccess1");
	RUN_TEST(bit_fields, 1);
//	PRINT("\nsuccess2");
	
	return 0;
}

/**
 * @brief 
 * init nandReadPageSpare test
 */
error_t init_nandReadPageSpareTest(){
	if(nandInit())
		return -1;		
				
	init_flash();	
	init_buf(sequencing_buffer);
	
	return 1;	
}									

/**
 * @brief 
 * tear down nandReadPageSpare test
 */
error_t tearDown_nandReadPageSpareTest(){
//	init_flash();	
	init_buf(sequencing_buffer);
	nandTerminate();			
	
	return 1;
}

error_t nandReadPageSpareTest1(){
	uint32_t i,phy_addr = 10;
	uint8_t byte = 'c';
	INIT_FLAGS_STRUCT_AND_PTR(only_flags);
	
	fsMemset(sequencing_buffer,byte,NAND_TOTAL_SIZE);

	nandProgramTotalPage(sequencing_buffer,phy_addr);	
	
	init_buf(sequencing_buffer);
	nandReadPage(sequencing_buffer, phy_addr, 0, NAND_TOTAL_SIZE);
	
//	for(i=0;i<NAND_TOTAL_SIZE;i++){
//		PRINT_MSG_AND_HEX("\nsequencing_buffer[i]=",);
////		VERIFY(COMPARE(sequencing_buffer[i],byte));
//	}
//	PRINT("\nsequencing buffer flags ok");
	nandReadPageSpare(CAST_TO_UINT8(only_flags), phy_addr, 0, NAND_SPARE_SIZE);
	for(i=0;i<NAND_SPARE_SIZE;i++){
//		PRINT_MSG_AND_HEX("\nsequencing_buffer[NAND_PAGE_SIZE+i]=",sequencing_buffer[NAND_PAGE_SIZE+i]);
//		PRINT_MSG_AND_HEX("\nCAST_TO_UINT8(only_flags)[i]=",CAST_TO_UINT8(only_flags)[i]);
		VERIFY(COMPARE(CAST_TO_UINT8(only_flags)[i],sequencing_buffer[NAND_PAGE_SIZE+i]));
	}
	
	return 1;
}

/**
 * @brief 
 * init bit_fields test
 */
error_t init_bit_fieldsTest(){	
	return 1;	
}									

/**
 * @brief 
 * tear down bit_fields test
 */
error_t tearDown_bit_fieldsTest(){
	return 1;
}

error_t bit_fieldsTest1(){
	logical_addr_struct str;
	logical_addr_t las = &str;
	
	SET_LOGICAL_OFFSET(las, 0);
	SET_LOGICAL_SEGMENT(las, 0);
	SET_LOGICAL_SPARE(las, 0);	
	
	SET_LOGICAL_OFFSET(las, 0xfff);	
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(las) ,0));
	VERIFY(COMPARE(GET_LOGICAL_SPARE(las) ,0));
	
	SET_LOGICAL_OFFSET(las, 0);
	SET_LOGICAL_SEGMENT(las, 0);
	SET_LOGICAL_SPARE(las, 0);	
	
	SET_LOGICAL_SEGMENT(las, 0xfff);
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(las), 0));
	VERIFY(COMPARE(GET_LOGICAL_SPARE(las), 0));
	
	SET_LOGICAL_OFFSET(las, 0);
	SET_LOGICAL_SEGMENT(las, 0);
	SET_LOGICAL_SPARE(las, 0);	
	
	SET_LOGICAL_SPARE(las, 0xff);
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(las),0));
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(las),0));
	
	return 1;
}
