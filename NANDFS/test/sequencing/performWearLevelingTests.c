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

/** @file performWearLevelingTests.c  */
#include <test/sequencing/performWearLevelingTests.h>
#include <test/sequencing/testsHeader.h>

/**
 * run all performWearLeveling tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllPerformWearLevelingTests(){
//	PRINT("\nrunAllPerformWearLevelingTests() - starting");
#ifdef Debug	
	RUN_TEST(performWearLeveling, 1);		
	RUN_TEST(performWearLeveling, 2);	
	RUN_TEST(performWearLeveling, 3);	
	RUN_TEST(performWearLeveling, 4);	
	RUN_TEST(performWearLeveling, 5);	
	RUN_TEST(performWearLeveling, 6);	
	RUN_TEST(performWearLeveling, 7);
#endif
	
	return 0;
}

/**
 * @brief 
 * init performWearLeveling test
 */
error_t init_performWearLevelingTest(){
	if(nandInit())
		return -1;		
	
	init_seg_map();
	init_obs_counters()	;		
	init_flash();
	mark_reserve_segment_headers();
	init_buf(sequencing_buffer);
	
//	PRINT("\ninit performWearLevelingTest - done");
	return 1;	
}									

/**
 * @brief 
 * tear down performWearLeveling test
 */
error_t tearDown_performWearLevelingTest(){
	init_seg_map();
	init_obs_counters();	
	init_flash();
	nandTerminate();		
	init_buf(sequencing_buffer);
	
	return 1;
}

/**
 * @brief
 * perform wear leveling on a slot with 0 bad EU's to a slot with 0 bad EU's
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest1(){
	uint32_t i,
	         sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = SEQ_SEGMENTS_COUNT,
	         previously_written_segment = SEQ_FIRST_RESERVE_SEGMENT-SEQ_K_RESERVE_SLOTS, new_slot_id = SEQ_NEW_SEGMENT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	uint8_t byte = 'c';		
	
	init_logical_address(log_addr);	
	init_logical_address(next_log_addr);	
	
//	PRINT("\nperformWearLevelingTest1() - starting");	
	for(i=0;i<SEQ_SEGMENTS_COUNT;i++){
		seg_map_ptr->seg_to_slot_map[i] = i;
	}	
	
	/* set seg map - we've jsut finished reclaiming a segment,and
	 * set it as the previous segment reclaimed*/
	set_seg_map(seg_map_ptr,
				 previously_written_segment, // reclaimed segment's slot
				 SEQ_PAGES_PER_SLOT-1, // reclaimed offset. notice-  page offset is last page written!
				 previously_written_segment, // reclaimed segment
				 sequence_num,				 
				 new_slot_id, // new slot id
				 previously_written_segment, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY); // no reserve EU
	
	fsMemset(sequencing_buffer, byte, NAND_PAGE_SIZE);
	SET_SEG_TYPE_FLAG(flags, SEG_TYPE_USED);	
	/* fill segment with data*/			 	
	for(i=CALC_ADDRESS(TEMP_RANDOM_SLOT,0,0); i< CALC_ADDRESS((TEMP_RANDOM_SLOT+1),0,0); i++){
//		PRINT_MSG_AND_NUM("\nsetting page ",i);
		VERIFY(!nandProgramTotalPage(sequencing_buffer, i));
	}
	
//	PRINT("\nabout to perform wear leveling");
	/* perform wear leveling*/
	VERIFY(!performWearLeveling());
//	PRINT("\nverifying erasure of random slot");				 
	/* verify the expected slot is now erased*/
	VERIFY(verifySlotIsErased(TEMP_RANDOM_SLOT));
	
//	PRINT("\nverifying segment map");
//	PRINT_MSG_AND_NUM("\nnew slot id=",new_slot_id);
	/* verify segment map was changed as expected*/
	VERIFY(COMPARE(new_slot_id,seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]));
