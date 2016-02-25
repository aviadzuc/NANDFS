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

/** @file commitTests.c  */
#include <test/sequencing/commitTests.h>
#include <test/sequencing/testsHeader.h>

/**
 * run all Commit tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllCommitTests(){
	RUN_TEST(commit, 1);
	RUN_TEST(commit, 2);
	RUN_TEST(commit, 3);
	RUN_TEST(commit, 4);
	RUN_TEST(commit, 5);
	
	return 0;
}

/**
 * @brief 
 * init commit test
 */
error_t init_commitTest(){
	if(nandInit())
		return -1;		
		
	init_seg_map();
	init_obs_counters()	;	
	
	init_flash();
	mark_reserve_segment_headers();
//	PRINT("\nfinished init commit");
	return 1;
}									

/**
 * @brief 
 * tear down commit test
 */
error_t tearDown_commitTest(){
	init_seg_map();
	init_obs_counters();
	init_logical_address(&(seg_map_ptr->reclaimed_logical_addr));
	
	init_flash();
	nandTerminate();		
	
	return 1;
}

/**
 * @brief
 * write a checkpoint as part a segment header and verify it was indeed written - 
 * compare expected and read values of segment map
 * assumes that the checkpoint is under one page
 */
error_t commitTest1(){
	/* set logical address at the beginning of a segment*/
	uint32_t phy_addr, i, seg_num = 4, slot_id = 1, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = 20,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments     = 2;
	uint8_t buffer[sizeof(segment_map)+sizeof(obs_pages_per_seg_counters)+MOCK_FS_DATA_SIZE];	
	segment_map *cp_seg_map_ptr = (segment_map*)buffer;
	obs_pages_per_seg_counters *cp_obs_map_ptr = (obs_pages_per_seg_counters*)(buffer+sizeof(segment_map));	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);	
	init_logical_address(log_address);
	uint32_t old_lfsr_state;
	uint8_t obs_level;
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_0;
#else
	obs_level = 5;
#endif
	/* init segment map with various details*/
	seg_map_ptr->seg_to_slot_map[seg_num] = slot_id;	
			
	SET_RECLAIMED_OFFSET(1);
	SET_RECLAIMED_SEGMENT(seg_num);
		
	seg_map_ptr->sequence_num  = sequence_num;
	seg_map_ptr->new_slot_id   = new_slot_id;
	seg_map_ptr->previously_written_segment = previously_written_segment;
	seg_map_ptr->is_eu_reserve = is_eu_reserve;
	seg_map_ptr->nSegments     = nSegments;	
	SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, SEQ_PHY_ADDRESS_EMPTY);
	
	/* init obs map	*/
	set_obs_counters(obs_level);	
		
	/* save expected address before writing */
	SET_LOGICAL_OFFSET(log_address, GET_RECLAIMED_OFFSET());
	SET_LOGICAL_SEGMENT(log_address, GET_RECLAIMED_SEGMENT());
	
//	PRINT_MSG_AND_NUM("\ncp will be written to ", GET_RECLAIMED_OFFSET());
	
	/* save physical address to which the checkpoint will be written*/
	phy_addr = logicalAddressToPhysical(log_address);		
//	PRINT_MSG_AND_NUM("\nread cp from page ", phy_addr);
	
	old_lfsr_state = lfsr_state;
		
	/* write with no file system data (for now)*/
	VERIFY(!(commit(fs_data, 0, 1)));
//	PRINT("\nfinished commit");
	
	VERIFY(COMPARE(old_lfsr_state, lfsr_state));
	/* read checkpoint*/
	read_cp_to_buffers(fs_data, 0, cp_seg_map_ptr, cp_obs_map_ptr);
//	PRINT("\nfinished read_cp_to_buffers");
	
	/* compare written and read checkpoint*/
	for(i=0; i<SEQ_OBS_COUNTERS; i++){		
//		PRINT_MSG_AND_NUM("\n", i);
//		PRINT_MSG_AND_NUM(". counter level = ", get_counter_level_by_map(i,cp_obs_map_ptr));
		VERIFY(COMPARE(get_counter_level_by_map(i,cp_obs_map_ptr), get_counter_level(i)));
	}
