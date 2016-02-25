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

/** @file allocAndWriteBlockTests.c  */
#include <test/sequencing/allocAndWriteBlockTests.h>
#include <test/sequencing/testsHeader.h>
#include <test/sequencing/testSequencingUtils.h>

/**
 * run all allocAndWriteBlock tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllallocAndWriteBlockTests(){
	RUN_TEST(allocAndWriteBlock, 1);
	RUN_TEST(allocAndWriteBlock, 2);
	RUN_TEST(allocAndWriteBlock, 3);
	RUN_TEST(allocAndWriteBlock, 4);
	RUN_TEST(allocAndWriteBlock, 5);
	RUN_TEST(allocAndWriteBlock, 7);
	RUN_TEST(allocAndWriteBlock, 8);	
	RUN_TEST(allocAndWriteBlock, 9);
	RUN_TEST(allocAndWriteBlock, 10);
	RUN_TEST(allocAndWriteBlock, 11);
	RUN_TEST(allocAndWriteBlock, 12);
	RUN_TEST(allocAndWriteBlock, 13);
	
	return 0;
}

/**
 * @brief 
 * init allocAndWriteBlock test
 */
error_t init_allocAndWriteBlockTest(){
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
 * tear down allocAndWriteBlock test
 */
error_t tearDown_allocAndWriteBlockTest(){
	init_seg_map();
	init_obs_counters();	
	init_flash();
	nandTerminate();		
	init_buf(sequencing_buffer);
	
	return 1;
}

/**
 * @brief
 * write to a segment when the flash is not full (not in allocation mode).
 * write is done in the middle of an EU (no funny stuff).
 * should simply write the page and advance pointer
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest1(){
	uint32_t res, i, phy_addr, seg_num = 5, slot_id = 2, page_offset = 3, new_slot_id = slot_id, sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);		
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];	
	
	init_logical_address(log_addr);	
	init_logical_address(prev_log_addr);
	init_logical_address(next_log_addr);	
	
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
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	
	init_buf(sequencing_buffer);
	fsMemset(buf, byte, NAND_PAGE_SIZE);
	/* change a flags, just to see if it is written and read write*/
//	SET_VOTS_FLAG(flags, VOTS_FLAG_TRUE);s
//	PRINT_MSG_AND_NUM("\nGET_VOTS_FLAG(flags)=",GET_VOTS_FLAG(flags));
	res = allocAndWriteBlock(next_log_addr, buf, 1, prev_log_addr, &cpWritten, checkpointWriter,0);
	
	VERIFY(!res);
//	PRINT("\nallocAndWriteBlock success");
	/* verify that indeed the physical address is advanced by one*/
	res = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());	
	VERIFY(COMPARE(phy_addr+1, res));
	/* verify logical address - 
	 * only offset should increment by 1*/
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(log_addr),GET_RECLAIMED_SEGMENT()));
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(log_addr)+1,GET_RECLAIMED_OFFSET()));
//	PRINT("\nlog address success");
	VERIFY(COMPARE(seg_map_ptr->is_eu_reserve, IS_EU_RESERVE_FALSE));
	
//	init_buf(sequencing_buffer);
		
	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(next_log_addr));	
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(sequencing_buffer[i], byte));
	}
//	PRINT("\nread data success");
//	PRINT_MSG_AND_NUM("\nGET_VOTS_FLAG(flags)=",GET_VOTS_FLAG(flags));
//	PRINT_MSG_AND_NUM("\nVOTS_FLAG_TRUE=",VOTS_FLAG_TRUE);
	/* verify the flag*/
	VERIFY(COMPARE(GET_VOTS_FLAG(flags), VOTS_FLAG_TRUE));
	
	return 1;
}
//
/**
 * @brief
 * write to a segment when the flash is not full (not in reclamation mode).
 * write is to the end of EU, forcing the test of the next EU whetehr it is valid.
 * should simply write the page and advance pointer
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest2(){
	uint32_t res, i, phy_addr, seg_num = 5, slot_id = 2, page_offset = NAND_PAGES_PER_ERASE_UNIT-1, new_slot_id = slot_id, sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);	
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	
	init_logical_address(log_addr);	
	init_logical_address(next_log_addr);	
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
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	
	fsMemset(buf, byte, NAND_PAGE_SIZE);
	/* change a flags, just to see if it is written and read write*/	
	res = allocAndWriteBlock(next_log_addr, buf, 1, prev_log_addr, &cpWritten, checkpointWriter,0);
	VERIFY(!res);
	
	/* verify that indeed the physical address is advanced by one*/
	VERIFY(COMPARE(phy_addr+1, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	/* verify logical address - 
	 * only offset should increment by 1*/
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(log_addr),GET_RECLAIMED_SEGMENT()));
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(log_addr)+1,GET_RECLAIMED_OFFSET()));
	/* verify we didn't advance to a reserve eu*/
	VERIFY(COMPARE(seg_map_ptr->is_eu_reserve, IS_EU_RESERVE_FALSE));
	
	init_buf(sequencing_buffer);	
	
	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(next_log_addr));
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(sequencing_buffer[i], byte));
	}
	
	/* verify the flag*/
	VERIFY(COMPARE(GET_VOTS_FLAG(flags), VOTS_FLAG_TRUE));
	
	return 1;
}

