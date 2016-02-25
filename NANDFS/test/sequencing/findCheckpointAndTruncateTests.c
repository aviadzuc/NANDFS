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

/** @file findCheckpointAndTruncateTests.c  */
#include <test/sequencing/findCheckpointAndTruncateTests.h>
#include <test/sequencing/testsHeader.h>

/**
 * run all findCheckpointAndTruncate tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllfindCheckpointAndTruncateTests(){
	RUN_TEST(findCheckpointAndTruncate, 1);
	RUN_TEST(findCheckpointAndTruncate, 2);
	RUN_TEST(findCheckpointAndTruncate, 3);
	RUN_TEST(findCheckpointAndTruncate, 4);
	RUN_TEST(findCheckpointAndTruncate, 5);		
	RUN_TEST(findCheckpointAndTruncate, 6);		
	RUN_TEST(findCheckpointAndTruncate, 7);
	RUN_TEST(findCheckpointAndTruncate, 8);

	return 0;
}

/**
 * @brief 
 * init findCheckpointAndTruncate test
 */
error_t init_findCheckpointAndTruncateTest(){
	if(nandInit())
		return -1;		
		
	init_seg_map();
	init_obs_counters()	;		
	init_flash();
	mark_reserve_segment_headers();
	
//	PRINT_MSG_AND_NUM("\ninit_findCheckpointAndTruncateTest() - slot of SEQ_FIRST_RESERVE_SEGMENT =",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,SEQ_FIRST_RESERVE_SEGMENT));
	return 1;	
}									

/**
 * @brief 
 * tear down findCheckpointAndTruncate test
 */
error_t tearDown_findCheckpointAndTruncateTest(){
	init_seg_map();
	init_obs_counters();	
	init_flash();
	nandTerminate();		
	
	return 1;
}

/**
 * @brief
 * write a checkpoint to header of segment and stop. then try to find the checkpoint
 * test checkpoint was read properly, that pending VOTs is set as expected.
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest1(){
	// set logical address at the beginning of a segment
	uint32_t i, seg_num = 0, slot_id = 0, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = 20,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = 2,
			 fs_data_size, page_offset = 1, new_page_offset;
	uint32_t cp_size;
	uint32_t old_count = 100;			
	bool_t pending_VOTs;	
	uint8_t obs_level;
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_1;
#else
	obs_level = 5;
#endif	
	if(SEQ_CHECKPOINT_SIZE > NAND_PAGE_SIZE){
		fs_data_size = NAND_PAGE_SIZE;
	}
	else{
		fs_data_size = NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200;
	}
	uint8_t fs_data[fs_data_size], test_byte = 'a';	
	cp_size          = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+SEQ_SEG_HEADER_PAGES_COUNT;
	
	// init segment map with various details
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	// init obs map	
	set_obs_counters(obs_level);	
	SET_OBS_COUNT(old_count);
	
	/* set fs_data*/
	fsMemset(fs_data, test_byte, fs_data_size);	
				
	// write with no file system data (for now)
	VERIFY(!commit(fs_data, fs_data_size,1));
//	PRINT_MSG_AND_NUM("\n after commit() GET_RECLAIMED_OFFSET()=", GET_RECLAIMED_OFFSET());	
	/* save new page offset*/	
	
		
	new_page_offset = GET_RECLAIMED_OFFSET();
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncateTest1() - new_page_offset ",new_page_offset );	
	/********* REBOOT!****************/
	mock_reboot();
	simpleRemarkReserveSegments();	
	/* init fs_data*/
	fsMemset(fs_data, 0xff, fs_data_size);	
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 new_page_offset, 
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);	
	
	new_page_offset = GET_RECLAIMED_OFFSET();	
	/* file system and sequencing data is initialized. find the checkpoint and verify we read everything properly */	
	VERIFY(!findCheckpointAndTruncate(fs_data, fs_data_size, &pending_VOTs));
	
	/* verify pending VOTS (since no page was written after the checkpoint)*/
	VERIFY(pending_VOTs);
//	print(uart0SendByte,0,"\nsuccess pending_VOTs ");
	
	/* compare expected data in fs_data*/
	for(i=0; i< fs_data_size; i++){		
		VERIFY(COMPARE(fs_data[i], test_byte));
	}
//	print(uart0SendByte,0,"\nsuccess fs_data ");
	/* compare written and current segment map */
	VERIFY(verify_seg_map(seg_map_ptr, 
				   sequence_num,
				   new_slot_id,
				   previously_written_segment,
				   is_eu_reserve,
				   nSegments));  
	
//	PRINT_MSG_AND_NUM("\n GET_RECLAIMED_OFFSET()=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nCALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+page_offset-1=",CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+page_offset-1);
	/* verify page offset is correct*/
	VERIFY(COMPARE(cp_size, GET_RECLAIMED_OFFSET()));
//	PRINT("\ncp size verified");
	/* verify the only segment we allocated*/ 
	VERIFY(COMPARE(seg_map_ptr->seg_to_slot_map[seg_num], slot_id));		
	for(i=0; i<SEQ_SEGMENTS_COUNT ;i++){
		if(i!=seg_num){
//			PRINT_MSG_AND_NUM("\n verifying seg empty ", i);			
//			PRINT_MSG_AND_NUM("\n seg slot is ", seg_map_ptr->seg_to_slot_map[i]);
			VERIFY(COMPARE(seg_map_ptr->seg_to_slot_map[i], SEQ_NO_SLOT));					
		}
	}
//	print(uart0SendByte,0,"\nsuccess segments ");
	/* compare written and read checkpoint */
	for(i=0; i<SEQ_OBS_COUNTERS; i++){				
		VERIFY(COMPARE(obs_level, get_counter_level(i)));
	}
//	print(uart0SendByte,0,"\nsuccess obs couters ");
	
	VERIFY(!isThereCopyBackEU());	
	VERIFY(COMPARE(old_count, GET_OBS_COUNT()));
	return 1;
}	