//	PRINT("\nverifying new slot id");
	VERIFY(COMPARE(seg_map_ptr->new_slot_id, TEMP_RANDOM_SLOT));
	
//	PRINT_MSG_AND_NUM("\nverifying that re-writing was done to what was the new slot ", new_slot_id);
	/* verify new slot contains information from old slot*/
	for(i=CALC_ADDRESS(new_slot_id,0,0); i< CALC_ADDRESS((new_slot_id+1),0,0); i+=NAND_PAGES_PER_ERASE_UNIT){
//		PRINT_MSG_AND_NUM("\nverifying re-writing to EU ",i);
		VERIFY(verifyEUWithByte(i,byte));		
	}
	
	return 1;
}


/**
 * @brief
 * perform wear leveling on a slot with 1 bad EU to a slot with 0 bad EU's
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest2(){
	uint32_t i, bad_eu_offset = 5,
	         sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = SEQ_SEGMENTS_COUNT,
	         previously_written_segment = SEQ_FIRST_RESERVE_SEGMENT-SEQ_K_RESERVE_SLOTS, new_slot_id = SEQ_NEW_SEGMENT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	uint8_t byte = 'c';		
	
	init_logical_address(log_addr);	
	init_logical_address(next_log_addr);	
	
	for(i=0;i<SEQ_SEGMENTS_COUNT;i++){
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, i, i);
	}	
	
	/* set seg map - we've jsut finished reclaiming a segment,and
	 * set it as the previous segment reclaimed*/
	set_seg_map(seg_map_ptr,
				 previously_written_segment, // reclaimed segment's slot
				 SEQ_PAGES_PER_SLOT-1, // reclaimed offset. notice-  page offset is last page written!
				 previously_written_segment, // reclaimed segment
				 sequence_num,				 
				 new_slot_id, // new slot id
				 previously_written_segment, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY); // no reserve EU
	
	
	fsMemset(sequencing_buffer, byte, NAND_PAGE_SIZE);
	SET_SEG_TYPE_FLAG(flags, SEG_TYPE_USED);	
	
	/* fill segment with data*/			 	
	for(i=CALC_ADDRESS(TEMP_RANDOM_SLOT,0,0); i< CALC_ADDRESS((TEMP_RANDOM_SLOT+1),0,0); i++){
		/* when we get to the bad EU, write a reserv one*/
		if(CALC_EU_OFFSET(i) == bad_eu_offset){
//			PRINT("\nmarking EU as bad");
			VERIFY(markEuAsMockBad(i));
//			PRINT("\nwriting data to reserve EU");
			VERIFY(!COMPARE(write_data_to_reserve_eu(i, sequencing_buffer), SEQ_PHY_ADDRESS_EMPTY));		
			i+=NAND_PAGES_PER_ERASE_UNIT-1;
			continue;
		}
//		PRINT_MSG_AND_NUM("\nsetting page ",i);
		VERIFY(!nandProgramTotalPage(sequencing_buffer, i));
	}
		
	/* perform wear leveling*/
//	PRINT("\nabout to perform wear leveling");
	VERIFY(!performWearLeveling());
//	PRINT("\nverifying erasure of random slot");				 
	
	/* verify the expected slot is now erased*/
	VERIFY(verifySlotIsErased(TEMP_RANDOM_SLOT));
//	PRINT("\nverifying segment map");
//	PRINT_MSG_AND_NUM("\nnew slot id=",new_slot_id);
	
	/* verify segment map was changed as expected*/
	VERIFY(COMPARE(new_slot_id,seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]));
//	PRINT("\nverifying new slot id");
	VERIFY(COMPARE(seg_map_ptr->new_slot_id, TEMP_RANDOM_SLOT));
	