/**
 * @brief
 * write to a segment when the flash is not full (not in reclamation mode).
 * write is to the end of EU, forcing the test of the next EU whetehr it is valid.
 * next eu is not valid, so a reserve EU should be allocated * 
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest3(){
	uint32_t i;	
	INIT_FLAGS_STRUCT_AND_PTR(header_flags);	
	uint32_t res, phy_addr, seg_num = 5, slot_id = 2, page_offset = NAND_PAGES_PER_ERASE_UNIT-1, new_slot_id = slot_id, sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(before_alloc_seg_map_log_addr);		
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(written_to_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);			
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	
	initFlags(header_flags);
	init_logical_address(before_alloc_seg_map_log_addr);		
	init_logical_address(written_to_log_addr);	
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
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(before_alloc_seg_map_log_addr, GET_RECLAIMED_ADDRESS_PTR());
	phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	/* mark next EU as abd*/
	markEuAsMockBad(phy_addr+1);
	
//	nandReadPageFlags(header_flags, 64512);	
//	PRINT_MSG_AND_NUM("\n2. segment_type = ",header_flags->segment_type);
	
	fsMemset(buf, byte, NAND_PAGE_SIZE);	
	
	/* allocate. the logical address to which the data is written is stored in next_log_addr */
	res = allocAndWriteBlock(written_to_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);
	VERIFY(!res);
	
//	PRINT("\nallocAndWriteBlock successful");
	/* verify we didn't simply advance to next physical page*/
	VERIFY(!COMPARE(phy_addr+1, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\nwe didn't simply advance to next physical page");
	/* verify logical address - 
	 * only offset should increment by 1*/
	 
	 VERIFY(COMPARE(GET_LOGICAL_SEGMENT(before_alloc_seg_map_log_addr),GET_RECLAIMED_SEGMENT()));
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(before_alloc_seg_map_log_addr)+1,GET_RECLAIMED_OFFSET()));
	
//	PRINT("\nverified logical address");
//	PRINT_MSG_AND_NUM("\nverifying slot header in address",GET_PAGE_SLOT_HEADER_ADDRESS(test_seg_map_ptr->reserve_eu_addr));
	/* verify we advanced to a reserve eu*/
	VERIFY(COMPARE(seg_map_ptr->is_eu_reserve, IS_EU_RESERVE_TRUE));
//	PRINT("\nverified moved to reserve address");
	/* verify that the physical address is of the next free page is in a segment marked as reserve*/	
	nandReadPageFlags(header_flags, GET_PAGE_SLOT_HEADER_ADDRESS(seg_map_ptr->reserve_eu_addr));	
//	PRINT_MSG_AND_NUM("\nsegment_type = ",header_flags->segment_type);
	VERIFY(COMPARE(GET_SEG_TYPE_FLAG(header_flags), SEG_TYPE_RESERVE));
//	PRINT("\nverified slot header");
	/* verify written data is identical to read*/
	init_buf(sequencing_buffer);	
	
	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(written_to_log_addr));	
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(sequencing_buffer[i], byte));
	}
//	PRINT("\nverified written data");	
//	PRINT("\nverified vots_data_flag");
	/* verify the page is not marked as reserve, with a slot id and offset*/	
	VERIFY(COMPARE(GET_SLOT_EU_OFFSET(flags), SEQ_NO_SLOT_AND_EU_OFFSET));
	
//	PRINT("\nverified flags");
	return 1;
}

/**
 * @brief
 * write to a segment when we are in reclamation state.
 * no copying is needed. 
 * should simply write the page and advance pointer
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest4(){
	uint32_t res, i, phy_addr, seg_num = 0, slot_id = 0, slot_id1 = 8, page_offset = 5, 
	         new_slot_id = slot_id1, sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = SEQ_SEGMENTS_COUNT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(before_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(written_to_log_addr);	
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	
	init_logical_address(before_log_addr);	
	init_logical_address(prev_log_addr);	
	init_logical_address(written_to_log_addr);	
		
	/* set seg map*/
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 SEQ_NO_SEGMENT_NUM, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
//	test_seg_map_ptr->seg_to_slot_map[seg_num1] = slot_id1;
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(before_log_addr, GET_RECLAIMED_ADDRESS_PTR());
	copyLogicalAddress(written_to_log_addr, before_log_addr);
		
	SET_LOGICAL_OFFSET(written_to_log_addr,GET_LOGICAL_OFFSET(written_to_log_addr) + 1);
	
	phy_addr = CALC_ADDRESS(slot_id1,0,page_offset);
	
	/* mark next page as obsolete so that the address in the seg map will simply
	 * be incremented (no page copying)*/
	VERIFY(!markAsObsolete(written_to_log_addr, MARK_OBSOLETE_NOT_AFTER_REBOOT));
//	{
//	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr1);
//	INIT_FLAGS_POINTER_TO_BUFFER(flags_ptr,sequencing_buffer);
//	log_addr1->segment_num=0;
//	log_addr1->page_offset=5;
//	init_buf(sequencing_buffer);	
//	readBlock(log_addr1, sequencing_buffer);
//	VERIFY(COMPARE(flags_ptr->obsolete_flag, OBS_FLAG_TRUE));	
//	init_buf(sequencing_buffer);	
//	}
	
	init_buf(buf);
	fsMemset(buf, byte, NAND_PAGE_SIZE);
	init_buf(sequencing_buffer);