/**
 * @brief
 * write a checkpoint to header of segment, write another page and stop. 
 * now we treat the situation as if we rebooted.
 * test if pending VOTs is set to 0 as expected,
 * and that the checkpoint is as expected
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest2(){
	// set logical address at the beginning of a segment
	uint32_t i, seg_num = 1, slot_id = 1, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = SEQ_NO_SEGMENT_NUM,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = 2,
			 fs_data_size, page_offset = 1, new_page_offset, data_blk_offset;
	uint32_t cp_page_idx_max, cp_size_in_pages;			
	bool_t pending_VOTs, cpWritten;
	page_area_flags *page_header = (page_area_flags*)(sequencing_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t cp_size;			
	uint32_t old_count = 100;
	uint32_t obs_level;
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_1;
#else
	obs_level = 5;
#endif	
	if(SEQ_CHECKPOINT_SIZE > NAND_PAGE_SIZE){
		fs_data_size = NAND_PAGE_SIZE;
	}
	else{
		fs_data_size = NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200;
	}
	uint8_t fs_data[fs_data_size], test_byte = 'a';	
	cp_size_in_pages = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+1;
	cp_size          = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+SEQ_SEG_HEADER_PAGES_COUNT;
	cp_page_idx_max  = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size);
	init_logical_address(log_address);	
	init_logical_address(prev_log_addr);	
	
	// init segment map with various details
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	// init obs map	
	set_obs_counters(obs_level);	
	SET_OBS_COUNT(old_count);
	
	/* set fs_data*/
	fsMemset(fs_data, test_byte, fs_data_size);	
//	PRINT_MSG_AND_NUM("\n1. slot of SEQ_FIRST_RESERVE_SEGMENT =",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,SEQ_FIRST_RESERVE_SEGMENT));
	/* write header before commiting*/
	init_buf(sequencing_buffer);
	SET_HEADER_SEGMENT_ID(page_header, seg_num);
	SET_HEADER_SEQUENCE_NUM(page_header, sequence_num);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);
	VERIFY(!nandProgramTotalPage(sequencing_buffer,CALC_ADDRESS(slot_id,0,0)));
	init_buf(sequencing_buffer);
				
	// write with no file system data (for now)
	VERIFY(!commit(fs_data, fs_data_size,1));
	
//	PRINT_MSG_AND_NUM("\n2. after commit slot of SEQ_FIRST_RESERVE_SEGMENT =",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,SEQ_FIRST_RESERVE_SEGMENT));	
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_page_idx_max+1));	
//	PRINT("\ncommit success");	
	
	init_buf(sequencing_buffer);
	/* set sequencing buffer, and write it to flash*/
	fsMemset(sequencing_buffer, test_byte, NAND_PAGE_SIZE);	
	VERIFY(!allocAndWriteBlock(log_address, sequencing_buffer, VOTS_FLAG_FALSE, prev_log_addr, &cpWritten, checkpointWriter,0));
	VERIFY(!IS_ADDRESS_EMPTY(log_address));	
		
	data_blk_offset = GET_LOGICAL_OFFSET(log_address);	
	
	/* save new page offset*/		
	new_page_offset = GET_RECLAIMED_OFFSET();
//	PRINT_MSG_AND_NUM("\nafter alloc rec offset=",new_page_offset );	
	
	/********* REBOOT!****************/
	mock_reboot();	
	simpleRemarkReserveSegments();
	/* init fs_data*/
	fsMemset(fs_data, 0xff, fs_data_size);	
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 new_page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	
//	PRINT_MSG_AND_NUM("\n after reboot rec offset ", GET_RECLAIMED_OFFSET());
	new_page_offset = GET_RECLAIMED_OFFSET();	
	/* file system and sequencing data is initialized. 
	 * find the checkpoint and verify we read everything properly */	
	VERIFY(!findCheckpointAndTruncate(fs_data, fs_data_size, &pending_VOTs));
//	PRINT("\nsuccess findCheckpointAndTruncate");
	/* verify no pending VOTS*/
	VERIFY(pending_VOTs);
//	PRINT("\nsuccess pending_VOTs ");
	/* verify page offset is correct*/
	VERIFY(COMPARE(cp_size, GET_RECLAIMED_OFFSET()));
	
	/* compare expected data in fs_data*/
	for(i=0; i< fs_data_size; i++){		
		VERIFY(COMPARE(fs_data[i], test_byte));
	}
//	PRINT("\nsuccess fs_data ");
	
	/* compare written and read obs map */
	for(i=0; i<SEQ_OBS_COUNTERS; i++){				
		VERIFY(COMPARE(obs_level, get_counter_level(i)));
	}
//	PRINT("\nsuccess obs map ");
	/* verify page after checkpoint is empty as expected*/
	SET_LOGICAL_OFFSET(log_address, data_blk_offset);
	VERIFY(verifyPageIsEmpty(log_address));
//	PRINT("\n success  log_address");
	VERIFY(!isThereCopyBackEU());
	VERIFY(COMPARE(old_count, GET_OBS_COUNT()));
	return 1;
}	

/**
 * @brief
 * write a checkpoint to header of segment, write (pages per EU) * 2 pages. 
 * now we treat the situation as if we rebooted. all EU's and pages after checkpoint should be empty
 * test if pending VOTs is set to 0 as expected,
 * and that the checkpoint is as expected
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest3(){
	/* set logical address at the beginning of a segment*/
	uint32_t i, j, seg_num = 1, slot_id = 1, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = SEQ_NO_SEGMENT_NUM,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = 2,
			 fs_data_size, page_offset = 1, new_page_offset, data_blk_offset;
	uint32_t cp_size_in_pages;
	bool_t pending_VOTs, cpWritten;
	uint32_t cp_size;			
	uint32_t old_count = 100;
	uint32_t obs_level;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	init_logical_address(log_address);	
	init_logical_address(prev_log_addr);	
	
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_1;
#else
	obs_level = 5;