//	PRINT_MSG_AND_NUM("\nverifying that writing was done to what was the new slot ", new_slot_id);
	/* verify new slot contains information from old slot*/
	for(i=CALC_ADDRESS(new_slot_id,0,0); i< CALC_ADDRESS((new_slot_id+1),0,0); i+=NAND_PAGES_PER_ERASE_UNIT){		
		VERIFY(verifyEUWithByte(i,byte));				
	}
	
	return 1;
}

/**
 * @brief
 * perform wear leveling on a slot with 0 bad EU to a slot with 1 bad EU
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest3(){
	uint32_t i, bad_eu_offset = 5, 
	         sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = SEQ_SEGMENTS_COUNT,
	         previously_written_segment = SEQ_FIRST_RESERVE_SEGMENT-SEQ_K_RESERVE_SLOTS, new_slot_id = SEQ_NEW_SEGMENT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	uint8_t byte = 'c';		
	
	init_logical_address(log_addr);	
	init_logical_address(next_log_addr);	
	
	for(i=0;i<SEQ_SEGMENTS_COUNT;i++){
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, i, i);
	}
	
	/* set seg map - we've jsut finished reclaiming a segment,and
	 * set it as the previous segment reclaimed*/
	set_seg_map(seg_map_ptr,
				 previously_written_segment, // reclaimed segment's slot
				 SEQ_PAGES_PER_SLOT-1, // reclaimed offset. notice-  page offset is last page written!
				 previously_written_segment, // reclaimed segment
				 sequence_num,				 
				 new_slot_id, // new slot id
				 previously_written_segment, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY); // no reserve EU
	
	
	fsMemset(sequencing_buffer, byte, NAND_PAGE_SIZE);
	SET_SEG_TYPE_FLAG(flags, SEG_TYPE_USED);	
	/* fill segment with data*/			 	
	for(i=CALC_ADDRESS(TEMP_RANDOM_SLOT,0,0); i< CALC_ADDRESS((TEMP_RANDOM_SLOT+1),0,0); i++){
		/* when we get to the bad EU, write a reserve one*/		
//		PRINT_MSG_AND_NUM("\nsetting page ",i);
		VERIFY(!nandProgramTotalPage(sequencing_buffer, i));
	}
	
	markEuAsMockBad(CALC_ADDRESS(new_slot_id,bad_eu_offset,0));
//	PRINT("\nabout to perform wear leveling");
	
	/* perform wear leveling*/
	VERIFY(!performWearLeveling());
//	PRINT("\nverifying erasure of random slot");				 
	
	/* verify the expected slot is now erased*/
	VERIFY(verifySlotIsErased(TEMP_RANDOM_SLOT));	
//	PRINT("\nverifying segment map");
//	PRINT_MSG_AND_NUM("\nnew slot id=",new_slot_id);
//	PRINT_MSG_AND_NUM("\ntest_seg_map_ptr->new_slot_id=",test_seg_map_ptr->new_slot_id);
//	PRINT_MSG_AND_NUM("\ntest_seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]=",test_seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]);
	
	/* verify segment map was changed as expected*/
	VERIFY(COMPARE(new_slot_id,seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]));
//	PRINT("\nverifying new slot id");
	VERIFY(COMPARE(seg_map_ptr->new_slot_id, TEMP_RANDOM_SLOT));
	
	/* verify new slot contains information from old slot*/
//	PRINT_MSG_AND_NUM("\nverifying that writing was done to what was the new slot ", new_slot_id);
	for(i=CALC_ADDRESS(new_slot_id,0,0); i< CALC_ADDRESS((new_slot_id+1),0,0); i+=NAND_PAGES_PER_ERASE_UNIT){
//		PRINT_MSG_AND_NUM("\ni=", i);		
		VERIFY(verifyEUWithByte(i,byte));
//		PRINT(". done");				
	}
	
	return 1;
}