//	PRINT_MSG_AND_NUM("\n phy_addr+1=",phy_addr+1);
//	PRINT("\nabout to allocAndWriteBlock ");
//	PRINT_MSG_AND_NUM("\ncopy_flags1->obsolete_flag=",flags->obsolete_flag);
//	PRINT_MSG_AND_NUM("\ncopy_flags1->cp_flag=",flags->cp_flag);
	res = allocAndWriteBlock(written_to_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);	
	VERIFY(!res);
//	PRINT("\nallocAndWriteBlock successful");
	init_buf(sequencing_buffer);
		
	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(written_to_log_addr));	
//	readBlock(written_to_log_addr, sequencing_buffer);
	for(i=0; i<NAND_PAGE_SIZE;i++){
//		PRINT_MSG_AND_HEX("\n byte is ",sequencing_buffer[i]);
		VERIFY(COMPARE(sequencing_buffer[i], byte));
	}
//	PRINT("\n verified written data");
	
//	PRINT("\n allocAndWriteBlock successful");
	/* verify that indeed the physical address is advanced by one*/
//	PRINT_MSG_AND_NUM("\n phy_addr+1=",phy_addr+1);
//	PRINT_MSG_AND_NUM("\n logicalAddressToPhysical(SEQ_SEG_MAP_RECLAIMED_ADDR_PTR(test_seg_map_ptr))=",logicalAddressToPhysical(SEQ_SEG_MAP_RECLAIMED_ADDR_PTR(test_seg_map_ptr)));
	VERIFY(COMPARE(CALC_ADDRESS(slot_id,0,page_offset)+1, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	VERIFY(COMPARE(phy_addr+1, logicalAddressToPhysical(before_log_addr)+1));
	
//	PRINT("\n verified physical address advanced by one");
	/* verify logical address - 
	 * only offset should increment by 1*/	 
	 VERIFY(COMPARE(GET_LOGICAL_SEGMENT(before_log_addr),GET_RECLAIMED_SEGMENT()));
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(before_log_addr)+1,GET_RECLAIMED_OFFSET()));
//	PRINT("\n verified logical address ");

	/* verify we didn't advance to a reserve eu*/
	VERIFY(COMPARE(seg_map_ptr->is_eu_reserve, IS_EU_RESERVE_FALSE));
//	PRINT("\n verified not in reserve EU");
	
	return 1;
}

/**
 * @brief
 * write to a segment when we are in reclamation state.
 * write is done to last page of EU, so we now move to a new one.
 * the next one is bad so a reserve EU should be allocated
 * no copying is needed. 
 * should simply write the page and advance pointer.
 * verify segment map us amrked with the reserve EU address
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest5(){
	uint32_t res, i, org_phy_addr, phy_addr, seg_num = 2, slot_id = seg_num, seg_num1 = 6, slot_id1 = seg_num1, page_offset = NAND_PAGES_PER_ERASE_UNIT-1, 
	         new_slot_id = slot_id1, sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE, nSegments = SEQ_SEGMENTS_COUNT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(before_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(header_flags);
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];	
	
	init_logical_address(before_log_addr);	
	init_logical_address(prev_log_addr);	
	init_logical_address(next_log_addr);
		
	markDataSegementsSequentially();
	
	/* set seg map*/
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 SEQ_NO_SEGMENT_NUM, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	seg_map_ptr->seg_to_slot_map[seg_num1] = slot_id1;
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(before_log_addr, GET_RECLAIMED_ADDRESS_PTR());
	copyLogicalAddress(next_log_addr, before_log_addr);
	
	SET_LOGICAL_OFFSET(next_log_addr, GET_LOGICAL_OFFSET(next_log_addr)+1);
	phy_addr     = CALC_ADDRESS(slot_id1,0,page_offset);
	org_phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	/* mark next EU as bad*/
//	PRINT_MSG_AND_NUM("\nmarked eu as bad in address ",phy_addr);
	markEuAsMockBad(phy_addr+1);
	
	/* mark next page as obsolete so that the address in the seg map will simply
	 * be incremented (no page copying)*/
	markAsObsolete(next_log_addr, MARK_OBSOLETE_NOT_AFTER_REBOOT);
	
	fsMemset(buf, byte, NAND_PAGE_SIZE);
	
//	PRINT("\n about to allocAndWriteBlock");
	res = allocAndWriteBlock(next_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);
	VERIFY(!res);
//	PRINT("\n allocAndWriteBlock successful");
	/* verify that indeed the physical address is not simply advanced by one*/
	VERIFY(!COMPARE(phy_addr+1, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\n verified physical address not simply advanced by one");
	/* verify logical address - 
	 * only offset should increment by 1*/
	 
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(before_log_addr),GET_RECLAIMED_SEGMENT()));
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(before_log_addr)+1,GET_RECLAIMED_OFFSET()));
//	PRINT("\n verified logical address");

	/* verify we advanced to a reserve eu*/
	VERIFY(COMPARE(seg_map_ptr->is_eu_reserve, IS_EU_RESERVE_TRUE));