#endif	
	if(SEQ_CHECKPOINT_SIZE > NAND_PAGE_SIZE){
		fs_data_size = NAND_PAGE_SIZE;
	}
	else{
		fs_data_size = NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200;
	}
	uint8_t fs_data[fs_data_size], test_byte = 'a';	
	cp_size_in_pages = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+1;
	cp_size          = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+SEQ_SEG_HEADER_PAGES_COUNT;
	/* init segment map with various details*/
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	// init obs map	
	set_obs_counters(obs_level);	
	SET_OBS_COUNT(old_count);
	/* set fs_data*/
	fsMemset(fs_data, test_byte, fs_data_size);	
				
	/* write checkpoint*/
	if(commit(fs_data, fs_data_size,1))
		return 0;
	
	/* set sequencing buffer, and write it to flash*/
	fsMemset(sequencing_buffer, test_byte, NAND_PAGE_SIZE);	
	
	for(i=0; i < NAND_PAGES_PER_ERASE_UNIT * 2; i++){
		VERIFY(!allocAndWriteBlock(log_address, sequencing_buffer, VOTS_FLAG_FALSE, prev_log_addr, &cpWritten, checkpointWriter,0));
	}	
	data_blk_offset = GET_LOGICAL_OFFSET(log_address);	
	
	/* save new page offset*/		
	new_page_offset = GET_RECLAIMED_OFFSET();
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncateTest2() - new_page_offset ",new_page_offset );	

	/********* REBOOT!****************/
	mock_reboot();
	simpleRemarkReserveSegments();	
	/* init fs_data*/
	fsMemset(fs_data, 0xff, fs_data_size);	
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 new_page_offset,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);	
	
	new_page_offset = GET_RECLAIMED_OFFSET();	
	/* file system and sequencing data is initialized. find the checkpoint and verify we read everything properly */	
	VERIFY(!findCheckpointAndTruncate(fs_data, fs_data_size, &pending_VOTs));
	
	/* verify page offset is correct*/
	VERIFY(COMPARE(cp_size, GET_RECLAIMED_OFFSET()));
	
	/* verify no pending VOTS*/
	VERIFY(pending_VOTs);
//	print(uart0SendByte,0,"\nsuccess pending_VOTs ");
	
	/* compare expected data in fs_data*/
	for(i=0; i< fs_data_size; i++){		
		VERIFY(COMPARE(fs_data[i], test_byte));
	}
//	print(uart0SendByte,0,"\nsuccess fs_data ");
//	/* compare written and current segment map */
//	VERIFY(verify_seg_map(test_seg_map_ptr, 
//				   sequence_num,
//				   new_slot_id,
//				   previously_written_segment,
//				   is_eu_reserve,
//				   nSegments));  
//
//	/* verify page offset is correct*/
//	VERIFY(COMPARE(page_offset+cp_page_idx_max, SEQ_SEG_MAP_RECLAIMED_ADDR_PTR(test_seg_map_ptr)->page_offset));
//	
//	/* verify the only segment we allocated*/ 
//	VERIFY(COMPARE(test_seg_map_ptr->seg_to_slot_map[seg_num], slot_id));		
//	for(i=0; i<SEQ_SEGMENTS_COUNT ;i++){
//		if(i!=seg_num){			
//			VERIFY(COMPARE(test_seg_map_ptr->seg_to_slot_map[i], SEQ_NO_SLOT));		
//		}
//	}

	/* compare written and read obs map */
	for(i=0; i<SEQ_OBS_COUNTERS; i++){				
		VERIFY(COMPARE(obs_level, get_counter_level(i)));
	}
	
	/* verify pages after checkpoint are empty as expected*/
	for(j=cp_size_in_pages+1; j<= data_blk_offset; j++){
		SET_LOGICAL_OFFSET(log_address, j);		
//		PRINT_MSG_AND_NUM("\npage offset=",log_address->page_offset);
		VERIFY(verifyPageIsEmpty(log_address));		
	}
	
	VERIFY(!isThereCopyBackEU());
	VERIFY(COMPARE(old_count, GET_OBS_COUNT()));
	return 1;
}	

/**
 * @brief
 * test segment truncation.
 * write a checkpoint to header of segment, write (segment size) pages after it, so we move to the next segment, and there
 * only page 0 (header) is written. 
 * now we treat the situation as if we rebooted. all EU's and pages after checkpoint should be empty
 * test if pending VOTs is set to 0 as expected,
 * and that the checkpoint is as expected
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest4(){
	/* set logical address at the beginning of a segment*/
	uint32_t i, j, seg_num = 1, slot_id = 1, seg_num1 = 2, slot_id1 = 2, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = 20,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = 2,
			 fs_data_size, page_offset = 1, new_page_offset;
	uint32_t cp_size_in_pages;
	uint32_t cp_page_idx_max;			
	bool_t pending_VOTs, cpWritten;
	uint32_t cp_size;			
	uint32_t old_count = 100;
	uint32_t obs_level;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	uint8_t buf[NAND_TOTAL_SIZE];
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_1;
#else
	obs_level = 5;
#endif	
	if(SEQ_CHECKPOINT_SIZE > NAND_PAGE_SIZE){
		fs_data_size = NAND_PAGE_SIZE;
	}
	else{
		fs_data_size = NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200;
	}
	uint8_t fs_data[fs_data_size], test_byte = 'a';	
	cp_size_in_pages = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+1;
	cp_size          = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+SEQ_SEG_HEADER_PAGES_COUNT;
	cp_page_idx_max  = cp_size_in_pages -1;
	
	init_logical_address(log_address);	
	init_logical_address(prev_log_addr);
