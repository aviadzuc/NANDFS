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

/** @file readVOTsAndPrevTests.c  */
#include <test/sequencing/readVOTsAndPrevTests.h>
#include <test/sequencing/testsHeader.h>

/**
 * run all readVOTsAndPrev tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllReadVOTsAndPrevTests(){
	RUN_TEST(readVOTsAndPrev, 1);		
	RUN_TEST(readVOTsAndPrev, 2);		
	RUN_TEST(readVOTsAndPrev, 3);		
	RUN_TEST(readVOTsAndPrev, 4);		
	RUN_TEST(readVOTsAndPrev, 5);		
	RUN_TEST(readVOTsAndPrev, 6);
	
	return 0;
}

/**
 * @brief 
 * init readVOTsAndPrev test
 */
error_t init_readVOTsAndPrevTest(){
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
 * tear down readVOTsAndPrev test
 */
error_t tearDown_readVOTsAndPrevTest(){
	init_seg_map();
	init_obs_counters();	
	init_flash();
	nandTerminate();		
	init_buf(sequencing_buffer);
	
	return 1;
}

/**
 * @brief
 * write a page marked as VOTs, read it using readVOTsAndPrevTest1() and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest1(){
	uint32_t i, seg_num = 5, slot_id = 2, sequence_num = 3, new_slot_id = slot_id, is_eu_reserve = IS_EU_RESERVE_FALSE;
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	bool_t isVOTs; 
	//INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	//logical_addr_struct addr_struct;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	
	init_logical_address(log_addr);	
	init_logical_address(prev_log_addr);
	seg_map_ptr->seg_to_slot_map[seg_num] = slot_id;
	
//	PRINT("\n set segment map");
	set_seg_map(seg_map_ptr,
				 slot_id,
				 3, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 SEQ_NO_SEGMENT_NUM, // previously reclaimed segment
				 is_eu_reserve,
				 1,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	assert(prev_log_addr != log_addr);
	
	/* write buffer marked as vot's to flash*/
	init_buf(buf);	
	fsMemset(buf,byte, NAND_PAGE_SIZE);	
	
//	for(i = 0; i<NAND_PAGE_SIZE; i++		){
//		PRINT_MSG_AND_NUM("\nbefore writing byte ",i);
//		PRINT_MSG_AND_NUM(" = ",sequencing_buffer[i]);
//	}
	VERIFY(!allocAndWriteBlock(log_addr, buf, 1, prev_log_addr, &cpWritten, checkpointWriter,0));
		
//	PRINT_MSG_AND_NUM("\nblock was written to offset ",log_addr->page_offset);	
//	PRINT_MSG_AND_NUM("\nsegment_num ",prev_log_addr->segment_num);
	
	/* try reading it */
//	PRINT("\n try reading");
	init_buf(buf);
	/* readVOTsAndPrev and verify read */
	VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
//	PRINT("\n reading succeeds");
	/* verify no prev */	
//	PRINT_MSG_AND_NUM("\nprev_log_addr->segment_num",prev_log_addr->segment_num);	
	VERIFY(IS_ADDRESS_EMPTY(prev_log_addr));
//	PRINT("\n prev address succeeds");	
	/* verify page was written properly*/
	for(i=0; i<NAND_PAGE_SIZE; i++){
//		PRINT_MSG_AND_NUM("\nbyte is",sequencing_buffer[i] );
		VERIFY(COMPARE(buf[i], byte));
	}
	
	VERIFY(isVOTs);
	
//	PRINT("\n page succeeds");
	return 1;
}

/**
 * @brief
 * - write a page marked as VOTs
 * - write another regular page
 * - write a page marked as VOTs with the previous VOTs write marked in it's header
 * read pages using readVOTsAndPrevTest1() and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest2(){
	uint32_t seg_num = 5, slot_id = 2, page_offset = 3, sequence_num = 3, new_slot_id = slot_id, is_eu_reserve = IS_EU_RESERVE_FALSE;
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	bool_t isVOTs;
	
	init_logical_address(log_addr);	
	init_logical_address(prev_log_addr);
	seg_map_ptr->seg_to_slot_map[seg_num] = slot_id;
	
//	PRINT("\n set segment map");
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
	
	assert(prev_log_addr != log_addr);
	
	//	for(i = 0; i<NAND_PAGE_SIZE; i++		){
//		PRINT_MSG_AND_NUM("\nbefore writing byte ",i);
//		PRINT_MSG_AND_NUM(" = ",sequencing_buffer[i]);
//	}
	/* write buffer marked as vot's to flash*/	
	fsMemset(buf,byte, NAND_PAGE_SIZE);
	
	/* write buffer marked as vot's to flash*/	
	VERIFY(!allocAndWriteBlock(log_addr, buf, 1, prev_log_addr, &cpWritten, checkpointWriter,0));
	
	/* the address now becomes previous adderss*/
	copyLogicalAddress(prev_log_addr, log_addr);