/**
 * @brief
 * perform wear leveling on a slot with 1 bad EU to a slot with 1 bad EU
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest4(){
	uint32_t i, to_bad_eu_offset = 5, from_bad_eu_offset = 5, 
	         sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = SEQ_SEGMENTS_COUNT,
	         previously_written_segment = SEQ_FIRST_RESERVE_SEGMENT-SEQ_K_RESERVE_SLOTS, new_slot_id = SEQ_NEW_SEGMENT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	uint8_t byte = 'c';		
	
	init_logical_address(log_addr);	
	init_logical_address(next_log_addr);	
	
	for(i=0;i<SEQ_SEGMENTS_COUNT;i++){
		seg_map_ptr->seg_to_slot_map[i] = i;
	}
		
	/* set seg map - we've jsut finished reclaiming a segment,and
	 * set it as the previous segment reclaimed*/
	set_seg_map(seg_map_ptr,
				 previously_written_segment, // reclaimed segment's slot
				 SEQ_PAGES_PER_SLOT-1, // reclaimed offset. notice-  page offset is last page written!
				 previously_written_segment, // reclaimed segment
				 sequence_num,				 
				 new_slot_id, // new slot id
				 previously_written_segment, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY); // no reserve EU	
	
	fsMemset(sequencing_buffer, byte, NAND_PAGE_SIZE);
	SET_SEG_TYPE_FLAG(flags, SEG_TYPE_USED);	
	/* fill segment with data*/			 	
	for(i=CALC_ADDRESS(TEMP_RANDOM_SLOT,0,0); i< CALC_ADDRESS((TEMP_RANDOM_SLOT+1),0,0); i++){
		SET_SEG_TYPE_FLAG(flags, SEG_TYPE_USED);
		/* when we get to the bad EU, write a reserve one*/	
		/* when we get to the bad EU, write a reserv one*/
		if(CALC_EU_OFFSET(i) == from_bad_eu_offset){
//			PRINT("\nmarking EU as bad");
			VERIFY(markEuAsMockBad(i));
//			PRINT("\nwriting data to reserve EU");
			VERIFY(!COMPARE(write_data_to_reserve_eu(i, sequencing_buffer), SEQ_PHY_ADDRESS_EMPTY));		
			i+=NAND_PAGES_PER_ERASE_UNIT-1;
			continue;
		}	
//		PRINT_MSG_AND_NUM("\nsetting page ",i);
		VERIFY(!nandProgramTotalPage(sequencing_buffer, i));
	}
	
	markEuAsMockBad(CALC_ADDRESS(new_slot_id,to_bad_eu_offset,0));
//	PRINT("\nabout to perform wear leveling");
	/* perform wear leveling*/
	VERIFY(!performWearLeveling());
//	PRINT("\nverifying erasure of random slot");				 
	/* verify the expected slot is now erased*/
	VERIFY(verifySlotIsErased(TEMP_RANDOM_SLOT));
	
//	PRINT("\nverifying segment map");
//	PRINT_MSG_AND_NUM("\nnew slot id=",new_slot_id);
//	PRINT_MSG_AND_NUM("\ntest_seg_map_ptr->new_slot_id=",test_seg_map_ptr->new_slot_id);
//	PRINT_MSG_AND_NUM("\ntest_seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]=",test_seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]);
	/* verify segment map was changed as expected*/
	VERIFY(COMPARE(new_slot_id,seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]));
//	PRINT("\nverifying new slot id");
	VERIFY(COMPARE(seg_map_ptr->new_slot_id, TEMP_RANDOM_SLOT));
	
//	PRINT_MSG_AND_NUM("\nverifying that writing was done to what was the new slot ", new_slot_id);
	/* verify new slot contains information from old slot*/
	for(i=CALC_ADDRESS(new_slot_id,0,0); i< CALC_ADDRESS((new_slot_id+1),0,0); i+=NAND_PAGES_PER_ERASE_UNIT){		
		VERIFY(verifyEUWithByte(i,byte));				
	}
	
	return 1;
}