//	PRINT_MSG_AND_NUM("\ncp_size_in_pages=",cp_size_in_pages);
//	PRINT_MSG_AND_NUM("\nSEQ_CHECKPOINT_SIZE=",SEQ_CHECKPOINT_SIZE);
//	PRINT_MSG_AND_NUM("\nfs_data_size=",fs_data_size);
	
	/* init segment map with various details*/
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	/* init obs map */	
	set_obs_counters(obs_level);	
	SET_OBS_COUNT(old_count);
	
	/* set fs_data*/
	fsMemset(fs_data, test_byte, fs_data_size);	
				
	/* write checkpoint*/
//	PRINT_MSG_AND_NUM("\nb4 writing cp offset=", GET_RECLAIMED_OFFSET());
	VERIFY(!(commit(fs_data, fs_data_size, 1)));
//	PRINT_MSG_AND_NUM("\nafter writing cp offset=", GET_RECLAIMED_OFFSET());
//	return 0;
	
	/* set sequencing buffer, and write it to flash*/
	fsMemset(buf, test_byte, NAND_PAGE_SIZE);	
	
	/* write pages until end of segment*/
//	PRINT_MSG_AND_NUM("\nmax offset=", SEQ_PAGES_PER_SLOT-cp_size_in_pages-1);
	for(i=0; i < SEQ_PAGES_PER_SLOT-cp_size_in_pages-1; i++){		
		/* set sequencing buffer, and write it to flash*/
		init_buf(buf);
		fsMemset(buf, test_byte, NAND_PAGE_SIZE);	
		VERIFY(!allocAndWriteBlock(log_address, buf, VOTS_FLAG_FALSE, prev_log_addr, &cpWritten, checkpointWriter,0));
//		PRINT_MSG_AND_NUM("\nrec addr offset=", GET_RECLAIMED_OFFSET());
//		PRINT_MSG_AND_NUM(" max offset=", SEQ_PAGES_PER_SLOT-cp_size_in_pages-1);		
//		PRINT_MSG_AND_NUM(" i=", i);
//		PRINT_MSG_AND_NUM(" i < SEQ_PAGES_PER_SLOT-cp_size_in_pages-1=", i < SEQ_PAGES_PER_SLOT-cp_size_in_pages-1);
	}	
//	PRINT_MSG_AND_NUM("\nfinished. i=", i);
//	PRINT_MSG_AND_NUM(" i < SEQ_PAGES_PER_SLOT-cp_size_in_pages-1=", i < SEQ_PAGES_PER_SLOT-cp_size_in_pages-1);
//	
//	PRINT_MSG_AND_NUM("\nafter allocations reclaimed offset=",GET_RECLAIMED_OFFSET());
	/* mark next slot*/
	seg_map_ptr->seg_to_slot_map[seg_num1] = slot_id1;
	
	/* write mock header*/
	SET_RECLAIMED_SEGMENT(seg_num1);
	SET_RECLAIMED_OFFSET(0);
	seg_map_ptr->new_slot_id = slot_id1;
	seg_map_ptr->nSegments += 1;
	
	VERIFY(!nandProgramTotalPage(buf, CALC_ADDRESS(slot_id1,0,0)));
		
//	PRINT("\nfindCheckpointAndTruncateTest4() - about to reboot");	

	/********* REBOOT!****************/
	mock_reboot();	
	simpleRemarkReserveSegments();
	/* init fs_data*/
	fsMemset(fs_data, 0xff, fs_data_size);	
	
	set_seg_map(seg_map_ptr,
				 slot_id1,
				 1, // notice-  page offset is last page written!
				 seg_num1,
				 sequence_num,				 
				 new_slot_id,
				 seg_num, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);	
	
	/* mark previous slot*/
	seg_map_ptr->seg_to_slot_map[seg_num] = slot_id;
	
	new_page_offset = GET_RECLAIMED_OFFSET();	
//	PRINT("\nabout to findCheckpointAndTruncate()");
	/* file system and sequencing data is initialized. find the checkpoint and verify we read everything properly */	
	VERIFY(!findCheckpointAndTruncate(fs_data, fs_data_size, &pending_VOTs));
//	PRINT("\n findCheckpointAndTruncate success");
	/* verify page offset is correct*/
	VERIFY(COMPARE(cp_size, GET_RECLAIMED_OFFSET()));
//	PRINT("\nsuccess cp_size ");
	/* verify no pending VOTS (we are at the very first checkpoint. can't be VOTs*/
	VERIFY(!pending_VOTs);
//	PRINT("\nsuccess pending_VOTs ");
	
	/* compare expected data in fs_data*/
	for(i=0; i< fs_data_size; i++){		
		VERIFY(COMPARE(fs_data[i], test_byte));
	}
//	PRINT("\nsuccess fs_data ");
//	/* compare written and current segment map */
//	VERIFY(verify_seg_map(test_seg_map_ptr, 
//				   sequence_num,
//				   slot_id,
//				   previously_written_segment,
//				   is_eu_reserve,
//				   nSegments-1));  
//
	/* verify page offset is correct*/
//	PRINT_MSG_AND_NUM("\npage_offset+cp_page_idx_max=", page_offset+cp_page_idx_max);
//	PRINT_MSG_AND_NUM("\nGET_RECLAIMED_OFFSET()=", GET_RECLAIMED_OFFSET());
	VERIFY(COMPARE(page_offset+cp_page_idx_max, GET_RECLAIMED_OFFSET()));
//	PRINT("\nsuccess page offset ");
				
//	/* verify the only segment we allocated*/ 
	VERIFY(COMPARE(seg_map_ptr->seg_to_slot_map[seg_num], slot_id));		
//	for(i=0; i<SEQ_SEGMENTS_COUNT ;i++){
//		if(i!=seg_num){			
//			VERIFY(COMPARE(test_seg_map_ptr->seg_to_slot_map[i], SEQ_NO_SLOT));		
//		}
//	}

	/* compare written and read obs map */
	for(i=0; i<SEQ_OBS_COUNTERS; i++){				
		VERIFY(COMPARE(obs_level, get_counter_level(i)));
	}