//	PRINT_MSG_AND_NUM("\nblock was written to offset ",log_addr->segment_num);	
//	PRINT_MSG_AND_NUM("\nits offset is now ",log_addr->page_offset);	
//	PRINT_MSG_AND_NUM("\ncopy to prev. its segment is now ",prev_log_addr->segment_num);	
//	PRINT_MSG_AND_NUM("\nsegment_num ",prev_log_addr->page_offset);
	
	init_buf(sequencing_buffer);
	fsMemset(buf,byte+1, NAND_PAGE_SIZE);
	
	/* write data buffer to flash*/	
	VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0));
//	PRINT_MSG_AND_NUM("\ndata block was written to offset ",log_addr->page_offset);	
	/* write buffer marked as vot's to flash*/	
	
	init_buf(sequencing_buffer);
	fsMemset(buf,byte, NAND_PAGE_SIZE);
//	copyPrevAddressToFlags(flags, prev_log_addr);
	copyLogicalAddress(prev_log_addr, log_addr);
//	PRINT_MSG_AND_NUM("\ncopied prev to flags. its offset is ",prev_log_addr->page_offset);	
//	PRINT_MSG_AND_NUM("\n and its segment is ",prev_log_addr->segment_num);	
//	PRINT_MSG_AND_NUM("\n flags->prev_block_ptr= ",flags->prev_block_ptr);	
	/* write buffer marked as vot's to flash*/	
	VERIFY(!allocAndWriteBlock(log_addr, buf, 1, prev_log_addr, &cpWritten, checkpointWriter,0));
//	PRINT_MSG_AND_NUM("\nsecond VOT's block was written to offset ",log_addr->page_offset);	
	/* try reading it */
//	PRINT("\n try reading");
	init_buf(sequencing_buffer);	
	/* readVOTsAndPrev and verify page is indeed marked as VOTs*/
	VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
	VERIFY(isVOTs);
//	PRINT_MSG_AND_NUM("\nfirst readVOTsAndPrev succeeds,prev is in offset ",prev_log_addr->page_offset);
	
	/* verify we indeed read previous page properly*/
	VERIFY(!IS_ADDRESS_EMPTY(prev_log_addr));
		
	init_logical_address(log_addr);
	/* and try reading previous page*/
	VERIFY(!readVOTsAndPrev(prev_log_addr, buf, log_addr, &isVOTs));
	VERIFY(!isVOTs);
//	PRINT("\nsecond readVOTsAndPrev succeeds");	
	VERIFY(!IS_ADDRESS_EMPTY(log_addr));	

	/* and try reading previous page*/
	VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
	VERIFY(isVOTs);
//	PRINT("\n prev address succeeds");	
	
	return 1;
}

/**
 * @brief
 * repeat k times the process of 
 * - write VOTs
 * - write data
 * and try k sequential reads using readVOTsAndPrevTest1() and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest3(){
	uint32_t k=100, i, seg_num = 5, slot_id = 2, page_offset = 3, sequence_num = 3, new_slot_id = slot_id, is_eu_reserve = IS_EU_RESERVE_FALSE;
	uint8_t buf[NAND_TOTAL_SIZE];
	bool_t isVOTs;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(data_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_data_log_addr);
	
	init_logical_address(log_addr);	
	init_logical_address(prev_log_addr);
	init_logical_address(data_log_addr);	
	init_logical_address(prev_data_log_addr);
		
	SET_ADDRESS_EMPTY(log_addr);
	SET_ADDRESS_EMPTY(prev_log_addr);

	seg_map_ptr->seg_to_slot_map[seg_num] = slot_id;	
		
//	PRINT("\n set segment map");
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
	
	assert(prev_log_addr != log_addr);	
	
//	PRINT("\nwriting mock pages");
	/* write buffer marked as vot's to flash*/	
	for(i=0;i<k;i++){
		VERIFY(writeMockVOTsPage(log_addr, prev_log_addr));
//		PRINT_MSG_AND_NUM("\nwrote vots to offset=", GET_LOGICAL_OFFSET(prev_log_addr));				
		VERIFY(writeMockDataPage(log_addr, prev_log_addr));				
//		PRINT_MSG_AND_NUM("\nwrote data to offset=", GET_LOGICAL_OFFSET(prev_log_addr));
	}
	
//	copyLogicalAddress(log_addr, data_log_addr);
	/* try reading it */
//	PRINT("\n try reading");
	for(i=0;i<k;i++){
//		PRINT_MSG_AND_NUM("\ni=",i);
		init_buf(buf);
		/* readVOTsAndPrev and verify page is indeed marked as VOTs*/
		VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));		