/**
 * @brief
 * perform wear leveling on a slot with 2 consecutive bad EU's to a slot with 2 consecutive bad EU
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest5(){
	uint32_t i, to_bad_eu_offset1 = 5, from_bad_eu_offset1 = 5, to_bad_eu_offset2 = 5, from_bad_eu_offset2 = 5, 
	         sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = SEQ_SEGMENTS_COUNT,
	         previously_written_segment = SEQ_FIRST_RESERVE_SEGMENT-SEQ_K_RESERVE_SLOTS, new_slot_id = SEQ_NEW_SEGMENT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	uint8_t byte = 'c';		
	
	init_logical_address(log_addr);	
	init_logical_address(next_log_addr);	
	
	for(i=0;i<SEQ_SEGMENTS_COUNT;i++){
		seg_map_ptr->seg_to_slot_map[i] = i;
	}
			
	/* set seg map - we've jsut finished reclaiming a segment,and
	 * set it as the previous segment reclaimed*/
	set_seg_map(seg_map_ptr,
				 previously_written_segment, // reclaimed segment's slot
				 SEQ_PAGES_PER_SLOT-1, // reclaimed offset. notice-  page offset is last page written!
				 previously_written_segment, // reclaimed segment
				 sequence_num,				 
				 new_slot_id, // new slot id
				 previously_written_segment, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY); // no reserve EU	
	
	fsMemset(sequencing_buffer, byte, NAND_PAGE_SIZE);	
	/* fill segment with data*/			 	
	for(i=CALC_ADDRESS(TEMP_RANDOM_SLOT,0,0); i< CALC_ADDRESS((TEMP_RANDOM_SLOT+1),0,0); i++){
		SET_SEG_TYPE_FLAG(flags, SEG_TYPE_USED);
		/* when we get to the bad EU, write a reserve one*/	
		/* when we get to the bad EU, write a reserv one*/
		if(CALC_EU_OFFSET(i) == from_bad_eu_offset1 || CALC_EU_OFFSET(i) == from_bad_eu_offset2){
//			PRINT("\nmarking EU as bad");
			VERIFY(markEuAsMockBad(i));
//			PRINT("\nwriting data to reserve EU");
			VERIFY(!COMPARE(write_data_to_reserve_eu(i, sequencing_buffer), SEQ_PHY_ADDRESS_EMPTY));		
			i+=NAND_PAGES_PER_ERASE_UNIT-1;
			continue;
		}	
//		PRINT_MSG_AND_NUM("\nsetting page ",i);
		VERIFY(!nandProgramTotalPage(sequencing_buffer, i));
	}
	
	markEuAsMockBad(CALC_ADDRESS(new_slot_id,to_bad_eu_offset1,0));
	markEuAsMockBad(CALC_ADDRESS(new_slot_id,to_bad_eu_offset2,0));
//	PRINT("\nabout to perform wear leveling");
	/* perform wear leveling*/
	VERIFY(!performWearLeveling());
//	PRINT("\nverifying erasure of random slot");				 
	/* verify the expected slot is now erased*/
	VERIFY(verifySlotIsErased(TEMP_RANDOM_SLOT));
	
//	PRINT("\nverifying segment map");
//	PRINT_MSG_AND_NUM("\nnew slot id=",new_slot_id);
//	PRINT_MSG_AND_NUM("\ntest_seg_map_ptr->new_slot_id=",test_seg_map_ptr->new_slot_id);
//	PRINT_MSG_AND_NUM("\ntest_seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]=",test_seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]);
	/* verify segment map was changed as expected*/
	VERIFY(COMPARE(new_slot_id,seg_map_ptr->seg_to_slot_map[TEMP_RANDOM_SLOT]));
//	PRINT("\nverifying new slot id");
	VERIFY(COMPARE(seg_map_ptr->new_slot_id, TEMP_RANDOM_SLOT));
	