//	PRINT("\n verified reserve is marked in seg map");
	/* verify that the physical address is of the next free page is in a segment marked as reserve*/
	nandReadPageFlags(header_flags, GET_PAGE_SLOT_HEADER_ADDRESS(seg_map_ptr->reserve_eu_addr));	
	VERIFY(COMPARE(GET_SEG_TYPE_FLAG(header_flags), SEG_TYPE_RESERVE));
//	PRINT("\n verified physical address is in reserve segment");
	
	/* verify read data identical to written one */
	init_buf(sequencing_buffer);	
	
	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(next_log_addr));
//	readBlock(next_log_addr, sequencing_buffer);
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(sequencing_buffer[i], byte));
	}
//	PRINT("\n verified written data");
	/* verify the eu to which the page was written, was not marked in the page header as replacement*/
	VERIFY(COMPARE(GET_SLOT_EU_OFFSET(flags), SEQ_NO_SLOT_AND_EU_OFFSET));
	
	return 1;
}


/**
 * @brief
 * write to a segment when the flash is not full (not in reclamation mode).
 * write is to the end of a segment, forcing us to find and allocate a new segment.
 * should write the page, write header, checkpoint and advance pointer to a new segment
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest7(){
	uint32_t res, i, phy_addr, seg_num = 0, slot_id = 0, page_offset = SEQ_PAGES_PER_SLOT-1, 
	         new_slot_id = slot_id, sequence_num = 1, is_eu_reserve = IS_EU_RESERVE_FALSE ,nSegments = 1;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);	
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	
	init_logical_address(log_addr);	
	init_logical_address(prev_log_addr);	
	init_logical_address(next_log_addr);
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 SEQ_NO_SEGMENT_NUM, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	
	fsMemset(buf, byte, NAND_PAGE_SIZE);	
//	PRINT("\n allocAndWriteBlock() - about to allocAndWriteBlock");
	res = allocAndWriteBlock(next_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);
	VERIFY(!res);
//	PRINT("\n  allocAndWriteBlock successful");
	
	/* verify that indeed the physical address is advanced by more than one*/
	VERIFY(!COMPARE(phy_addr+1, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	
	/* verify logical address - 
	 * segment should change, and so does the offset*/	 
	VERIFY(!COMPARE(GET_LOGICAL_SEGMENT(log_addr),GET_RECLAIMED_SEGMENT()));
	VERIFY(!COMPARE(GET_LOGICAL_OFFSET(log_addr)+1,GET_RECLAIMED_OFFSET()));		
//	PRINT("\n  verified logical address");
	
	/* read written data*/	
	init_buf(sequencing_buffer);
	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(next_log_addr));	
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(sequencing_buffer[i], byte));
	}	
//	PRINT("\n  verified data");
	
	/* verify that what was written before was a checkpoint*/
	/* read written data*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);			
//	PRINT_MSG_AND_NUM("\nrec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nrec seg=", GET_RECLAIMED_SEGMENT());
	
	init_buf(sequencing_buffer);	
//	PRINT_MSG_AND_NUM("\nlog_addr offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nlog_addr seg=", GET_RECLAIMED_SEGMENT());
	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(log_addr));
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(flags), CP_NOT_PART_OF_CP));
//	PRINT("\n verified previous data is checkpoint");
	/* verify cpWritten*/
	VERIFY(cpWritten);
	
	/* verify expected changes in the segment map have occured- 
	 * - sequence number incremented
	 * - previously written segment changed
	 * - slot id changed
	 * - nSegments changed
	 */	 
	VERIFY(seg_map_ptr->sequence_num > sequence_num);
//	PRINT("\nverified seq num");
	VERIFY(COMPARE(seg_map_ptr->previously_written_segment, seg_num));
	VERIFY(!COMPARE(seg_map_ptr->new_slot_id,slot_id));
//	PRINT("\nverified new slot id");
//	PRINT_MSG_AND_NUM("\nGET_SEG_MAP_NSEGMENTS(seg_map_ptr)=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM("\nnSegments+1=",nSegments +1);	
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), nSegments+1));
//	PRINT("\n allocAndWriteBlock() - verified segment map changes");
	
	return 1;
}

/**
 * @brief
 * write to the end of a segment, forcing us to find and allocate a new segment, in the point we move from
 * regular allocation to reclamation (meaning, we now have to find a segment to reclaim).
 * should write the page, not truncate any segment, find a segment to reclaim, write header, 
 * checkpoint and advance pointer to the new segment.
 * (no copying required)
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest8(){
	uint32_t res, i, j,phy_addr, seg_num = SEQ_SEGMENTS_COUNT-1, slot_id = SEQ_SEGMENTS_COUNT-1, page_offset = SEQ_PAGES_PER_SLOT-1, 
	new_slot_id = slot_id, sequence_num = SEQ_SEGMENTS_COUNT, is_eu_reserve = IS_EU_RESERVE_FALSE , nSegments = SEQ_SEGMENTS_COUNT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);		
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	
	init_logical_address(log_addr);
	init_logical_address(prev_log_addr);	
	init_logical_address(next_log_addr);
	
	/* init all segments*/
	for(i=0; i < SEQ_SEGMENTS_COUNT; i++){
		seg_map_ptr->seg_to_slot_map[i] = i;
		SET_LOGICAL_SEGMENT(log_addr, i);		
		SET_LOGICAL_OFFSET(log_addr, 0);
		
		/* mark all first pages as obsolete to ensure no copying will be preformed*/
		for(j=0;j < 6; j++){
			markAsObsolete(log_addr, MARK_OBSOLETE_NOT_AFTER_REBOOT);
			SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)+1);			
		}
	}
	
	/* set all obsolete counters to 1*/
	set_obs_counters(OBS_COUNT_LEVEL_1);	
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 seg_num-1, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);	
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	
	fsMemset(buf, byte, NAND_PAGE_SIZE);	
	
	res = allocAndWriteBlock(next_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);
	VERIFY(!res);