//	PRINT("\nsuccess obs_level");
	
	SET_LOGICAL_SEGMENT(log_address, seg_num);
	/* verify pages after checkpoint are empty as expected*/
	for(j=cp_size_in_pages+1; j< SEQ_PAGES_PER_SLOT; j++){
		SET_LOGICAL_OFFSET(log_address, j);		
//		PRINT_MSG_AND_NUM("\npage offset=",GET_LOGICAL_OFFSET(log_address));
		VERIFY(verifyPageIsEmpty(log_address));		
	}
//	PRINT("\nsuccess pages after checkpoint are empty");
	
	VERIFY(!isThereCopyBackEU());
	VERIFY(COMPARE(old_count, GET_OBS_COUNT()));
	return 1;
}	

/**
 * @brief
 * write a checkpoint to header of segment and stop. then try to find the checkpoint
 * test checkpoint was read properly, that pending VOTs is set as expected.
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest5(){
	/* set logical address at the beginning of a segment*/
	uint32_t seg_num = 0, slot_id = 0, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = SEQ_NO_SEGMENT_NUM,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = 1,
			 fs_data_size, page_offset = 1, new_page_offset;				
	bool_t pending_VOTs, cpWrittenDuringAllocation;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);	
	uint8_t obs_level;
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_1;
#else
	obs_level = 5;
#endif	
	if(SEQ_CHECKPOINT_SIZE > NAND_PAGE_SIZE){
		fs_data_size = NAND_PAGE_SIZE;
	}
	else{
		fs_data_size = NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200;
	}
	uint8_t fs_data[fs_data_size], test_byte = 'a';	
	
	init_logical_address(log_address);
	init_logical_address(prev_log_addr);
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncateTest5() - fs_data_size=",fs_data_size);
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncateTest5() - SEQ_CHECKPOINT_SIZE=",SEQ_CHECKPOINT_SIZE);
		
	/* init segment map with various details*/
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	/* init obs map*/
	set_obs_counters(obs_level);	
			
	/* set fs_data*/
	fsMemset(fs_data, test_byte, fs_data_size);	
					
	/* write mock checkpoint first page */
	
	SET_CHECKPOINT_FLAG(flags, CP_LOCATION_FIRST);
	/* write it to flash*/	
	VERIFY(!allocAndWriteBlock(log_address, sequencing_buffer, 0, prev_log_addr, &cpWrittenDuringAllocation, checkpointWriter,1));
//	PRINT("\nallocAndWriteBlock() success");	
				
	/* save new page offset*/		
	new_page_offset = GET_RECLAIMED_OFFSET();

	/********* REBOOT *********/
	mock_reboot();
	simpleRemarkReserveSegments();
//	PRINT("\nmock reboot");	
	/* init fs_data*/
	fsMemset(fs_data, 0xff, fs_data_size);	
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 new_page_offset, 
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);	
		
	/* file system and sequencing data is initialized. try to find the checkpoint and verify none is found */	
	VERIFY(COMPARE(findCheckpointAndTruncate(fs_data, fs_data_size, &pending_VOTs),ERROR_NO_CP));
//	PRINT("\nfindCheckpointAndTruncate() success");
	VERIFY(!isThereCopyBackEU());	
	return 1;
}	

/**
 * @brief
 * test segment truncation.
 * - write a checkpoint to header of segment
 * - write k pages.
 * - write another checkpoint
 * - write til the end of segment
 * - write header of next segment
 * now we treat the situation as if we rebooted. all EU's and pages after checkpoint should be empty
 * test if pending VOTs is set to 0 as expected,
 * and that the checkpoint is as expected
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest6(){
	/* set logical address at the beginning of a segment*/
	uint32_t k=6, i, j, seg_num = 0, slot_id = 0, seg_num1 = 1, slot_id1 = 1, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = 20,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = 2,
			 fs_data_size, page_offset = 1, new_page_offset;
	uint32_t cp_size_in_pages;
	bool_t pending_VOTs, cpWritten;
	uint32_t cp_size;			
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	uint32_t old_count = 100;	
	uint8_t obs_level, buf[NAND_TOTAL_SIZE];
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_1;
#else
	obs_level = 5;
#endif	
	if(SEQ_CHECKPOINT_SIZE > NAND_PAGE_SIZE){
		fs_data_size = NAND_PAGE_SIZE;
	}
	else{
		fs_data_size = NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200;
	}
	uint8_t fs_data[fs_data_size], test_byte = 'a';	
	cp_size_in_pages = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+1;
	cp_size          = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + fs_data_size)+SEQ_SEG_HEADER_PAGES_COUNT;
	init_logical_address(log_address);	
	init_logical_address(prev_log_addr);
		
	/* init segment map with various details*/
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	/* init obs map	*/
	set_obs_counters(obs_level);
	SET_OBS_COUNT(old_count);
			
	/* set fs_data*/
	fsMemset(fs_data, test_byte, fs_data_size);					
	/* write checkpoint*/
	VERIFY(!commit(fs_data, fs_data_size,1));
		
//	PRINT_MSG_AND_NUM("\nafter commit the next page is in offset ",GET_RECLAIMED_OFFSET());
	/* set sequencing buffer, and write it to flash*/
	init_buf(sequencing_buffer);
	fsMemset(sequencing_buffer, test_byte, NAND_PAGE_SIZE);		
	
	/* write k pages */	
	for(i=0; i<k; i++){
		init_buf(buf);
		fsMemset(buf, test_byte, NAND_PAGE_SIZE);	
		VERIFY(!allocAndWriteBlock(log_address, buf, VOTS_FLAG_FALSE, prev_log_addr, &cpWritten, checkpointWriter,0));		
	}
	
	/* write checkpoint again*/
	VERIFY(!commit(fs_data, fs_data_size,0));