//	PRINT_MSG_AND_NUM("\nverifying that writing was done to what was the new slot ", new_slot_id);
	/* verify new slot contains information from old slot*/
	for(i=CALC_ADDRESS(new_slot_id,0,0); i< CALC_ADDRESS((new_slot_id+1),0,0); i+=NAND_PAGES_PER_ERASE_UNIT){		
		VERIFY(verifyEUWithByte(i,byte));				
	}
	
	return 1;
}



/**
 * @brief
 * perform wear leveling on a reserve slot with no bad EU's to a slot with no bad EU's
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest6(){
	uint32_t i, j,
	         sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = SEQ_SEGMENTS_COUNT,
	         previously_written_segment = SEQ_FIRST_RESERVE_SEGMENT-SEQ_K_RESERVE_SLOTS, new_slot_id = SEQ_NEW_SEGMENT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	uint8_t byte = 'c';		
	page_area_flags *paf = (page_area_flags*)(sequencing_buffer);
	
	init_logical_address(log_addr);	
	init_logical_address(next_log_addr);	
	
	for(i=0;i<SEQ_SEGMENTS_COUNT;i++){
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, i, i);
	}	
	
	/* set seg map - we've jsut finished reclaiming a segment,and
	 * set it as the previous segment reclaimed*/
	set_seg_map(seg_map_ptr,
				previously_written_segment, // reclaimed segment's slot
				SEQ_PAGES_PER_SLOT-1, // reclaimed offset. notice-  page offset is last page written!
				previously_written_segment, // reclaimed segment
				sequence_num,				 
				new_slot_id, // new slot id
				previously_written_segment, // previously reclaimed segment
				is_eu_reserve,
				nSegments,
				SEQ_PHY_ADDRESS_EMPTY); // no reserve EU
//	PRINT("\nstart test");
	/* make TEMP_RANDOM_SLOT as reserve
	 * do this before setting data, since we only replace headers!*/
	 
	VERIFY(replaceReserveSlotWithDataSlot(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, SEQ_FIRST_RESERVE_SEGMENT), TEMP_RANDOM_SLOT));
//	PRINT("\nreplaceReserveSlotWithDataSlot success");
		
	fsMemset(sequencing_buffer, byte, NAND_PAGE_SIZE);
	SET_SEG_TYPE_FLAG(flags, SEG_TYPE_RESERVE);
		
	
	/* fill segment with data*/			 	
	for(i=CALC_ADDRESS(TEMP_RANDOM_SLOT,0,1); i< CALC_ADDRESS((TEMP_RANDOM_SLOT+1),0,0); i++){
//		PRINT_MSG_AND_NUM("\nsetting page ",i);
		VERIFY(!nandProgramTotalPage(sequencing_buffer, i));
	}	
	
//	PRINT("\nabout to perform wear leveling");
	/* perform wear leveling*/
	VERIFY(!performWearLeveling());
//	PRINT("\nverifying erasure of random slot");				 
	
	/* verify the expected slot is now erased*/
	VERIFY(verifySlotIsErased(TEMP_RANDOM_SLOT));	
//	PRINT("\nverifying segment map");
//	PRINT_MSG_AND_NUM("\nnew slot id=",new_slot_id);
//	PRINT_MSG_AND_NUM("\ntest_seg_map_ptr->new_slot_id=",seg_map_ptr->new_slot_id);	
	
	/* verify segment map was changed as expected*/	
	VERIFY(COMPARE(new_slot_id,GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,SEQ_FIRST_RESERVE_SEGMENT)));
//	PRINT("\nverifying new slot id");
	VERIFY(COMPARE(seg_map_ptr->new_slot_id, TEMP_RANDOM_SLOT));		
		
	/* verify new slot contains information from old slot*/