//	PRINT("\nallocAndWriteBlock() successful");
	/* verify that indeed the physical address is not simply advanced by one*/
	VERIFY(!COMPARE(phy_addr+1, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	/* verify logical address - 
	 * segment should change, and so does the offset*/	 
	VERIFY(!COMPARE(GET_LOGICAL_SEGMENT(log_addr),GET_RECLAIMED_SEGMENT()));
	VERIFY(!COMPARE(GET_LOGICAL_OFFSET(log_addr)+1,GET_RECLAIMED_OFFSET()));	
//	PRINT("\nverified logical address");
	
	/* read written data*/
	init_buf(sequencing_buffer);	
//	nandReadTotalPage(sequencing_buffer, 63999);

	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(next_log_addr));
//	readBlock(next_log_addr, sequencing_buffer);
	for(i=0; i<NAND_PAGE_SIZE;i++){
//		PRINT_MSG_AND_HEX("\n1. sequencing_buffer[i]=",sequencing_buffer[i]);
		VERIFY(COMPARE(sequencing_buffer[i], byte));
	}		
//	PRINT("\nverified read data");
	
	/* verify that what was written bgefore was a checkpoint*/
	/* read written data*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());	
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);		
	
	init_buf(sequencing_buffer);
	nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr));	
//	readBlock(log_addr, sequencing_buffer);
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverified checkpoint was written");
	/* verify cpWritten*/
	VERIFY(cpWritten);
//	PRINT("\nverified cpWritten");
	/* verify expected changes in the segment map have occured- 
	 * - sequence number incremented
	 * - previously written segment changed
	 * - slot id changed
	 * - nSegments not changed
	 */	
	VERIFY(COMPARE(seg_map_ptr->previously_written_segment, seg_num));
//	PRINT("\nverified previously_written_segment");
	VERIFY(!COMPARE(seg_map_ptr->new_slot_id,slot_id));	
//	PRINT("\nverified new_slot_id");
//	PRINT_MSG_AND_NUM("\nGET_SEG_MAP_NSEGMENTS(seg_map_ptr)=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM("\nnSegments=",nSegments );
	VERIFY(COMPARE(seg_map_ptr->nSegments, nSegments));
//	PRINT("\nverified seg map");
	
//	PRINT_MSG_AND_NUM("\nGET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr)=",GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
//	PRINT_MSG_AND_NUM("\nsequence_num=",sequence_num );
	VERIFY(GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr) > sequence_num);
//	PRINT("\nverified seq num");
	return 1;
}

/**
 * @brief
 * write is to the end of a segment, forcing us to find and allocate a new segment (now in reclamation mode).
 * should write the page, find a segment to reclaim, write header, checkpoint and advance pointer to the new segment.
 * now we will mark only the k'th page of every segment as obsolete. this is to ensure it will not be copied
 * and that whatever segment we choose to reclaim, the reclaimed address in the segment map will be in offset k
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest9(){
	uint32_t res, i, k = 8, phy_addr, seg_num = SEQ_SEGMENTS_COUNT-1, slot_id = seg_num, page_offset = SEQ_PAGES_PER_SLOT-1, 
	new_slot_id = SEQ_SEGMENTS_COUNT, sequence_num = SEQ_SEGMENTS_COUNT, is_eu_reserve = IS_EU_RESERVE_FALSE , nSegments = SEQ_SEGMENTS_COUNT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];	
	
	init_logical_address(prev_log_addr);	
	init_logical_address(log_addr);
	init_logical_address(next_log_addr);	
	
	SET_LOGICAL_OFFSET(log_addr, k);
	/* init all segments*/
	for(i=0; i < SEQ_SEGMENTS_COUNT; i++){
		seg_map_ptr->seg_to_slot_map[i] = i;
		SET_LOGICAL_SEGMENT(log_addr, i);		
				
		/* mark all first pages as obsolete to ensure no copying will be preformed*/
		markAsObsolete(log_addr, MARK_OBSOLETE_NOT_AFTER_REBOOT);
	}	
	
	/* set all obsolete counters to 1*/
	set_obs_counters(OBS_COUNT_LEVEL_1);	
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 seg_num-1, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);	
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	
	fsMemset(buf, byte, NAND_PAGE_SIZE);	
	
	res = allocAndWriteBlock(next_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);
	VERIFY(!res);