//	PRINT("\ndone");	
	return 1;
}

/**
 * @brief
 * write 2 checkpoints,one as part a segment header, and one afterwords with changes.
 * verify the first was indeed written - compare expected and read values of segment map
 * assumes that the checkpoint is under one page
 */
error_t commitTest2(){
	/* set logical address at the beginning of a segment*/
	uint32_t phy_addr, i, seg_num = 4, slot_id = 1, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = 20,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments  = 2,
			 seg_num1 = 16, slot_id1 = 8, sequence_num1 = 644, new_slot_id1 = slot_id, 
			 previously_written_segment1 = 4,is_eu_reserve1 = IS_EU_RESERVE_FALSE, nSegments1 = 3;
	uint8_t buffer[sizeof(segment_map)+sizeof(obs_pages_per_seg_counters)+MOCK_FS_DATA_SIZE];
	segment_map *cp_seg_map_ptr = (segment_map*)buffer;
	obs_pages_per_seg_counters *cp_obs_map_ptr = (obs_pages_per_seg_counters*)(buffer+sizeof(segment_map));
	uint8_t obs_level;
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_1;
#else
	obs_level = 5;
#endif
	/* init segment map with various details*/
	seg_map_ptr->seg_to_slot_map[seg_num] = slot_id;	
	
	/* set segment map addres pointer to the next free address*/		
	SET_RECLAIMED_SEGMENT(seg_num);
	SET_RECLAIMED_OFFSET(1);
		
	set_seg_map(seg_map_ptr,
				slot_id,
				1,
				seg_num,
				sequence_num,
				new_slot_id,
				previously_written_segment,
				is_eu_reserve,
				nSegments,
				SEQ_PHY_ADDRESS_EMPTY);
	
	/* init obs map	*/
	set_obs_counters(obs_level);	
		
	/* write with no file system data (for now)*/
	if(commit(fs_data, 0, 1))
		return 0;
	
	/* write second checkpoint*/
	/* init segment map with various details*/
	seg_map_ptr->seg_to_slot_map[seg_num1] = slot_id1;		
		
	seg_map_ptr->sequence_num  = sequence_num1;
	seg_map_ptr->new_slot_id   = new_slot_id1;	
	seg_map_ptr->is_eu_reserve = is_eu_reserve1;
	seg_map_ptr->nSegments     = nSegments1;
	seg_map_ptr->previously_written_segment = previously_written_segment1;
	
	// save expected address before writing	
	phy_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	
	// write with no file system data (for now)
	VERIFY(!commit(fs_data, 0, 0));
	
	// try reading checkpoint
	read_cp_to_buffers(fs_data, 0, cp_seg_map_ptr, cp_obs_map_ptr);
	//nandReadPage(buffer, phy_addr, 0, NAND_TOTAL_SIZE);
	
	// compare written and current segment map	
//	PRINT_MSG_AND_NUM("\ncp_seg_map_ptr->sequence_num",cp_seg_map_ptr->sequence_num)	;	
//	VERIFY(verify_seg_map(cp_seg_map_ptr,
//						  sequence_num1,
//						  new_slot_id1,
//						  previously_written_segment1,
//						  is_eu_reserve1,
//						  nSegments1));	
//	
//	SEQ_SEG_MAP_RECLAIMED_ADDR_PTR(test_seg_map_ptr)->page_offset -= 1;
//	VERIFY(COMPARE(logicalAddressToPhysical(SEQ_SEG_MAP_RECLAIMED_ADDR_PTR(test_seg_map_ptr)), phy_addr));	
//	SEQ_SEG_MAP_RECLAIMED_ADDR_PTR(test_seg_map_ptr)->page_offset += 1;
//	
//	for(i=0; i<SEQ_N_SLOTS_COUNT ;i++){
//		VERIFY(COMPARE(cp_seg_map_ptr->seg_to_slot_map[i], test_seg_map_ptr->seg_to_slot_map[i]));		
//	}
	
	/* compare written and read obs map*/
	for(i=0; i<SEQ_OBS_COUNTERS; i++){
//		PRINT_MSG_AND_NUM("\ncommitTest2() - counter level = ", get_counter_level_by_map(i,cp_obs_map_ptr));
		VERIFY(COMPARE(get_counter_level_by_map(i,cp_obs_map_ptr), obs_level));
	}
	
	return 1;
}