//	PRINT_MSG_AND_NUM("\nafter 2nd commit rec offset=",GET_RECLAIMED_OFFSET() );		
//	PRINT_MSG_AND_NUM("\nafter 2nd commit rec segment=",GET_RECLAIMED_SEGMENT() );
	
	/* write pages until end of segment*/
	while( GET_RECLAIMED_OFFSET() < SEQ_PAGES_PER_SLOT-2 * cp_size_in_pages-1){
		init_buf(buf);
		fsMemset(buf, test_byte, NAND_PAGE_SIZE);	
		VERIFY(!allocAndWriteBlock(log_address, buf, VOTS_FLAG_FALSE, prev_log_addr, &cpWritten, checkpointWriter,0));		
	}	
//	PRINT_MSG_AND_NUM("\nafter more allocs rec offset=",GET_RECLAIMED_OFFSET() );
	
	/* mark next slot*/
	seg_map_ptr->seg_to_slot_map[seg_num1] = slot_id1;
	
	/* write mock header*/
	SET_RECLAIMED_SEGMENT(seg_num1);
	SET_RECLAIMED_OFFSET(0);
	seg_map_ptr->new_slot_id = slot_id1;
	seg_map_ptr->nSegments += 1;
	
	init_buf(sequencing_buffer);
	fsMemset(sequencing_buffer, test_byte, NAND_PAGE_SIZE);	
	VERIFY(!allocAndWriteBlock(log_address, sequencing_buffer, VOTS_FLAG_FALSE, prev_log_addr, &cpWritten, checkpointWriter,0));
	
	/* save new page offset*/		
	new_page_offset = GET_RECLAIMED_OFFSET();
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncateTest6() - new_page_offset ",new_page_offset );	

	/********* REBOOT!****************/
	mock_reboot();
	simpleRemarkReserveSegments();	
	/* init fs_data*/
	fsMemset(fs_data, 0xff, fs_data_size);		
	
	set_seg_map(seg_map_ptr,
				 slot_id1,
				 new_page_offset, 
				 seg_num1,
				 sequence_num,				 
				 new_slot_id,
				 seg_num, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);	
	
	/* mark previous slot*/
	seg_map_ptr->seg_to_slot_map[seg_num] = slot_id;
	
	new_page_offset = GET_RECLAIMED_OFFSET();
		
//	PRINT("\nabout to findCheckpointAndTruncate()");
	/* file system and sequencing data is initialized. find the checkpoint and verify we read everything properly */	
	VERIFY(!findCheckpointAndTruncate(fs_data, fs_data_size, &pending_VOTs));
//	PRINT("\nsuccess findCheckpointAndTruncate()");
//	PRINT_MSG_AND_NUM("\nrec offset=",GET_RECLAIMED_OFFSET() );	
//	PRINT_MSG_AND_NUM("\nrec segment=",GET_RECLAIMED_SEGMENT() );
	/* verify page offset is correct*/
	VERIFY(!COMPARE(cp_size, GET_RECLAIMED_OFFSET()));
	
//	PRINT("\nsuccess rec offset");
	/* verify no pending VOTS*/
	VERIFY(!pending_VOTs);
//	PRINT("\nsuccess pending_VOTs ");
	
	/* compare expected data in fs_data*/
	for(i=0; i< fs_data_size; i++){		
		VERIFY(COMPARE(fs_data[i], test_byte));
	}	
//	PRINT("\nsuccess seg map ");
	
	/* compare written and read obs map */
	for(i=0; i<SEQ_OBS_COUNTERS; i++){				
		VERIFY(COMPARE(obs_level, get_counter_level(i)));
	}
	
	/* verify pages after checkpoint are empty as expected*/
	SET_LOGICAL_SEGMENT(log_address, seg_num);
	for(j=cp_size_in_pages*2+k+1; j< SEQ_PAGES_PER_SLOT; j++){
		SET_LOGICAL_OFFSET(log_address, j);		
//		PRINT_MSG_AND_NUM("\npage offset=",log_address->page_offset);
		VERIFY(verifyPageIsEmpty(log_address));		
	}
	
//	print(uart0SendByte,0,"\nsuccess obs map ");
	VERIFY(!isThereCopyBackEU());
	VERIFY(COMPARE(old_count, GET_OBS_COUNT()));
	return 1;
}	

/**
 * @brief
 * write a checkpoint to header of segment in reclamation and stop. then try to find the checkpoint.
 * should return to reclamation state.
 * test checkpoint was read properly, that pending VOTs is set as expected.
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest7(){
	// set logical address at the beginning of a segment
	uint32_t i, seg_num = 0, slot_id = 0, sequence_num = 1, new_slot_id = slot_id, 
			 previously_written_segment = 20,is_eu_reserve = IS_EU_RESERVE_FALSE, 
			 seg_to_reclaim = 0, page_offset = 1, new_page_offset;	
	uint32_t cp_size;
	uint8_t test_byte = 'a', obs_level, buf[NAND_TOTAL_SIZE];	
	bool_t pending_VOTs, cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	uint32_t old_count = 100;	
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_2;
#else
	obs_level = 5;
#endif	
	cp_size          = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	init_logical_address(log_addr);
	init_logical_address(prev_addr);
	init_logical_address(log_address);
	
	// init segment map with various details
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 1,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	/* init obs map*/	
	set_obs_counters(obs_level);	
					
	// commit	
	VERIFY(!commit(fs_data, MOCK_FS_DATA_SIZE,1));
//	PRINT_MSG_AND_NUM("\n after commit() GET_RECLAIMED_OFFSET()=", GET_RECLAIMED_OFFSET());	
	/* save new page offset*/	
			
	new_page_offset = GET_RECLAIMED_OFFSET();
	
	/* mark obs counters levels*/
	set_obs_counters(OBS_COUNT_LEVEL_1);
	