//	PRINT("\nallocAndWriteBlock() success");
	/* verify that indeed the physical address is not simply advanced by one*/
	VERIFY(!COMPARE(phy_addr+1, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\nverified phy_addr");
	/* verify logical address - 
	 * segment should change, and so does the offset*/	 
	VERIFY(!COMPARE(GET_LOGICAL_SEGMENT(log_addr),GET_RECLAIMED_SEGMENT()));
//	PRINT("\nverified logical address segment");
//	PRINT_MSG_AND_NUM("\nk=",k);
//	PRINT_MSG_AND_NUM("\nSEQ_SEG_MAP_RECLAIMED_ADDR_PTR(test_seg_map_ptr)->page_offset)=",SEQ_SEG_MAP_RECLAIMED_ADDR_PTR(test_seg_map_ptr)->page_offset);
	VERIFY(COMPARE(k,GET_RECLAIMED_OFFSET()));	
//	PRINT("\nverified logical address");

	/* read written data*/
	init_buf(sequencing_buffer);	
	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(next_log_addr));	
//	readBlock(next_log_addr, sequencing_buffer);
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(sequencing_buffer[i], byte));
	}	
//	PRINT("\nverified read data");
	
	/* verify cpWritten*/
	VERIFY(cpWritten);
//	PRINT("\nverified checkpoint");
	/* verify expected changes in the segment map have occured- 
	 * - sequence number incremented
	 * - previously written segment changed
	 * - slot id changed
	 * - nSegments not changed
	 */	 
	VERIFY(seg_map_ptr->sequence_num > sequence_num);
//	PRINT("\nverified seq num");
	VERIFY(COMPARE(seg_map_ptr->previously_written_segment, seg_num));
//	PRINT("\nverified previously_written_segment");
	VERIFY(COMPARE(seg_map_ptr->nSegments, nSegments));
//	PRINT("\nverified nSegments");
//	PRINT_MSG_AND_NUM("\ntest_seg_map_ptr->new_slot_id=", test_seg_map_ptr->new_slot_id);
	VERIFY(COMPARE(seg_map_ptr->new_slot_id,slot_id));	
//	PRINT("\nverified new_slot_id");
	
//	PRINT("\nverified seg map");
	return 1;
}

/**
 * @brief
 * write to a segment when the flash is not full (not in reclamation mode).
 * write is to the end of EU, forcing the test of the next EU whetehr it is valid.
 * next eu is not valid, so a reserve EU should be allocated.
 * However ALL reserve EU's are taken, so an error should be returned 
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest10(){
	uint32_t reserve_phy_addr, res, i, phy_addr, seg_num = 5, slot_id = 2, page_offset = NAND_PAGES_PER_ERASE_UNIT-1, new_slot_id = slot_id, sequence_num = 3, is_eu_reserve = IS_EU_RESERVE_FALSE;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	INIT_FLAGS_STRUCT_AND_PTR(header_flags);
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	
	init_logical_address(log_addr);	
	init_logical_address(next_log_addr);	
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
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	/* mark next EU as abd*/
	markEuAsMockBad(phy_addr+1);
	
	init_flags(header_flags);
	
	/* mark all reserve EU's as taken for some address*/	
	while(1){
		reserve_phy_addr = allocReserveEU(header_flags, 0);
		if(reserve_phy_addr == SEQ_PHY_ADDRESS_EMPTY){
//			PRINT("\nno more reserve EUs");
			break;
		}				
		
		SET_SLOT_EU_OFFSET(header_flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(0));
		VERIFY(!nandProgramPageC(CAST_TO_UINT8(header_flags), reserve_phy_addr, 0, NAND_SPARE_SIZE));
		init_flags(header_flags);
	}	
	
//	PRINT("\nmarked reserve EU's");
	
	fsMemset(buf, byte, NAND_PAGE_SIZE);	
	
	/* allocate*/
	res = allocAndWriteBlock(next_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);	
	VERIFY(COMPARE(res,ERROR_NO_RESERVE_EU));
//	PRINT("\nallocAndWriteBlock success");
	/* TODO: behaviour not yet defined 26.6.07*/
	/* verify we didn't simply advance to next physical page*/
	VERIFY(!COMPARE(phy_addr+1, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	
	return 1;
	
	/* verify logical address - 
	 * only offset should increment by 1*/	 
	VERIFY(COMPARE(GET_LOGICAL_SEGMENT(log_addr),GET_RECLAIMED_SEGMENT()));
	VERIFY(COMPARE(GET_LOGICAL_OFFSET(log_addr)+1,GET_RECLAIMED_OFFSET()));
	
//	PRINT("\nverified logical address");
	/* verify we advanced to a reserve eu*/
	VERIFY(COMPARE(seg_map_ptr->is_eu_reserve, IS_EU_RESERVE_TRUE));
	
	/* verify that the physical address is of the next free page is in a segment marked as reserve*/
	nandReadPageFlags(header_flags, GET_PAGE_SLOT_HEADER_ADDRESS(seg_map_ptr->reserve_eu_addr));	
	VERIFY(COMPARE(GET_SEG_TYPE_FLAG(header_flags), SEG_TYPE_RESERVE));
	
	/* verify written data is identical to read*/
	init_buf(sequencing_buffer);	
//	readBlock(next_log_addr, sequencing_buffer);
	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(next_log_addr));
	for(i=0; i<NAND_PAGE_SIZE;i++){
		VERIFY(COMPARE(sequencing_buffer[i], byte));
	}	
//	PRINT("\nverified read data");
	/* verify the eu to which the page was written, was marked in the page header as replacement for the original*/
	VERIFY(COMPARE(GET_SLOT_EU_OFFSET(flags), CALCULATE_SLOT_ID_AND_EU_OFFSET(phy_addr+1)));
	
	return 1;
}