//	PRINT_MSG_AND_NUM("\nverifying that re-writing was done to what was the new slot ", new_slot_id);
	for(i=CALC_ADDRESS(new_slot_id,1,0); i< CALC_ADDRESS((new_slot_id+1),0,0); i+=NAND_PAGES_PER_ERASE_UNIT){		
//		PRINT_MSG_AND_NUM("\nverifying re-writing to EU ",i);
//		PRINT_MSG_AND_NUM(" next slot begins at ",CALC_ADDRESS((new_slot_id+1),0,0));
		if(!nandCheckEuStatus(i))
					continue;
					
		VERIFY(verifyEUWithByte(i,byte));		
	}
	
	/* verify reserve header*/
//	PRINT_MSG_AND_NUM("\nread header of first reserve seg from ",CALC_ADDRESS(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, SEQ_FIRST_RESERVE_SEGMENT),0,0));
	nandReadPageTotal(sequencing_buffer, CALC_ADDRESS(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, SEQ_FIRST_RESERVE_SEGMENT),0,0));
//	PRINT("\nverify reserve seg header ");
//	PRINT_MSG_AND_NUM("\nseg in header=",GET_HEADER_SEGMENT_ID(paf));	
	VERIFY(COMPARE(GET_HEADER_SEGMENT_ID(paf), SEQ_FIRST_RESERVE_SEGMENT));
//	PRINT("\nverified header seg ");	
	VERIFY(COMPARE(GET_SEG_TYPE_FLAG(flags), SEG_TYPE_RESERVE));
//	PRINT("\nverified reserve seg header ");
	
	for(i=CALC_ADDRESS(new_slot_id,0,1); i<CALC_ADDRESS(new_slot_id,1,0);i++){
		nandReadPageTotal(sequencing_buffer, i);
//		PRINT_MSG_AND_NUM("\nverifying re-writing to page ",i);
		for(j=0;j<NAND_PAGE_SIZE;j++){
			VERIFY(COMPARE(byte, sequencing_buffer[j]));
		} 
	}
	
	return 1;
}

/**
 * @brief
 * perform wear leveling on a reserve slot with 1 bad EU's to a slot with 1 bad EU's
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest7(){
	uint32_t i, j, to_bad_eu_offset = 5,from_bad_eu_offset = 5, 
	         sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = SEQ_SEGMENTS_COUNT,
	         previously_written_segment = SEQ_FIRST_RESERVE_SEGMENT-SEQ_K_RESERVE_SLOTS, new_slot_id = SEQ_NEW_SEGMENT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	uint8_t byte = 'c';		
	page_area_flags *paf = (page_area_flags*)(sequencing_buffer);
	
	init_logical_address(log_addr);	
	init_logical_address(next_log_addr);	
	
	for(i=0;i<SEQ_SEGMENTS_COUNT;i++){
		seg_map_ptr->seg_to_slot_map[i] = i;
	}
	
	/* set seg map - we've jsut finished reclaiming a segment,and
	 * set it as the previous segment reclaimed*/
	set_seg_map(seg_map_ptr,
				 previously_written_segment, // reclaimed segment's slot
				 SEQ_PAGES_PER_SLOT-1, // reclaimed offset. notice-  page offset is last page written!
				 previously_written_segment, // reclaimed segment
				 sequence_num,				 
				 new_slot_id, // new slot id
				 previously_written_segment, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY); // no reserve EU
	
	/* make TEMP_RANDOM_SLOT as reserve
	 * do this before setting data, since we only replace headers!*/	
	VERIFY(replaceReserveSlotWithDataSlot(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, SEQ_FIRST_RESERVE_SEGMENT), TEMP_RANDOM_SLOT));
//	PRINT("\nreplaceReserveSlotWithDataSlot success");

	fsMemset(sequencing_buffer, byte, NAND_PAGE_SIZE);
	SET_SEG_TYPE_FLAG(flags, SEG_TYPE_RESERVE);
	