//	PRINT("\nset obs counters success");
//	PRINT_MSG_AND_NUM("\nSEQ_PAGES_PER_SLOT=", SEQ_PAGES_PER_SLOT);
//	PRINT_MSG_AND_NUM("\nNAND_PAGE_COUNT=", NAND_PAGE_COUNT);
//	PRINT_MSG_AND_NUM("\nSEQ_N_SLOTS_COUNT=", SEQ_N_SLOTS_COUNT);
	
	fsMemset(buf,test_byte,NAND_PAGE_SIZE);
	/* write SEQ_PAGES_PER_SLOT pages*/ 
	for(i=0;i < SEQ_PAGES_PER_SLOT*2;i++){		 
		init_buf(buf);		
		VERIFY(!(allocAndWriteBlock(log_addr, buf, 0, prev_addr, &cpWritten, checkpointWriter,0)));
//		PRINT_MSG_AND_NUM("\nalloc success rec offset=",GET_RECLAIMED_OFFSET());
//		PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());	
	}
//	PRINT("\nallocs success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());	
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());	
	
	/* write headers for all segments*/
	VERIFY(!writeSimpleSegmentHeaders(3));
//	PRINT("\nwrite headers success");
	
	/* write header of segment without checkpoint*/
	init_buf(sequencing_buffer);
			
//	/* mark pages as obsolete in all segments 1*/
//	SET_LOGICAL_OFFSET(log_addr, obs_page);
//	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
////		PRINT_MSG_AND_NUM("\nmark offset 6 as obs in seg=", i);
////		PRINT_MSG_AND_NUM(" slot=", GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, i));
//		SET_LOGICAL_SEGMENT(log_addr, i);		
//		markAsObsolete(log_addr);		
////		PRINT(". done");
//	}
//	PRINT("\nmark as obsolete success")
	
	/* reclaim segment */	
	SET_HEADER_SEGMENT_ID(page_hdr_ptr,seg_to_reclaim);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);	
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));		
	VERIFY(!nandProgramTotalPage(sequencing_buffer, CALC_ADDRESS(SEQ_SEGMENTS_COUNT,0,0)));
	SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, SEQ_SEGMENTS_COUNT);	
	SET_RECLAIMED_SEGMENT(seg_to_reclaim);
	SET_RECLAIMED_OFFSET(1);	
//	PRINT("\nabout to commit mock header cp");	
//	PRINT_MSG_AND_NUM(" is state rec?=",IS_STATE_RECLAMATION());	
//	PRINT_MSG_AND_NUM(" nsegments=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());	
//	PRINT_MSG_AND_NUM(" rec seg slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" new slot id=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	
//	PRINT_MSG_AND_NUM(" SEQ_SEGMENTS_COUNT=",SEQ_SEGMENTS_COUNT);

	/* write checkpoint */
	VERIFY(!commit(fs_data, MOCK_FS_DATA_SIZE, 1));
	sequence_num = GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	new_page_offset = GET_RECLAIMED_OFFSET();	
//	PRINT("\nobs markings success");
	old_count = GET_OBS_COUNT(); 
	
	/* reboot*/
	mock_reboot();
	
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncateTest7() - b4 reboot rec offset=",new_page_offset );	
	/********* REBOOT!****************/
	mock_reboot();
	simpleRemarkReserveSegments();		
	
	set_seg_map(seg_map_ptr,
				 0,
				 new_page_offset, 
				 seg_to_reclaim,
				 sequence_num,				 
				 SEQ_SEGMENTS_COUNT,
				 SEQ_SEGMENTS_COUNT-1,
				 is_eu_reserve,
				 SEQ_SEGMENTS_COUNT,
				 SEQ_PHY_ADDRESS_EMPTY);	
	
	new_page_offset = GET_RECLAIMED_OFFSET();	
	/* file system and sequencing data is initialized. find the checkpoint and verify we read everything properly */		
	VERIFY(!findCheckpointAndTruncate(fs_data, MOCK_FS_DATA_SIZE, &pending_VOTs));
//	PRINT("\nsuccess findCheckpointAndTruncate() ");
	
	/* verify pending VOTS */
	VERIFY(pending_VOTs);
//	PRINT("\nsuccess pending_VOTs ");
	
//	/* compare expected data in fs_data*/
//	for(i=0; i< MOCK_FS_DATA_SIZE; i++){		
//		VERIFY(COMPARE(fs_data[i], test_byte));
//	}
//	PRINT("\nsuccess fs data ");

	/* compare written and current segment map */
	VERIFY(verify_seg_map(seg_map_ptr, 
				   sequence_num,
				   SEQ_SEGMENTS_COUNT,
				   SEQ_SEGMENTS_COUNT-1,
				   is_eu_reserve,
				   SEQ_SEGMENTS_COUNT));
				   
//	PRINT("\nsuccess seg map ");
	
//	PRINT("\nabout check rec state. ");	
//	PRINT_MSG_AND_NUM(" is state rec?=",IS_STATE_RECLAMATION());	
//	PRINT_MSG_AND_NUM(" nsegments=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());	
//	PRINT_MSG_AND_NUM(" rec seg slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" new slot id=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));			     
	VERIFY(IS_STATE_RECLAMATION());
//	PRINT("\nsuccess reclaim state ");		
	
	/* verify page offset is correct*/
	VERIFY(COMPARE(cp_size, GET_RECLAIMED_OFFSET()));
//	PRINT("\nsuccess rec offset");	 
	
	/* compare written and read checkpoint */
	verify_obs_counters(obs_level);
//	PRINT("\nsuccess obs counters ");
	
	VERIFY(!isThereCopyBackEU());
	return 1;
}	