//		PRINT_MSG_AND_NUM("\nprev offset=", GET_LOGICAL_OFFSET(prev_log_addr));
		VERIFY(!isVOTs);
		copyLogicalAddress(log_addr, prev_log_addr);		
//		PRINT("\ndata success");
		VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
		VERIFY(isVOTs);		
		copyLogicalAddress(log_addr, prev_log_addr);
//		PRINT("\nvot success");
	}
//	PRINT("\n finished reading");
	/* check prev for the last page - should be empty*/
//	VERIFY(!readVOTsAndPrev(log_addr, sequencing_buffer, prev_log_addr, &isVOTs));
//	VERIFY(isVOTs);
//	VERIFY(IS_ADDRESS_EMPTY(prev_log_addr));		
	
	return 1;
}

/**
 * @brief
 * repeat k times the process of 
 * - write VOTs of list
 * - write data
 * - write VOTs of different list
 * - write data
 * and try k sequential reads using readVOTsAndPrevTest1() and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest4(){
	uint32_t k=10, i, seg_num = 5, slot_id = 2, page_offset = 3, sequence_num = 3, new_slot_id = slot_id, is_eu_reserve = IS_EU_RESERVE_FALSE;		
	uint8_t buf[NAND_TOTAL_SIZE];
	bool_t isVOTs;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(data_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_data_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);		
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr1);		
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr1);
	
	init_logical_address(data_log_addr);	
	init_logical_address(prev_data_log_addr);	
	init_logical_address(log_addr);		
	init_logical_address(prev_log_addr);
	init_logical_address(log_addr1);		
	init_logical_address(prev_log_addr1);
	
	seg_map_ptr->seg_to_slot_map[seg_num] = slot_id;
	SET_ADDRESS_EMPTY(log_addr);
	SET_ADDRESS_EMPTY(prev_log_addr);
	SET_ADDRESS_EMPTY(log_addr1);
	SET_ADDRESS_EMPTY(prev_log_addr1);
//	PRINT("\n set segment map");
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
	
	assert(prev_log_addr != log_addr);	
	
	/* write buffer marked as vot's to flash*/	
	for(i=0;i<k;i++){
		VERIFY(writeMockVOTsPage(log_addr, prev_log_addr));
		VERIFY(writeMockDataPage(data_log_addr, prev_data_log_addr));		
		VERIFY(writeMockVOTsPage(log_addr1, prev_log_addr1));
		VERIFY(writeMockDataPage(data_log_addr, prev_data_log_addr));	
	}

	/* try reading it */
//	PRINT("\n try reading");
	for(i=0;i<k-1;i++){
		init_buf(buf);
		/* readVOTsAndPrev and verify page is indeed marked as VOTs*/
//		PRINT_MSG_AND_NUM("\nreading VOTs from offset ",log_addr->page_offset);
		VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
		VERIFY(isVOTs);	
		/* verify we indeed read previous page properly*/
		VERIFY(!IS_ADDRESS_EMPTY(prev_log_addr));
		copyLogicalAddress(log_addr, prev_log_addr);
		SET_ADDRESS_EMPTY(prev_log_addr);		
	}	
//	PRINT("\n finished reading");
	/* check prev for the last page - should be empty*/
//	PRINT_MSG_AND_NUM("\nreading VOTs from offset ",log_addr->page_offset);
	VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
	VERIFY(isVOTs);		
	VERIFY(IS_ADDRESS_EMPTY(prev_log_addr));		
	
	for(i=0;i<k-1;i++){
		init_buf(buf);
		/* readVOTsAndPrev and verify page is indeed marked as VOTs*/
//		PRINT_MSG_AND_NUM("\nreading VOTs from offset ",log_addr1->page_offset);
		VERIFY(!readVOTsAndPrev(log_addr1, buf, prev_log_addr1, &isVOTs));
		VERIFY(isVOTs);		
		/* verify we indeed read previous page properly*/
		VERIFY(!IS_ADDRESS_EMPTY(prev_log_addr1));
		copyLogicalAddress(log_addr1, prev_log_addr1);
		SET_ADDRESS_EMPTY(prev_log_addr1);	
	}
	/* check prev for the last page - should be empty*/
//	PRINT_MSG_AND_NUM("\nreading VOTs from offset ",log_addr1->page_offset);
	VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
	VERIFY(isVOTs);		
	VERIFY(IS_ADDRESS_EMPTY(prev_log_addr));		

	/* check prev for the last page - should be empty*/
	VERIFY(!readVOTsAndPrev(log_addr1, buf, prev_log_addr1, &isVOTs));
	VERIFY(IS_ADDRESS_EMPTY(prev_log_addr1));		
	
	return 1;
}