//	PRINT("\nmarking EU as bad");
			/* mark EU as bad in the reserve segment*/
	markEuAsMockBad(CALC_ADDRESS(TEMP_RANDOM_SLOT,from_bad_eu_offset,0));	
	/* fill segment with data*/			 	
	for(i=CALC_ADDRESS(TEMP_RANDOM_SLOT,0,1); i< CALC_ADDRESS((TEMP_RANDOM_SLOT+1),0,0); i++){
//		PRINT_MSG_AND_NUM("\nabout to set page ",i);		
		/* when we get to the bad EU, write a reserv one*/
		if(CALC_EU_OFFSET(i) == from_bad_eu_offset){						
//			PRINT("\nwriting data to bad EU");
//			VERIFY(!COMPARE(write_data_to_reserve_eu(i, sequencing_buffer), SEQ_PHY_ADDRESS_EMPTY));		
			i+=NAND_PAGES_PER_ERASE_UNIT-1;
			continue;
		}	
		
		VERIFY(!nandProgramTotalPage(sequencing_buffer, i));
	}	
	
//	PRINT("\nabout to perform wear leveling");
	/* mark an EU as bad in the original segment*/
	markEuAsMockBad(CALC_ADDRESS(new_slot_id,to_bad_eu_offset,0));
	
	/* perform wear leveling*/
	VERIFY(!performWearLeveling());
	
//	PRINT("\nverifying erasure of random slot");				 
	VERIFY(verifySlotIsErased(TEMP_RANDOM_SLOT));
	
//	PRINT("\nverifying segment map");
//	PRINT_MSG_AND_NUM("\nnew slot id=",new_slot_id);

	/* verify segment map was changed as expected*/
	VERIFY(COMPARE(new_slot_id,seg_map_ptr->seg_to_slot_map[SEQ_FIRST_RESERVE_SEGMENT]));
//	PRINT("\nverifying new slot id");
	VERIFY(COMPARE(seg_map_ptr->new_slot_id, TEMP_RANDOM_SLOT));
	
//	PRINT_MSG_AND_NUM("\nverifying that re-writing was done to what was the new slot ", new_slot_id);
	/* verify new slot contains information from old slot*/
	for(i=CALC_ADDRESS(new_slot_id,1,0); i< CALC_ADDRESS((new_slot_id+1),0,0); i+=NAND_PAGES_PER_ERASE_UNIT){	
		/* this is a reserve segment. we don't care about bad EU's*/
		if(!nandCheckEuStatus(i))
			continue;
			
//		PRINT_MSG_AND_NUM("\nverifying re-writing to EU ",i);
		VERIFY(verifyEUWithByte(i,byte));		
	}
	
	/* verify reserve header*/
//	PRINT_MSG_AND_NUM("\nread header of first reserve seg from ",CALC_ADDRESS(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, SEQ_FIRST_RESERVE_SEGMENT),0,0));
	nandReadPageTotal(sequencing_buffer, CALC_ADDRESS(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, SEQ_FIRST_RESERVE_SEGMENT),0,0));
//	PRINT("\nverify reserve seg header ");
//	PRINT_MSG_AND_NUM("\nseg in header=",GET_HEADER_SEGMENT_ID(paf));	
	VERIFY(COMPARE(GET_HEADER_SEGMENT_ID(paf), SEQ_FIRST_RESERVE_SEGMENT));
//	PRINT("\nverified header seg ");	
	VERIFY(COMPARE(GET_SEG_TYPE_FLAG(flags), SEG_TYPE_RESERVE));
//	PRINT("\nverified reserve seg header ");
	
	for(i=CALC_ADDRESS(new_slot_id,0,1); i<CALC_ADDRESS(new_slot_id,1,0);i++){
		nandReadPageTotal(sequencing_buffer, i);
		for(j=0;j<NAND_PAGE_SIZE;j++){
			VERIFY(COMPARE(byte, sequencing_buffer[j]));
		} 
	}
	
	return 1;
}