/**
 * @brief
 * write to a segment, in reclamation mode.
 * write is to the end of a segment, forcing us to find and allocate a new segment.
 * we force choosing a segment to reclaim with obs counter level high, but actually no obsolete pages.
 * should write the page, mark another segment for writing.
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest11(){
	uint32_t res, first_obs_page=8, i, phy_addr, seg_num = 0, slot_id = 0, page_offset = SEQ_PAGES_PER_SLOT-1, new_slot_id = SEQ_SEGMENTS_COUNT, sequence_num = 1,
	is_eu_reserve = IS_EU_RESERVE_FALSE ,nSegments = SEQ_SEGMENTS_COUNT, real_full_seg = 5, real_full_seg_org_slot;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	
	init_logical_address(log_addr);	
	init_logical_address(prev_log_addr);	
	init_logical_address(next_log_addr);	
	
	/* init all segments*/	
	SET_LOGICAL_OFFSET(log_addr, first_obs_page);	
	for(i=0; i <= SEQ_SEGMENTS_COUNT; i++){
		seg_map_ptr->seg_to_slot_map[i] = i;		
		
		/* mark a page as obsolete in all segment except real_full_seg*/
		if(i == real_full_seg)
			continue;
		
		SET_LOGICAL_SEGMENT(log_addr, i);									
		markAsObsolete(log_addr, MARK_OBSOLETE_NOT_AFTER_REBOOT);
	}	
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 5, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	real_full_seg_org_slot = seg_map_ptr->seg_to_slot_map[real_full_seg];
	
	set_obs_counters(OBS_COUNT_LEVEL_0);
	/* change the level of one segment to be really higher than the rest*/
	change_counter_level(OBS_COUNT_LEVEL_5,real_full_seg);	
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	
	fsMemset(buf, byte, NAND_PAGE_SIZE);	
	
	res = allocAndWriteBlock(next_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);
	VERIFY(!res);
//	PRINT("\nallocAndWriteBlock() success");
//	
//	PRINT_MSG_AND_NUM("\nreal_full_seg_org_slot=",real_full_seg_org_slot);
//	PRINT_MSG_AND_NUM("\ntest_seg_map_ptr->new_slot_id=",seg_map_ptr->new_slot_id);
	/* what was the old real full slot, is now the one written to*/
	VERIFY(COMPARE(real_full_seg_org_slot, seg_map_ptr->new_slot_id));
//	PRINT("\nreal_full_seg_org_slot is new slot id");
	VERIFY(!COMPARE(real_full_seg, GET_RECLAIMED_SEGMENT()));	
//	PRINT("\nreclaimed segment is not real_full_seg");
	VERIFY(COMPARE(first_obs_page, GET_RECLAIMED_OFFSET()));	
//	PRINT("\nfirst_obs_page success");	
	
	return 1;
}

/**
 * @brief
 * write to a segment when the flash is full (not in reclamation mode).
 * write is to the end of a segment, forcing us to find and allocate a new segment.
 * should write the page and return error
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest12(){
	uint32_t res, i, phy_addr, seg_num = SEQ_SEGMENTS_COUNT-1, slot_id = seg_num, page_offset = SEQ_PAGES_PER_SLOT-1, new_slot_id = slot_id, 
	sequence_num = 300, is_eu_reserve = IS_EU_RESERVE_FALSE ,nSegments = SEQ_SEGMENTS_COUNT;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	
	init_logical_address(log_addr);	
	init_logical_address(prev_log_addr);	
	init_logical_address(next_log_addr);	
	
	for(i=0; i < SEQ_SEGMENTS_COUNT; i++){
		seg_map_ptr->seg_to_slot_map[i] = i;					
	}
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 SEQ_NO_SEGMENT_NUM, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);
	
	/* we try to copy all segments*/
	set_obs_counters(OBS_COUNT_LEVEL_0);
	
	/* copy logical address from seg map, and calcualte the matching physical address*/	
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	
	fsMemset(buf, byte, NAND_PAGE_SIZE);	
	
	res = allocAndWriteBlock(next_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);
	VERIFY(COMPARE(res,ERROR_FLASH_FULL));
	
	return 1;
}

/**
 * @brief
 * in regulat allocation mode 
 * - allocate the last page
 * - mark pages as obsolete
 * 
 * verify obsolete pages count increase, and free pages counter  
 * @return 1 if succesful, 0 otherwise
 */