/**
 * @brief
 * write a checkpoint as part a segment header and verify it was indeed written - 
 * compare expected and read values of segment map
 * checkpoint is more then 1 page now
 */
error_t commitTest3(){
	// set logical address at the beginning of a segment
	uint32_t phy_addr, i, seg_num = 4, slot_id = 0, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = 20,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = 2,
			 fs_data_size = NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200;
	uint8_t fs_data[NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200], test_byte = 'a';	
	segment_map cp_map, *cp_seg_map_ptr = &cp_map;
	obs_pages_per_seg_counters cp_obs_map, *cp_obs_map_ptr = &cp_obs_map;
	uint8_t obs_level;
	#ifndef EXACT_COUNTERS	
		obs_level =OBS_COUNT_LEVEL_1;
	#else
		obs_level = 5;
	#endif
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
	// init segment map with various details
	set_seg_map(seg_map_ptr,
				 slot_id,
				 1,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	// init obs map	
	set_obs_counters(obs_level);	
	
	/* init fs_data*/
	fsMemset(fs_data, test_byte, fs_data_size);	
	
	// save expected address before writing
	copyLogicalAddress(log_address, GET_RECLAIMED_ADDRESS_PTR());	
	
	// save physical address to which the checkpoint will be written
	phy_addr = logicalAddressToPhysical(log_address);		
	
	/* commit*/	
	if(commit(fs_data, fs_data_size, 1))
		return 0;

	read_cp_to_buffers(fs_data, fs_data_size, cp_seg_map_ptr, cp_obs_map_ptr); 	 	
	
	/* compare expected data in fs_data*/
	for(i=0; i< fs_data_size; i++){		
		VERIFY(COMPARE(fs_data[i], test_byte));
	}

	/* compare written and current segment map */
//	VERIFY(COMPARE(cp_seg_map_ptr->sequence_num, test_seg_map_ptr->sequence_num));	
//	VERIFY(COMPARE(cp_seg_map_ptr->new_slot_id, test_seg_map_ptr->new_slot_id));
//	VERIFY(COMPARE(cp_seg_map_ptr->previously_written_segment, test_seg_map_ptr->previously_written_segment));	
//	VERIFY(COMPARE(cp_seg_map_ptr->is_eu_reserve, test_seg_map_ptr->is_eu_reserve));
//	VERIFY(COMPARE(cp_seg_map_ptr->nSegments, test_seg_map_ptr->nSegments));
//
//	for(i=0; i<SEQ_N_SLOTS_COUNT ;i++){
//		VERIFY(COMPARE(cp_seg_map_ptr->seg_to_slot_map[i], test_seg_map_ptr->seg_to_slot_map[i]));		
//	}

	/* compare written and read checkpoint */
	for(i=0; i<SEQ_OBS_COUNTERS; i++){				
		VERIFY(COMPARE(get_counter_level_by_map(i,cp_obs_map_ptr), get_counter_level(i)));
	}
		
	return 1;
}

/**
 * @brief
 * write a checkpoint as part a segment header, the first EU of the segment is bad.
 * verify it was indeed written - compare expected and read values of segment map
 * checkpoint is more then 1 page now
 * @return 1 if successful. if the commit failed 0 is returned.
 * if retrieved checkpoint data is not as expected 0 is returned
 */
error_t commitTest4(){
	// set logical address at the beginning of a segment
	uint32_t reserve_eu_addr, orig_phy_addr, phy_addr, i, seg_num = 4, slot_id = 0, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = 20,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = 2, page_offset = 1,
			 fs_data_size = NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200;
	uint8_t fs_data[NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200], test_byte = 'b';	
	segment_map cp_map, *cp_seg_map_ptr = &cp_map;
	obs_pages_per_seg_counters cp_obs_map, *cp_obs_map_ptr = &cp_obs_map;
	uint8_t obs_level;
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_1;
#else
	obs_level = 5;
#endif
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
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
	
	/* init fs_data*/
	fsMemset(fs_data, test_byte, fs_data_size);	
		
	/* allocate reserve eu for the original address
	 * and indicate in segment map that we are in a reserve segment*/ 	
	orig_phy_addr = CALC_ADDRESS(slot_id, 0, page_offset);
	markEuAsMockBad(orig_phy_addr);			
	reserve_eu_addr = write_to_reserve_eu(orig_phy_addr);		
	seg_map_ptr->is_eu_reserve   = IS_EU_RESERVE_TRUE;
	seg_map_ptr->reserve_eu_addr = reserve_eu_addr;
	
	// save expected address before writing	
	copyLogicalAddress(log_address, GET_RECLAIMED_ADDRESS_PTR());
	/* save physical address to which the checkpoint will be written */	
	phy_addr      = logicalAddressToPhysical(log_address);			
	
	/* commit*/
	if(commit(fs_data, fs_data_size, 1))
		return 0;
	
	read_cp_to_buffers(fs_data, fs_data_size, cp_seg_map_ptr, cp_obs_map_ptr); 	 	
	
	/* compare expected data in fs_data*/
	for(i=0; i< fs_data_size; i++){		
		VERIFY(COMPARE(fs_data[i], test_byte));
	}
//	print(uart0SendByte,0,"\nfs data verification successful ");	
//	print(uart0SendByte,0,"\ncompare segment map ");
	// compare written and current segment map	
//	VERIFY(verify_seg_map(cp_seg_map_ptr, 
//				   test_seg_map_ptr->sequence_num,
//				   test_seg_map_ptr->new_slot_id,
//				   test_seg_map_ptr->previously_written_segment,
//				   test_seg_map_ptr->is_eu_reserve,
//				   test_seg_map_ptr->nSegments));
////	print(uart0SendByte,0,"\ncompare map ");
//	for(i=0; i<SEQ_N_SLOTS_COUNT ;i++){
//		VERIFY(COMPARE(cp_seg_map_ptr->seg_to_slot_map[i], test_seg_map_ptr->seg_to_slot_map[i]));		
//	}

	// compare written and read checkpoint
	for(i=0; i<SEQ_OBS_COUNTERS; i++){		
		VERIFY(COMPARE(get_counter_level_by_map(i,cp_obs_map_ptr), get_counter_level(i)));
	}
		
	return 1;
}

/**
 * @brief
 * write a checkpoint which is larger then one page to the last page of a segment - 
 * verify the commit was successful
 * @return 1 if successful, 0 if the commit failed
 * 
 */
error_t commitTest5(){
	// set logical address at the beginning of a segment
	uint32_t phy_addr, seg_num = 4, slot_id = 4, sequence_num = 643, new_slot_id = slot_id, 
			 previously_written_segment = 3,is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = 2,
			 fs_data_size = NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200;
	uint8_t fs_data[NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200], test_byte = 'a';	
	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	uint8_t obs_level;
#ifndef EXACT_COUNTERS	
	obs_level =OBS_COUNT_LEVEL_1;
#else
	obs_level = 5;
#endif
	
	// init segment map with various details
	set_seg_map(seg_map_ptr,
				 slot_id,
				 SEQ_PAGES_PER_SLOT-1,
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 previously_written_segment,
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	// init obs map	
	set_obs_counters(obs_level);	
	
	/* init fs_data*/
	fsMemset(fs_data, test_byte, fs_data_size);	
	
	/* save expected address before writing */
	SET_LOGICAL_OFFSET(log_address, GET_RECLAIMED_OFFSET());
	SET_LOGICAL_SEGMENT(log_address, GET_RECLAIMED_SEGMENT());
	
	/* save physical address to which the checkpoint will be written */
	phy_addr = logicalAddressToPhysical(log_address);		
		
//	PRINT_MSG_AND_NUM("\nmain_area_writes[CALC_ADDRESS(5,0,0)]=", main_area_writes[CALC_ADDRESS(5,0,0)]);
//	PRINT_MSG_AND_NUM("\nrec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\ncp size=", NAND_PAGE_SIZE-SEQ_CHECKPOINT_SIZE+200);

	/* commit*/
	if(commit(fs_data, fs_data_size, 0))
		return 0;	
		
	return 1;
}