/**
 * @brief
 * repeat k times the process of 
 * - write VOTs to a segment
 * - write data to another segmnet
 * - write VOTs again for the other segmnet
 * and try to read using readVOTsAndPrevTest1() and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest5(){
	uint32_t seg_num = 5, slot_id = 2, page_offset = SEQ_PAGES_PER_SLOT-5, sequence_num = 3, new_slot_id = slot_id, is_eu_reserve = IS_EU_RESERVE_FALSE,
			 seg_num1 = 0, slot_id1 = 120;	
	bool_t isVOTs;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);		
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(data_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_data_addr);	
	uint8_t buf[NAND_TOTAL_SIZE];
	
	init_logical_address(log_addr);		
	init_logical_address(prev_log_addr);
	init_logical_address(data_log_addr);	
	init_logical_address(prev_data_addr);	
	
	SET_ADDRESS_EMPTY(log_addr);
	SET_ADDRESS_EMPTY(prev_log_addr);
//	PRINT("\n set segment map");
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 SEQ_NO_SEGMENT_NUM, // previously reclaimed segment
				 is_eu_reserve,
				 2,
				 SEQ_PHY_ADDRESS_EMPTY);
	seg_map_ptr->seg_to_slot_map[seg_num1] = slot_id1;		
	
	/* write buffer marked as vot's to flash, to first segment*/	
	VERIFY(writeMockVOTsPage(log_addr, prev_log_addr));
	
	/* move to next segment*/
	SET_RECLAIMED_SEGMENT(5);
	SET_RECLAIMED_OFFSET(78);
	seg_map_ptr->previously_written_segment = seg_num;
	seg_map_ptr->new_slot_id = slot_id1;
	/* write data in next segment*/
	VERIFY(writeMockDataPage(data_log_addr, prev_data_addr));			

	/* write buffer marked as vot's to flash, to first segment*/	
	VERIFY(writeMockVOTsPage(log_addr, prev_log_addr));
	
	/* try reading the VOTs, should be read twice */
	init_buf(buf);
	/* readVOTsAndPrev and verify page is indeed marked as VOTs*/
//	PRINT_MSG_AND_NUM("\nreading VOTs from offset ",log_addr->page_offset);
	VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
	VERIFY(isVOTs);		
	/* verify we indeed read previous page properly*/
	VERIFY(!IS_ADDRESS_EMPTY(prev_log_addr));
	copyLogicalAddress(log_addr, prev_log_addr);
	SET_ADDRESS_EMPTY(prev_log_addr);	
	/* check prev for the last page - should be empty*/
//	PRINT_MSG_AND_NUM("\nreading VOTs from offset ",log_addr->page_offset);
	VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
	VERIFY(isVOTs);		
	VERIFY(IS_ADDRESS_EMPTY(prev_log_addr));		
	
	return 1;
}

/**
 * @brief
 * repeat k times the process of 
 * - write VOTs
 * - write data
 * - write unrelated data
 * and try k sequential reads using readVOTsAndPrevTest1() and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest6(){
	uint32_t k=100, i, seg_num = 5, slot_id = 2, page_offset = 3, sequence_num = 3, new_slot_id = slot_id, is_eu_reserve = IS_EU_RESERVE_FALSE;	
	uint8_t buf[NAND_TOTAL_SIZE];
	bool_t isVOTs;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(data_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_data_log_addr);	
	SET_ADDRESS_EMPTY(log_addr);
	SET_ADDRESS_EMPTY(prev_log_addr);
	
	init_logical_address(log_addr);	
	init_logical_address(prev_log_addr);
	init_logical_address(data_log_addr);	
	init_logical_address(prev_data_log_addr);	

	seg_map_ptr->seg_to_slot_map[seg_num] = slot_id;		
//	PRINT("\n set segment map");
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
	
	assert(prev_log_addr != log_addr);	
	
	/* write buffer marked as vot's to flash*/	
	for(i=0;i<k;i++){
		VERIFY(writeMockVOTsPage(log_addr, prev_log_addr));
		VERIFY(writeMockDataPage(log_addr, prev_log_addr));		
		VERIFY(writeMockDataPage(data_log_addr, prev_data_log_addr));		
	}

	/* try reading it */
//	PRINT("\n try reading");
	for(i=0;i<k;i++){
//		PRINT_MSG_AND_NUM("\ni=",i);
		init_buf(sequencing_buffer);
		/* readVOTsAndPrev and verify page is indeed marked as VOTs*/
		VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
		VERIFY(!isVOTs);				
		copyLogicalAddress(log_addr, prev_log_addr);		
		
		VERIFY(!readVOTsAndPrev(log_addr, buf, prev_log_addr, &isVOTs));
		VERIFY(isVOTs);
		copyLogicalAddress(log_addr, prev_log_addr);
	}
//	PRINT("\n finished reading");		
	
	return 1;
}