error_t allocAndWriteBlockTest13(){		
	uint32_t res, i, j,phy_addr, seg_num = SEQ_SEGMENTS_COUNT-1, slot_id = SEQ_SEGMENTS_COUNT-1, page_offset = SEQ_PAGES_PER_SLOT-1, 
	new_slot_id = slot_id, sequence_num = SEQ_SEGMENTS_COUNT, is_eu_reserve = IS_EU_RESERVE_FALSE , nSegments = SEQ_SEGMENTS_COUNT;
	uint32_t old_obs_count;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(next_log_addr);	
	bool_t cpWritten;
	uint8_t byte = 'c', buf[NAND_TOTAL_SIZE];
	
	init_logical_address(log_addr);
	init_logical_address(prev_log_addr);
	
	/* init all segments*/
	for(i=0; i < SEQ_SEGMENTS_COUNT; i++){
		seg_map_ptr->seg_to_slot_map[i] = i;
				
		SET_LOGICAL_SEGMENT(log_addr, i);
		SET_LOGICAL_OFFSET(log_addr, 0);
		/* mark all first pages as obsolete to ensure no copying will be preformed*/
		for(j=0;j < 6; j++){
			markAsObsolete(log_addr, MARK_OBSOLETE_NOT_AFTER_REBOOT);
			SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)+1);			
		}
	}
	
	/* set all obsolete counters to 1*/
	set_obs_counters(OBS_COUNT_LEVEL_1);	
	
	set_seg_map(seg_map_ptr,
				 slot_id,
				 page_offset, // notice-  page offset is last page written!
				 seg_num,
				 sequence_num,				 
				 new_slot_id,
				 seg_num-1, // previously reclaimed segment
				 is_eu_reserve,
				 nSegments,
				 SEQ_PHY_ADDRESS_EMPTY);	
	
	/* copy logical address from seg map, and calcualte the matching physical address*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	phy_addr = CALC_ADDRESS(slot_id,0,page_offset);
	
	fsMemset(buf, byte, NAND_PAGE_SIZE);
	/* set free counter for 1, last refualt page allocated */	
	SET_FREE_COUNTER(1);
	old_obs_count = GET_OBS_COUNT();
	res = allocAndWriteBlock(next_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);
	VERIFY(!res);
	
//	PRINT("\nallocAndWriteBlock() successful");
	/* verify that indeed the physical address is not simply advanced by one*/
	VERIFY(!COMPARE(phy_addr+1, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
	/* verify logical address - 
	 * segment should change, and so does the offset*/	 
	VERIFY(!COMPARE(GET_LOGICAL_SEGMENT(log_addr),GET_RECLAIMED_SEGMENT()));
	VERIFY(!COMPARE(GET_LOGICAL_OFFSET(log_addr)+1,GET_RECLAIMED_OFFSET()));	
//	PRINT("\nverified logical address");
	
	/* read written data*/
	init_buf(sequencing_buffer);	
//	nandReadTotalPage(sequencing_buffer, 63999);

	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(next_log_addr));
//	readBlock(next_log_addr, sequencing_buffer);
	for(i=0; i<NAND_PAGE_SIZE;i++){
//		PRINT_MSG_AND_HEX("\n1. sequencing_buffer[i]=",sequencing_buffer[i]);
		VERIFY(COMPARE(sequencing_buffer[i], byte));
	}		
//	PRINT("\nverified read data");
	
	/* verify that what was written bgefore was a checkpoint*/
	/* read written data*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);		
	
	init_buf(sequencing_buffer);
	nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr));	
//	readBlock(log_addr, sequencing_buffer);
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverified checkpoint was written");
	/* verify cpWritten*/
	VERIFY(cpWritten);
//	PRINT("\nverified cpWritten");
	/* verify expected changes in the segment map have occured- 
	 * - sequence number incremented
	 * - previously written segment changed
	 * - slot id changed
	 * - nSegments not changed
	 */	
	 
	VERIFY(COMPARE(GET_SEG_MAP_PREV_SEG(seg_map_ptr), seg_num));
//	PRINT("\nverified previously_written_segment");
	VERIFY(!COMPARE(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr),slot_id));	
//	PRINT("\nverified new_slot_id");
//	PRINT_MSG_AND_NUM("\nGET_SEG_MAP_NSEGMENTS(seg_map_ptr)=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM("\nnSegments=",nSegments );
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), nSegments));
//	PRINT("\nverified seg map");
	
//	PRINT_MSG_AND_NUM("\nGET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr)=",GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
//	PRINT_MSG_AND_NUM("\nsequence_num=",sequence_num );
	VERIFY(GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr) > sequence_num);
//	PRINT("\nverified seq num");	
	
//	PRINT_MSG_AND_NUM("\nold_obs_count=",old_obs_count);
//	PRINT_MSG_AND_NUM("\nnew obs count=",GET_OBS_COUNT());
	/* verify counters */
	VERIFY(COMPARE(old_obs_count-1, GET_OBS_COUNT()));	
//	PRINT_MSG_AND_NUM("\nnew free count=",GET_FREE_COUNTER());
	VERIFY(COMPARE(0, GET_FREE_COUNTER()));
//	PRINT("\nfirst_obs_page success");	

	/* now do another allocAndWriteBlock(), and make sure obs count decreased, and free count is still 0*/
	/* do allocAndWriteBlock() */
	fsMemset(buf, byte, NAND_PAGE_SIZE);	 
	res = allocAndWriteBlock(next_log_addr, buf, 0, prev_log_addr, &cpWritten, checkpointWriter,0);
	VERIFY(!res);
	
	/* verify counters */
//	PRINT_MSG_AND_NUM("\nold_obs_count=",old_obs_count);
//	PRINT_MSG_AND_NUM("\nnew obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(old_obs_count-2, GET_OBS_COUNT()));
//	PRINT_MSG_AND_NUM("\nnew free count=",GET_FREE_COUNTER());
	VERIFY(COMPARE(0, GET_FREE_COUNTER()));
	
	return 1;
}