/**
 * @brief
 * write only first page of header of segment in reclamation and stop. then try to find the checkpoint
 * test checkpoint was read properly, and that pending VOTs is set as expected.
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest8(){
	/* set logical address at the beginning of a segment */
	uint32_t i, seg_num = 0, slot_id = 0, sequence_num = 1, new_slot_id = slot_id, 
			 previously_written_segment = 20,is_eu_reserve = IS_EU_RESERVE_FALSE, 
			 seg_to_reclaim = 0, page_offset = 1, new_page_offset;	
	uint32_t cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE + MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	uint8_t test_byte = 'a', obs_level, buf[NAND_TOTAL_SIZE];	
	bool_t pending_VOTs, cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	uint32_t old_count = 100;		
	
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_2;
#else
	obs_level = 5;
#endif	
	init_logical_address(log_addr);
	init_logical_address(prev_addr);
	init_logical_address(log_address);
	
	/* init segment map with various details */
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 1,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	/* init obs map*/	
	set_obs_counters(obs_level);	
	
	old_count = GET_OBS_COUNT();				
	/* commit */	
	VERIFY(!commit(fs_data, MOCK_FS_DATA_SIZE,1));
//	PRINT_MSG_AND_NUM("\n after commit() GET_RECLAIMED_OFFSET()=", GET_RECLAIMED_OFFSET());	
	
	/* save new page offset*/			
	new_page_offset = GET_RECLAIMED_OFFSET();
	
	/* mark obs counters levels*/
	set_obs_counters(OBS_COUNT_LEVEL_1);
//	PRINT("\nset obs counters success");
	
	/* write SEQ_PAGES_PER_SLOT pages*/ 
	for(i=0;i < SEQ_PAGES_PER_SLOT*2;i++){
		init_buf(buf);
		fsMemset(buf,test_byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, buf, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}
//	PRINT("\nallocs success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());	
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());		
//	PRINT_MSG_AND_NUM(" rec address=",logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
		
	/* write headers for all segments*/
	VERIFY(!writeSimpleSegmentHeaders(3));
	sequence_num = GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	
	/* write header of segment without checkpoint*/
	init_buf(sequencing_buffer);
	
	/* reclaim segment */	
	SET_HEADER_SEGMENT_ID(page_hdr_ptr,seg_to_reclaim);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);	
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));		
	VERIFY(!nandProgramTotalPage(sequencing_buffer, CALC_ADDRESS(SEQ_SEGMENTS_COUNT,0,0)));
	SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, SEQ_SEGMENTS_COUNT);	
	SET_RECLAIMED_SEGMENT(seg_to_reclaim);
	SET_RECLAIMED_OFFSET(1);	
//	PRINT("\nabout to commit mock header cp");	
//	PRINT_MSG_AND_NUM(" is state rec?=",IS_STATE_RECLAMATION());	
//	PRINT_MSG_AND_NUM(" nsegments=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());	
//	PRINT_MSG_AND_NUM(" rec seg slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" new slot id=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	
//	PRINT_MSG_AND_NUM(" SEQ_SEGMENTS_COUNT=",SEQ_SEGMENTS_COUNT);
//	assert(0);
	/* write checkpoint */	
	new_page_offset = GET_RECLAIMED_OFFSET();	
//	PRINT("\nobs markings success");	
	
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncateTest8() - b4 reboot rec offset=",new_page_offset );	
	/********* REBOOT!****************/
	mock_reboot();
	simpleRemarkReserveSegments();		
	
	for(i=0;i<SEQ_SEGMENTS_COUNT;i++){
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,i,i);
	}
	
//	segment_map *seg_map_ptr,
//				 uint32_t slot_id,
//				 uint32_t page_offset,
//				 uint32_t seg_num,
//				 uint32_t sequence_num,				 
//				 uint32_t new_slot_id,
//				 uint32_t previously_written_segment,
//				 uint32_t is_eu_reserve,
//				 uint32_t nSegments,
//				 uint32_t reserve_eu_addr
	set_seg_map(seg_map_ptr,
				 0,
				 new_page_offset, 
				 seg_to_reclaim,
				 sequence_num,				 
				 SEQ_SEGMENTS_COUNT,
				 2,
				 is_eu_reserve,
				 SEQ_SEGMENTS_COUNT,
				 SEQ_PHY_ADDRESS_EMPTY);	
	
	new_page_offset = GET_RECLAIMED_OFFSET();
	
	/* file system and sequencing data is initialized. find the checkpoint and verify we read everything properly */
//		PRINT("\nabout to findCheckpointAndTruncate()");
	VERIFY(!findCheckpointAndTruncate(fs_data, MOCK_FS_DATA_SIZE, &pending_VOTs));
//	PRINT("\nsuccess findCheckpointAndTruncate() ");
	
	/* verify pending VOTS */
	VERIFY(pending_VOTs);
//	PRINT("\nsuccess pending_VOTs ");
	
//	PRINT("\nabout check rec state. ");	
//	PRINT_MSG_AND_NUM(" is state rec?=",IS_STATE_RECLAMATION());	
//	PRINT_MSG_AND_NUM(" nsegments=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());	
//	PRINT_MSG_AND_NUM(" rec seg slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" new slot id=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));			     
	VERIFY(!IS_STATE_RECLAMATION());
//	PRINT("\nsuccess reclaim state ");		
	
	/* verify page offset is correct*/
	VERIFY(COMPARE(cp_size, GET_RECLAIMED_OFFSET()));
//	PRINT("\nsuccess rec offset");	 
	
	/* compare written and read checkpoint */
	verify_obs_counters(obs_level);
//	PRINT("\nsuccess obs counters ");
	
	VERIFY(!isThereCopyBackEU());
//	PRINT_MSG_AND_NUM("\nold obs count=",old_count);
//	PRINT_MSG_AND_NUM("\nnew obs count=", GET_OBS_COUNT());
	VERIFY(COMPARE(old_count, GET_OBS_COUNT()));
	return 1;
}
