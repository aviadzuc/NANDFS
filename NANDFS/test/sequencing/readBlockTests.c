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

/** @file readBlockTests.c  */
#include <test/sequencing/readBlockTests.h>
#include <test/sequencing/testsHeader.h>

/**
 * run all readBlock tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllReadBlockTests(){
	RUN_TEST(readBlock, 1);
	RUN_TEST(readBlock, 2);
	RUN_TEST(readBlock, 3);

	return 0;
}

/**
 * @brief 
 * init readBlock test
 */
error_t init_readBlockTest(){
	if(nandInit())
		return -1;		
		
	init_seg_map();
	init_obs_counters()	;		
	init_flash();
	mark_reserve_segment_headers();
	init_buf(sequencing_buffer);
	
	return 1;	
}									

/**
 * @brief 
 * tear down readBlock test
 */
error_t tearDown_readBlockTest(){
	init_seg_map();
	init_obs_counters();	
	init_flash();
	nandTerminate();		
	init_buf(sequencing_buffer);
	
	return 1;
}

/**
 * @brief
 * write a block, read it and verify compared read is as expected
 * @return 1 if test was successful
 */
error_t readBlockTest1(){
	uint32_t i, seg_num = 5, slot_id = 2, page_offset = 3, new_slot_id = slot_id, sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	
	init_logical_address(log_addr);
	init_logical_address(prev_log_addr);	
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 SEQ_NO_SEGMENT_NUM, // previously reclaimed segment
				 is_eu_reserve,
				 1,
				 SEQ_PHY_ADDRESS_EMPTY);
				 
	fsMemset(buf, byte, NAND_PAGE_SIZE);
	
	VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0));
//	PRINT("\nallocAndWriteBlock success");
	
	VERIFY(!readBlock(log_addr, buf));	
//	PRINT("\nread block success");
	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(log_addr));
//	PRINT("\nprint block from seq buffer");
//	readBlock(log_addr, sequencing_buffer);
	
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(buf[i], byte));
	}
	
	return 1;
}

/**
 * @brief
 * write a block to a logical address whose physical address is corrupt,
 * read it and verify compared read is as expected
 * @return 1 if test was successful
 */
error_t readBlockTest2(){
	uint32_t i, seg_num = 5, slot_id = 2, page_offset = 3, new_slot_id = slot_id, sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_TRUE;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	uint32_t orig_phy_addr;
	
	init_logical_address(prev_log_addr);
	init_logical_address(log_addr);
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 SEQ_NO_SEGMENT_NUM, // previously reclaimed segment
				 is_eu_reserve,
				 1,
				 SEQ_PHY_ADDRESS_EMPTY);
				 
	init_buf(sequencing_buffer);
	fsMemset(sequencing_buffer, byte, NAND_PAGE_SIZE);
	
	/* rememebr original physical address, and initialize valid eus queue
	 * so  the system doesn't think this is a valid eu (it does after logicalAddressToPhysical passes ok)*/
	orig_phy_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	init_valid_eus_q();

	/*mark eu as bad physically and mark flag in seg map accordingly*/
	markEuAsMockBad(orig_phy_addr);
	seg_map_ptr->reserve_eu_addr = write_to_reserve_eu(orig_phy_addr);

	init_buf(buf);
	fsMemset(buf, byte, NAND_PAGE_SIZE);
	VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0));
	
	init_buf(buf);	
	VERIFY(!readBlock(log_addr, buf));
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(buf[i], byte));
	}
	
	return 1;
}

/**
 * @brief
 * write 2 blocks with different data
 * read and verify compared read is as expected
 * @return 1 if test was successful
 */
error_t readBlockTest3(){
	uint32_t i, seg_num = 5, slot_id = 2, page_offset = 3, new_slot_id = slot_id, sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr1);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];	
	
	init_logical_address(log_addr);	
	init_logical_address(log_addr1);	
	init_logical_address(prev_log_addr);
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 SEQ_NO_SEGMENT_NUM, // previously reclaimed segment
				 is_eu_reserve,
				 1,
				 SEQ_PHY_ADDRESS_EMPTY);
	/* first block allocation*/			 
	init_buf(buf);
	fsMemset(buf, byte, NAND_PAGE_SIZE);	
	
	VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0));
	
	init_buf(sequencing_buffer);	
	
	/* second block allocation*/			 
	init_buf(buf);
	fsMemset(buf, byte+1, NAND_PAGE_SIZE);		
	VERIFY(!allocAndWriteBlock(log_addr1, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0));
	
	init_buf(buf);	
	
	/* read first block*/
	readBlock(log_addr, buf);
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(buf[i], byte));
	}
	
	/* read second block*/
	readBlock(log_addr1, buf);
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(buf[i], byte+1));
	}	
	
	return 1;
}
