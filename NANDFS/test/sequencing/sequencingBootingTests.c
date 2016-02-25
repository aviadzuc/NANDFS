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

/** @file sequencingBootingTests.c  */
#include <test/sequencing/sequencingBootingTests.h>
#include <test/sequencing/testsHeader.h>

extern uint8_t fs_data[MOCK_FS_DATA_SIZE];
#ifdef Profiling
#include <profilingTests.h>
#endif
/**
 * run all sequencingBooting tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllSequencingBootingTests(){
	RUN_TEST(sequencingBooting,1);
	RUN_TEST(sequencingBooting,2);
	RUN_TEST(sequencingBooting,3);
	RUN_TEST(sequencingBooting,4);
	RUN_TEST(sequencingBooting,5);
	RUN_TEST(sequencingBooting,7);
	RUN_TEST(sequencingBooting,8);
	RUN_TEST(sequencingBooting,9);
	RUN_TEST(sequencingBooting,10);
	RUN_TEST(sequencingBooting,11);
	RUN_TEST(sequencingBooting,12);
	RUN_TEST(sequencingBooting,13);
	RUN_TEST(sequencingBooting,14);
	RUN_TEST(sequencingBooting,15);
	RUN_TEST(sequencingBooting,16);
	RUN_TEST(sequencingBooting,17);
	RUN_TEST(sequencingBooting,18);
	RUN_TEST(sequencingBooting,19);
	RUN_TEST(sequencingBooting,20);
	RUN_TEST(sequencingBooting,21);
	RUN_TEST(sequencingBooting,22);
	RUN_TEST(sequencingBooting,23);
	RUN_TEST(sequencingBooting,24);
	RUN_TEST(sequencingBooting,25);
	RUN_TEST(sequencingBooting,26);

	return 0;
}

/**
 * @brief
 * init sequencingBooting test
 */
error_t init_sequencingBootingTest(){
	if(nandInit())
		return -1;

	initializeSequencingStructs();
	init_flash();
	init_buf(sequencing_buffer);
	return 1;
}

/**
 * @brief
 * tear down sequencingBooting test
 */
error_t tearDown_sequencingBootingTest(){
	initializeSequencingStructs();
	init_flash();
	nandTerminate();
	init_buf(sequencing_buffer);

	return 1;
}

/**
 * @brief
 * the flash is empty.
 * booting should detect it, and initialize the file system. i.e:
 * 1) mark reserve segments
 * 2) mark first segment and it's checkpoint
 * 3) prepare for writing
 *
 * @return 1 if successful, 0 if a read/write error occured
 */
error_t sequencingBootingTest1(){
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	bool_t pendingVOTs = 1;
	uint32_t cp_size, expected_free_count, expected_obs_count;

	init_logical_address(log_addr);

	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting success");

//	nandReadPageFlags(flags, 2);
//	PRINT_MSG_AND_NUM("\nsequencingBootingTest1() - GET_CHECKPOINT_FLAG(flags)=",GET_CHECKPOINT_FLAG(flags));
	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

//	PRINT("\nreserve segments marking success");
	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
//	PRINT_MSG_AND_NUM("\nGET_SEG_MAP_SLOT_FOR_SEG(0)=",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,0));
	VERIFY(COMPARE(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,0),0));
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));
//	PRINT("\nfirst data segmnet success");

	/* verify previous page is of a checkpoint*/
//	PRINT_MSG_AND_NUM("\nGET_RECLAIMED_OFFSET()=",GET_RECLAIMED_OFFSET());
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
//	PRINT_MSG_AND_NUM("\nread page success from ",logicalAddressToPhysical(log_addr));
//	PRINT_MSG_AND_NUM("\nGET_CHECKPOINT_FLAG(flags)=",GET_CHECKPOINT_FLAG(flags));
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\ncheckpoint success");
//	PRINT_MSG_AND_NUM("\nGET_SEG_MAP_NSEGMENTS(seg_map_ptr)=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 1));

	/* verify no pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,0));

	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	/* verify reserve fields - we returned to previous segment which has no bad EU's*/
	VERIFY(!COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));
	VERIFY(COMPARE(SEQ_PHY_ADDRESS_EMPTY,GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr)));

	VERIFY(GET_SEG_MAP_NSEGMENTS(seg_map_ptr)==1);

	expected_free_count = SEQ_SEGMENTS_COUNT * (SEQ_PAGES_PER_SLOT-cp_size);
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));
	return 1;
}

/**
 * @brief
 * we crashed during stage (1) of initialization.
 * booting should detect it, delete header of last written reserve segment
 * and continue initialization
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest2(){
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	bool_t pendingVOTs = 1;
	uint32_t cp_size, expected_free_count, expected_obs_count;

	init_logical_address(log_addr);

	/* set only first reserve segment */
	page_hdr_ptr->segment_id   = SEQ_FIRST_RESERVE_SEGMENT;
	page_hdr_ptr->sequence_num = 1;
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_RESERVE);
	SET_SLOT_EU_OFFSET(seq_flags, SEQ_RESERVE_EU_IS_HEADER);

	VERIFY(!nandProgramTotalPage(sequencing_buffer, CALC_ADDRESS(SEQ_FIRST_RESERVE_SEGMENT,0,0)));
	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting success");

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nreserve segments success");

	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));
//	PRINT("\nseg header success");
	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));

//	nandReadPageFlags(flags, 2);
//	PRINT_MSG_AND_NUM("\nsequencingBootingTest2() - GET_CHECKPOINT_FLAG(flags)=",GET_CHECKPOINT_FLAG(flags));

	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	/* verify no pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,0));

	/* verify reserve fields - we returned to previous segment which has no bad EU's*/
	VERIFY(!COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));
	VERIFY(COMPARE(SEQ_PHY_ADDRESS_EMPTY,GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr)));

	VERIFY(GET_SEG_MAP_NSEGMENTS(seg_map_ptr)==1);

	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	expected_free_count = SEQ_SEGMENTS_COUNT * (SEQ_PAGES_PER_SLOT-cp_size);
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during stage (2) of initialization.
 * booting should detect it, delete header of first segment and
 * continue initialization
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest3(){
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	bool_t pendingVOTs = 1;
	uint32_t cp_size, expected_free_count, expected_obs_count;

	init_logical_address(log_addr);

	/* mark reserve segment*/
	mark_reserve_segment_headers();

	/* write header of segment without checkpoint*/
	init_buf(sequencing_buffer);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr,3);
	SET_HEADER_SEGMENT_ID(page_hdr_ptr,0);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);

	VERIFY(!nandProgramTotalPage(sequencing_buffer, SEQ_FIRST_RESERVE_SEGMENT));

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nsequencingBooting() success");
	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));

//	nandReadPageFlags(flags, 2);
//	PRINT_MSG_AND_NUM("\nsequencingBootingTest2() - GET_CHECKPOINT_FLAG(flags)=",GET_CHECKPOINT_FLAG(flags));

	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));

	/* verify no pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,0));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	/* verify reserve fields - we returned to previous segment which has no bad EU's*/
	VERIFY(!COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));
	VERIFY(COMPARE(SEQ_PHY_ADDRESS_EMPTY,GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr)));

	VERIFY(GET_SEG_MAP_NSEGMENTS(seg_map_ptr)==1);

	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	expected_free_count = SEQ_SEGMENTS_COUNT * (SEQ_PAGES_PER_SLOT-cp_size);
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during regular allocation (tests usage of allocAndWriteBlock() also).
 * last EU allocated is regular. booting should:
 * - detect it
 * - do a copy back
 * - find latest checkpoint
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest4(){
	uint32_t i;
	uint8_t byte = 'r';
	bool_t cpWritten;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	/* mock file system data. real one is extracted from checkpointWriter */
	bool_t pendingVOTs = 1;
	uint32_t cp_size, expected_free_count, expected_obs_count;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #1 success");
	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;

	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));
	/* do slot size allocations so the next segment must be allocated*/
	for(i=0; i<SEQ_PAGES_PER_SLOT; i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0));
	}
//	PRINT("\nslot size allocations success");
	/* reboot*/
	mock_reboot();

	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));
	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 1);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));
//	PRINT("\nverify first segment header success");
	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

//	PRINT_MSG_AND_NUM("\nGET_RECLAIMED_SEGMENT()=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM("\nGET_RECLAIMED_OFFSET()=",GET_RECLAIMED_OFFSET());
	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),1));
//	PRINT("\nverify reclaimed segment success");
//	PRINT_MSG_AND_NUM("\nGET_RECLAIMED_OFFSET()=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(", expected=",1+CALCULATE_IN_PAGES(MOCK_FS_DATA_SIZE)+SEQ_CHECKPOINT_PAGES_COUNT);
//	PRINT_MSG_AND_NUM(", CALCULATE_IN_PAGES(MOCK_FS_DATA_SIZE)=",CALCULATE_IN_PAGES(MOCK_FS_DATA_SIZE));
//	PRINT_MSG_AND_NUM(", SEQ_CHECKPOINT_PAGES_COUNT=",SEQ_CHECKPOINT_PAGES_COUNT);
//	PRINT_MSG_AND_NUM(", (sizeof(obs_counters_map))=",(sizeof(obs_counters_map)));

	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),1+CALCULATE_IN_PAGES(MOCK_FS_DATA_SIZE+SEQ_CHECKPOINT_SIZE)));
//	PRINT("\nverify reclaimed offset success");

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
//	PRINT_MSG_AND_NUM("\nlogicalAddressToPhysical(log_addr)=",logicalAddressToPhysical(log_addr));
//	nandReadPageFlags(flags, 514);
//	PRINT_MSG_AND_NUM("\nsequencingBootingTest4() - GET_CHECKPOINT_FLAG(flags)=",GET_CHECKPOINT_FLAG(seq_flags));

	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 2));
//	PRINT("\nverify NSEGMENTS success");

	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverify cp flag success");

	/* verify pending VOTs
	 * we do NOT continue to search backwards in prev seg if we have a valid checkpoint*/
	VERIFY(COMPARE(pendingVOTs,1));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	/* verify reserve fields - we returned to previous segment which has no bad EU's*/
	VERIFY(!COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));
	VERIFY(COMPARE(SEQ_PHY_ADDRESS_EMPTY,GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr)));

	expected_free_count = (SEQ_SEGMENTS_COUNT-2) * (SEQ_PAGES_PER_SLOT-cp_size)+(SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET());
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during regular allocation, when writing new header for segment #1. last EU allocated is regular.
 * booting should:
 * - detect it
 * - do a copy back
 * - findCheckpointAndTruncate()
 * - return to previous segment
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest5(){
	uint32_t i;
	uint8_t byte = 'r';
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	uint32_t cp_size, expected_free_count, expected_obs_count;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #1 success");
	/* do (slot size-1) allocations*/
	for(i=GET_RECLAIMED_OFFSET(); i<SEQ_PAGES_PER_SLOT-1; i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0));
	}

	/* write last page*/
	init_buf(sequencing_buffer);
	fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);
//	PRINT_MSG_AND_NUM("\nGET_RECLAIMED_SEGMENT()=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM("\nGET_RECLAIMED_SEGMENT_SLOT()=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM("\nGET_RECLAIMED_OFFSET()=",GET_RECLAIMED_OFFSET());
	VERIFY(!nandProgramTotalPage(sequencing_buffer, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));
//	PRINT("\nlast program success");
	/* write header of segment without checkpoint*/
	init_buf(sequencing_buffer);

	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr,GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr)+1);
	SET_HEADER_SEGMENT_ID(page_hdr_ptr,1);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);
	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
	VERIFY(!nandProgramTotalPage(sequencing_buffer, CALC_ADDRESS(1,0,0)));
//	PRINT("\nheader program success");
	/* reboot*/
	mock_reboot();

	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");
	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));

	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(), 0));

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));

	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 1));
//	PRINT("\nverified nsegments");
	/* verify no pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,0));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	/* verify reserve fields - we returned to previous segment which has no bad EU's*/
	VERIFY(!COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));
	VERIFY(COMPARE(SEQ_PHY_ADDRESS_EMPTY,GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr)));

	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	expected_free_count = (SEQ_SEGMENTS_COUNT-1) * (SEQ_PAGES_PER_SLOT-cp_size)+(SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET());
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during regular allocation, when writing new header for segment. last EU allocated is reserve.
 * booting should:
 * - detect it
 * - do a copy back
 * - findCheckpointAndTruncate
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest7(){
	uint32_t i, reserve_eu_addr;
	uint8_t byte = 'r';
	bool_t cpWritten, pendingVOTs = 1;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags,sequencing_buffer);
	uint32_t cp_size, expected_free_count, expected_obs_count;
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #1 success");

	/* do (slot size-1) allocations*/
	for(i=GET_RECLAIMED_OFFSET(); i<SEQ_PAGES_PER_SLOT-1; i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		if(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0))
			return 1;
	}

//	PRINT("\nslot size allocs success");
	/* write last page*/
	VERIFY(!nandProgramTotalPage(sequencing_buffer, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	/* mark first EU of next segent as bad*/
	VERIFY(markEuAsMockBad(CALC_ADDRESS(1,0,0)));
//	PRINT("\nmark eu as bad success");
	/* write header of segment without checkpoint*/
	init_buf(sequencing_buffer);
	reserve_eu_addr = allocReserveEU(flags,CALC_ADDRESS(1,0,0));

	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr,GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
	SET_HEADER_SEGMENT_ID(page_hdr_ptr,1);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_RESERVE);
	SET_SLOT_EU_OFFSET(seq_flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(CALC_ADDRESS(1,0,0)));
	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
	VERIFY(!nandProgramTotalPage(sequencing_buffer, reserve_eu_addr));

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting#2 success");
	/* verify the reserve EU is now empty*/
	VERIFY(!nandReadPageTotal(sequencing_buffer, reserve_eu_addr));
	for(i=0;i<NAND_TOTAL_SIZE;i++){
		VERIFY(COMPARE(sequencing_buffer[i],0xff));
	}

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nverified reserve segs");
	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));

	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 1));

	/* verify no pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,0));
//	PRINT("\nverified pendingVOTS");
	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());
//	PRINT("\nverified no cb");
	/* verify reserve fields - we returned to previous segment which has no bad EU's*/
	VERIFY(!COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));
	VERIFY(COMPARE(SEQ_PHY_ADDRESS_EMPTY,GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr)));

	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),0));
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),cp_size));

	expected_free_count = (SEQ_SEGMENTS_COUNT-1) * (SEQ_PAGES_PER_SLOT-cp_size)+(SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET());
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * the flash is empty. first EU is bad, and so is the first EU of the first reserve segment
 * booting should detect it, and initialize the file system. i.e:
 * 1) mark reserve segments
 * 2) mark first segment and it's checkpoint
 * 3) prepare for writing
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest8(){
	uint32_t i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	bool_t pendingVOTs = 1;
	uint32_t cp_size, expected_free_count, expected_obs_count;

	init_logical_address(log_addr);

	/* mark both EU's as bad*/
	VERIFY(markEuAsMockBad(CALC_ADDRESS(SEQ_FIRST_RESERVE_SEGMENT,0,0)));
	VERIFY(markEuAsMockBad(0));

	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting success");
	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));

	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr),1));

	/* verify no pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,0));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	/* verify reserve fields - we returned to previous segment which has no bad EU's*/
	VERIFY(COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));

	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),0));
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),cp_size));
//	PRINT_MSG_AND_NUM("\ncp_size=",cp_size);
//	PRINT_MSG_AND_NUM("\nrec offset=",GET_RECLAIMED_OFFSET());
	expected_free_count = (SEQ_SEGMENTS_COUNT-1) * (SEQ_PAGES_PER_SLOT-cp_size)+(SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET());
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief *
 * we crashed during stage (1) of initialization.
 * first EU is bad, and so is the first EU of the first reserve segment
 * booting should detect it, delete header of last written reserve segment
 * and continue initialization
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest9(){
	uint32_t phy_addr, i;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
//	INIT_PAGE_AREA_PTR_TO_BUFFER(page_hdr_ptr);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	bool_t pendingVOTs = 1;
	uint32_t cp_size, expected_free_count, expected_obs_count;

	init_logical_address(log_addr);

	/* mark both EU's as bad*/
	VERIFY(markEuAsMockBad(CALC_ADDRESS(SEQ_FIRST_RESERVE_SEGMENT,0,0)));
	VERIFY(markEuAsMockBad(0));

	/* set only first reserve segment */
	phy_addr = get_valid_eu_addr_in_location(SEQ_FIRST_RESERVE_SEGMENT,1);
	page_hdr_ptr->segment_id   = SEQ_FIRST_RESERVE_SEGMENT;
	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	page_hdr_ptr->sequence_num = GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_RESERVE);
	SET_SLOT_EU_OFFSET(seq_flags, SEQ_RESERVE_EU_IS_HEADER);

	nandProgramTotalPage(sequencing_buffer, phy_addr);
	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting success");

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));

	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr),1));

	/* verify no pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,0));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	/* verify reserve fields - we returned to previous segment which has no bad EU's*/
	VERIFY(COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));

	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),0));
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),cp_size));
//	PRINT_MSG_AND_NUM("\ncp_size=",cp_size);
//	PRINT_MSG_AND_NUM("\nrec offset=",GET_RECLAIMED_OFFSET());
	expected_free_count = (SEQ_SEGMENTS_COUNT-1) * (SEQ_PAGES_PER_SLOT-cp_size)+(SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET());
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during stage (2) of initialization.
 * first EU is bad, and so is the first EU of the first reserve segment.
 * booting should detect it, delete header of first segment and
 * continue initialization
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest10(){
	uint32_t i, phy_addr;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	bool_t pendingVOTs = 1;
	uint32_t cp_size, expected_free_count, expected_obs_count;

	init_logical_address(log_addr);

	/* mark both EU's as bad*/
	VERIFY(markEuAsMockBad(CALC_ADDRESS(SEQ_FIRST_RESERVE_SEGMENT,0,0)));
	VERIFY(markEuAsMockBad(0));

	/* mark reserve segment*/
	mark_reserve_segment_headers();

	/* write header of segment without checkpoint*/
	//phy_addr = allocReserveEU(flags, 0);
	phy_addr = allocReserveEU(flags, 0);
	VERIFY(!COMPARE(phy_addr,SEQ_PHY_ADDRESS_EMPTY));

	/* imitate header write to reserve eu*/
	init_buf(sequencing_buffer);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr,3);
	SET_HEADER_SEGMENT_ID(page_hdr_ptr,0);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);
	SET_SLOT_EU_OFFSET(seq_flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(0));
	VERIFY(!nandProgramTotalPage(sequencing_buffer, phy_addr));

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));

	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));

	/* verify no pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,0));
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_TRUE));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),0));
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),cp_size));
//	PRINT_MSG_AND_NUM("\ncp_size=",cp_size);
//	PRINT_MSG_AND_NUM("\nrec offset=",GET_RECLAIMED_OFFSET());
	expected_free_count = (SEQ_SEGMENTS_COUNT-1) * (SEQ_PAGES_PER_SLOT-cp_size)+(SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET());
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during regular allocation. last EU allocated is reserve.
 * booting should:
 * - detect it
 * - do a copy back
 * - find latest checkpoint
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest11(){
	uint32_t i;
	uint8_t byte = 'r';
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t cp_size, expected_free_count, expected_obs_count;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* mark second EU of segment #1 as bad*/
	markEuAsMockBad(CALC_ADDRESS(1,0,0));

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #1 success");

	/* do slot size allocations so the next segment must be allocated*/
	for(i=0; i<SEQ_PAGES_PER_SLOT; i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}

//	PRINT_MSG_AND_NUM("\nslot size allocations success. rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nverified reserve segments succes");
	SET_LOGICAL_SEGMENT(log_addr, 1);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));
//	PRINT("\nverified seg header succes");
	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));

	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverified cp flag succes");
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 2));
//	PRINT("\nverified nsegments");
	/* verify reserve attributes*/
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_TRUE));
//	PRINT("\nverified reserve flag");
	SET_LOGICAL_OFFSET(log_addr,0);
	SET_LOGICAL_SEGMENT(log_addr,1);
	VERIFY(COMPARE(GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr), logicalAddressToPhysical(log_addr)));
//	PRINT("\nverified reserve fields");
	/* verify no pending VOTs (we dont search in previous segments)*/
	VERIFY(COMPARE(pendingVOTs,1));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),1));
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),cp_size));
//	PRINT_MSG_AND_NUM("\ncp_size=",cp_size);
//	PRINT_MSG_AND_NUM("\nrec offset=",GET_RECLAIMED_OFFSET());
	expected_free_count = (SEQ_SEGMENTS_COUNT-2) * (SEQ_PAGES_PER_SLOT-cp_size)+(SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET());
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during regular allocation, when writing new header for segment #1. it's first EU is reserve.
 * booting should:
 * - detect it
 * - do a copy back
 * - findCheckpointAndTruncate()
 * - detect system is empty
 * - initialize
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest12(){
	uint32_t i, phy_addr;
	uint8_t byte = 'r';
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	uint32_t cp_size, expected_free_count, expected_obs_count;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #1 success");

	/* do (slot size-1) allocations*/
	for(i=GET_RECLAIMED_OFFSET(); i<SEQ_PAGES_PER_SLOT-1; i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		if(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0))
			return 1;
	}
	/* write last page*/
	VERIFY(!nandProgramTotalPage(sequencing_buffer, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())));

	/* write header of segment without checkpoint*/
	markEuAsMockBad(CALC_ADDRESS(1,0,0));
	init_buf(sequencing_buffer);

	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr,GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
	SET_HEADER_SEGMENT_ID(page_hdr_ptr,1);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);
	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));

	phy_addr = allocReserveEU(flags, 0);
	VERIFY(!COMPARE(phy_addr, SEQ_PHY_ADDRESS_EMPTY));
	VERIFY(!nandProgramTotalPage(sequencing_buffer, phy_addr));

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));

	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 1));

	/* verify no pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,0));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),0));
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),cp_size));
//	PRINT_MSG_AND_NUM("\ncp_size=",cp_size);
//	PRINT_MSG_AND_NUM("\nrec offset=",GET_RECLAIMED_OFFSET());
	expected_free_count = (SEQ_SEGMENTS_COUNT-1) * (SEQ_PAGES_PER_SLOT-cp_size)+(SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET());
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed after writing last page in regular allocation. we have a checkpoint halfway thru the segment.
 * booting should:
 * - detect it
 * - findCheckpointAndTruncate()
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest13(){
	uint32_t k=5, i;
	uint8_t byte = 'r', fs_data[MOCK_FS_DATA_SIZE];
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	uint32_t expected_rec_offset, obs_level=OBS_COUNT_LEVEL_1;
	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #1 success");
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* do slot size allocations so the next segment must be allocated*/
	for(i=0; i<SEQ_PAGES_PER_SLOT; i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}

//	PRINT_MSG_AND_NUM("\nafter first slot size allocs, rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segt=",GET_RECLAIMED_SEGMENT());

	/* mark third EU as bad in reclaimed slot*/
	markEuAsMockBad(CALC_ADDRESS(GET_RECLAIMED_SEGMENT_SLOT(),2,0));

	/* write until we reach it*/
	while(GET_RECLAIMED_OFFSET() < NAND_PAGES_PER_ERASE_UNIT * 2){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}

	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr),IS_EU_RESERVE_TRUE));
//	PRINT_MSG_AND_NUM("\nafter 2 EU's allocs, reclaimed offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());

	/* do another commit*/
	set_obs_counters(obs_level);
//	PRINT_MSG_AND_NUM("\nb4 second commit, obs count=",GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM("\nb4 second commit, rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nMOCK_FS_DATA_SIZE=",MOCK_FS_DATA_SIZE);
//	PRINT("\nabout to commit");
	fsMemset(fs_data,byte,MOCK_FS_DATA_SIZE);
	VERIFY(!(commit(fs_data,MOCK_FS_DATA_SIZE, 0)));
//	PRINT("\ncommit success");
	expected_rec_offset = GET_RECLAIMED_OFFSET();


	/* keep writing k pages*/
	for(i = GET_RECLAIMED_OFFSET(); GET_RECLAIMED_OFFSET()-i <k;){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}
//	PRINT_MSG_AND_NUM("\nafter k allocs, reclaimed offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nafter k allocs, reclaimed segemnet=",GET_RECLAIMED_SEGMENT());

	/*** REBOOT ***/
	mock_reboot();
//	PRINT("\nbooting #2 starting");
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");
//	PRINT_MSG_AND_NUM("\nafter sequencingBooting() obs count=",GET_OBS_COUNT());
	/* verify obs counters*/
	VERIFY(verify_obs_counters(obs_level));
//	PRINT("\nobs counter verified");

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 1);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));
//	PRINT("\nverified seg header");

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));

//	PRINT("\nverified prev page");
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 2));

//	PRINT_MSG_AND_NUM("\nreclaimed offset=",GET_RECLAIMED_OFFSET());
	/* verify no pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,0));
//	PRINT("\nverified pending VOTs");

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

//	PRINT_MSG_AND_NUM("\ncp_size=",cp_size);
//	PRINT_MSG_AND_NUM("\nrec offset=",expected_rec_offset);
	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),1));
	/* verify seg map offset is now at the second checkpoint, not the first one (in the header)*/
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),expected_rec_offset));

	/* verify obs and free counters*/
//	uint32_t expected_free_count = (SEQ_SEGMENTS_COUNT-2) * (SEQ_PAGES_PER_SLOT-cp_size)+(SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET());
//	uint32_t expected_obs_count  = 0;
////	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
////	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
////	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
////	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
//	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during reclamation, when writing new header for a newly reclaimed segment. first EU is regular.
 * booting should:
 * - detect it
 * - copyback
 * - findCheckpointAndTruncate()
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest14(){
	uint32_t i, seg_to_reclaim = 0;
	uint32_t cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	uint8_t byte = 'r', buf[NAND_TOTAL_SIZE];
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	uint32_t expected_free_count;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT_MSG_AND_NUM("\nbooting #1 success. rec offset=",GET_RECLAIMED_OFFSET());
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

	/* mark obs counters levels*/
	set_obs_counters(OBS_COUNT_LEVEL_1);
//	PRINT("\nset obs counters success");
	/* write SEQ_PAGES_PER_SLOT pages*/
	init_buf(buf);
	fsMemset(buf,byte,NAND_PAGE_SIZE);
	for(i=0;i < SEQ_PAGES_PER_SLOT*2;i++){
		VERIFY(!(allocAndWriteBlock(log_addr, buf, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}
//	PRINT("\nALLOCS success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());

	/* write headers for all segments*/
	writeSimpleSegmentHeaders(3);
//	PRINT("\nverified segment headers success");

	/* write header of segment without checkpoint*/
	init_buf(sequencing_buffer);

	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr,GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
	seg_map_ptr->new_slot_id = SEQ_SEGMENTS_COUNT;
	/* reclaim segment #1*/
	SET_HEADER_SEGMENT_ID(page_hdr_ptr,seg_to_reclaim);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
	VERIFY(!nandProgramTotalPage(sequencing_buffer, CALC_ADDRESS(SEQ_SEGMENTS_COUNT,0,0)));

	expected_free_count = GET_FREE_COUNTER();
//	PRINT("\nobs markings success");

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");

	/* verify we are not in reclamation -
	 * we returned to previous segment start*/
	VERIFY(!IS_STATE_RECLAMATION());

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nverified reserve seg headers");

	/* verify first page, in all segments*/
	SET_LOGICAL_OFFSET(log_addr, 0);
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
//		PRINT_MSG_AND_NUM("\ni=", i);
//		PRINT_MSG_AND_HEX(" slot=", GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, i));
		SET_LOGICAL_OFFSET(log_addr, 0);
		VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
//		PRINT("\nread page");
		VERIFY(verify_seg_header(i, SEG_TYPE_USED));
	}
//	PRINT("\nverified regular seg headers");
//	PRINT_MSG_AND_NUM("\nrec offset=",GET_RECLAIMED_OFFSET());

	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

//	PRINT("\nverified rec offset is obs_page");
	/* verify slot we are allocating from*/
	/* verify previous page is not of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverified cp flag");

//	PRINT_MSG_AND_HEX("\nreserve flag=",GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr));
	VERIFY(COMPARE(IS_EU_RESERVE_FALSE, GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));
//	PRINT("\nverified reserve flag");
	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 1));
//	PRINT("\nverified pending");
	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),SEQ_SEGMENTS_COUNT-1));
	/* verify seg map offset is now at the second checkpoint, not the first one (in the header)*/
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),cp_size));

	/* verify obs and free counters*/
//	expected_free_count = SEQ_PAGES_PER_SLOT-GET_RECLAIMED_OFFSET();/* old free count + prev written slot pages which were erased */
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",0);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	/* notice - 0 obsolete pages, becuase they were all marked after we wrote last checkpoint...*/
	VERIFY(COMPARE(GET_OBS_COUNT(),0));

	return 1;
}

/**
 * @brief
 * we crashed during reclamation, when writing new header for the new segment. it's first EU is reserve.
 * booting should:
 * - detect it
 * - copyback
 * - findCheckpointAndTruncate()
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest15(){
	uint32_t i, slot_phy_addr, seg_to_reclaim = 0, bad_eu_addr = CALC_ADDRESS(SEQ_SEGMENTS_COUNT,0,0);
	uint8_t byte = 'r', buf[NAND_TOTAL_SIZE];
	bool_t cpWritten, pendingVOTs = 1;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	uint32_t expected_free_count, cp_size;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* mark eu as bad in expected new generation slot*/
	markEuAsMockBad(bad_eu_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #1 success");
	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

	/* mark obs counters levels*/
	set_obs_counters(OBS_COUNT_LEVEL_1);

//	PRINT("\nset obs counters success");
	fsMemset(buf,byte,NAND_PAGE_SIZE);
	/* write SEQ_PAGES_PER_SLOT pages*/
	for(i=0;i < SEQ_PAGES_PER_SLOT*2;i++){
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}
//	PRINT("\nALLOCS success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());

	/* write headers for all segments*/
	VERIFY(!writeSimpleSegmentHeaders(3));

	/* write header of segment without checkpoint*/
	init_buf(sequencing_buffer);

	/* reclaim segment #1*/
	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr,GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
	SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, SEQ_SEGMENTS_COUNT);

	SET_HEADER_SEGMENT_ID(page_hdr_ptr,seg_to_reclaim);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));

	slot_phy_addr = allocReserveEU(flags, bad_eu_addr);
	VERIFY(!COMPARE(slot_phy_addr,SEQ_PHY_ADDRESS_EMPTY));
	VERIFY(!nandProgramTotalPage(sequencing_buffer, slot_phy_addr));

	expected_free_count = GET_FREE_COUNTER();
//	PRINT("\nobs markings success");

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");

	/* verify offset*/
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));
//	PRINT("\nverified rec offset");

	/* verify not in reclamation*/
	VERIFY(!IS_STATE_RECLAMATION());

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nverified reserve segs headers");

	/* verify first page, in all segments*/
	SET_LOGICAL_OFFSET(log_addr, 0);
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
		VERIFY(verify_seg_header(i, SEG_TYPE_USED));
	}
//	PRINT("\nverified regular reserve segs");

	/* verify slot we are allocating from*/
	/* verify previous page is of a checkpoint*/
	SET_LOGICAL_SEGMENT(log_addr, GET_RECLAIMED_SEGMENT());
	SET_LOGICAL_OFFSET(log_addr, GET_RECLAIMED_OFFSET()-1);
//	PRINT_MSG_AND_NUM("\nabout to read cp flag from page ",logicalAddressToPhysical(log_addr));
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverified cp page");
	/* verify reserve fields - we returned to previous segment which has no bad EU's*/
	VERIFY(!COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));
	VERIFY(COMPARE(SEQ_PHY_ADDRESS_EMPTY,GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr)));
//	PRINT("\nverified reserve flags");

	/* verify obs counters levels*/
	VERIFY(verify_obs_counters(OBS_COUNT_LEVEL_1));

	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 1));

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),SEQ_SEGMENTS_COUNT-1));
	/* verify seg map offset is now at the second checkpoint, not the first one (in the header)*/
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),cp_size));

	/* verify obs and free counters*/
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	/* notice - 0 obsolete pages, becuase they were all marked after we wrote last checkpoint...*/
	VERIFY(COMPARE(GET_OBS_COUNT(),0));

	return 1;
}

/**
 * @brief
 * we crashed during reclamation. last written EU is regular.
 * the new generation checkpoint is complete. we should return to reclamation state
 * booting should:
 * - detect it
 * - copyback
 * - findCheckpointAndTruncate()
 * - move to next page to be reclaimed
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest16(){
	uint32_t i, seg_to_reclaim = 0, obs_page = NAND_PAGES_PER_ERASE_UNIT - 3;
	uint8_t byte = 'r', obs_level = OBS_COUNT_LEVEL_1, buf[NAND_TOTAL_SIZE];
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
	uint32_t expected_obs_count, cp_size, expected_free_count;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

	/* mark obs counters levels*/
	set_obs_counters(obs_level);

//	PRINT("\nset obs counters success");
	/* write SEQ_PAGES_PER_SLOT pages*/
	for(i=0;i < SEQ_PAGES_PER_SLOT*2;i++){
		init_buf(buf);
		fsMemset(buf,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, buf, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}
//	PRINT("\nALLOCS success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());

	/* write headers for all segments*/
	writeSimpleSegmentHeaders(3);

	/* mark pages as obsolete in all segments 1*/
	SET_LOGICAL_OFFSET(log_addr, obs_page);
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		markAsObsolete(log_addr, 0);
	}

//	PRINT("\nmarked obs pages");
	/* write header of segment without checkpoint*/
	init_buf(sequencing_buffer);

	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	/* reclaim segment */
	SET_HEADER_SEGMENT_ID(page_hdr_ptr,seg_to_reclaim);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);
	SET_HEADER_SEQUENCE_NUM(page_hdr_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
	VERIFY(!nandProgramTotalPage(sequencing_buffer, CALC_ADDRESS(SEQ_SEGMENTS_COUNT,0,0)));
	SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, SEQ_SEGMENTS_COUNT);
	SET_RECLAIMED_SEGMENT(seg_to_reclaim);
	SET_RECLAIMED_OFFSET(1);
//	PRINT("\nabout to commit mock header cp");

	/* write checkpoint */
	VERIFY(!commit(fs_data, MOCK_FS_DATA_SIZE, 1));
	expected_free_count = GET_FREE_COUNTER();
	expected_obs_count = GET_OBS_COUNT();

//	PRINT_MSG_AND_NUM("\nis state reclaim?=",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM("\n nsegments=",seg_map_ptr->nSegments);
//	PRINT_MSG_AND_NUM("\ncurrent reclaimed address=",logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
	/* reboot. we have */
	mock_reboot();

	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");

//	PRINT_MSG_AND_NUM("\n rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));

	/* verify we are still in reclamation*/
	VERIFY(IS_STATE_RECLAMATION());
//	PRINT("\nstate rec verified");
	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nreserve headers verified");

	/* verify first page, in all segments*/
	SET_LOGICAL_OFFSET(log_addr, 0);
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
		VERIFY(verify_seg_header(i, SEG_TYPE_USED));
	}
//	PRINT("\nregular headers verified");
	VERIFY(COMPARE(obs_page, GET_RECLAIMED_OFFSET()));
//	PRINT("\nrec offset verified");
	VERIFY(COMPARE(SEQ_SEGMENTS_COUNT,GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr)));
//	PRINT("\nnew slot id verified");
	/* verify slot we are allocating from*/
	/* verify previous page is not of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());

	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\nprev pagse not checkpoint verified");

	/* verify no resreve flags*/
	VERIFY(!COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));
	VERIFY(COMPARE(SEQ_PHY_ADDRESS_EMPTY,GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr)));
//	PRINT("\nreserve flags success");
	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());
//	PRINT("\nno cb success");
	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 1));
//	PRINT("\npendingVOTs success");

	/* verify seg map offset is now at the second checkpoint, not the first one (in the header)*/
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),obs_page));

	/* verify obs and free counters*/
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());

	/* notice - expected_obs_count-1, since only AFTER booting we reclaim a page*/
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count-1));
	return 1;
}

/**
 * @brief
 * we crashed during copyback, during regular allocation. last written EU is regular, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate()
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest17(){
	uint32_t offset, i;
	uint8_t byte = 'r', obs_level = OBS_COUNT_LEVEL_1, buf[NAND_TOTAL_SIZE];
	uint32_t phy_addr, cb_eu_addr, last_addr, max_offset, org_phy_addr_eu_start;
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t expected_free_count, expected_obs_count, cp_size;
	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #1 success");

	/* do slot size allocations so the next segment must be allocated*/
	fsMemset(buf,byte,NAND_PAGE_SIZE);
	for(i=0; i<SEQ_PAGES_PER_SLOT; i++){
		VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_addr, &cpWritten, checkpointWriter,0));
	}
//	PRINT("\nslot size allocations success")
	/* keep writing until EU #2*/
	for(i=GET_RECLAIMED_OFFSET();i<NAND_PAGES_PER_ERASE_UNIT*2;i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0));
	}
//	PRINT("\n2 eus allocations success")

	/* change obs counter levels*/
	for(i=0;i<=GET_RECLAIMED_SEGMENT();i++){
		change_counter_level(obs_level,i);
	}

	/* do last commit*/
//	PRINT("\nabout to commit");
//	PRINT_MSG_AND_NUM(" obs=",GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" free=",GET_FREE_COUNTER());
	VERIFY(!(commit(fs_data,MOCK_FS_DATA_SIZE, 0)));
//	PRINT("\nafter commit");
//	PRINT_MSG_AND_NUM(" obs=",GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" free=",GET_FREE_COUNTER());
	expected_obs_count = GET_OBS_COUNT();
	expected_free_count = GET_FREE_COUNTER();

//	PRINT("\nverified commit");
	offset   = GET_RECLAIMED_OFFSET();
	phy_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
//	PRINT_MSG_AND_NUM("\nperformed another commit. rec page is now ",phy_addr);

//	{
//		uint8_t temp_buf[NAND_TOTAL_SIZE], i;
//		nandReadPageTotal(temp_buf, 577);
//		PRINT_MSG_AND_NUM("\n",i);
//		PRINT_MSG_AND_HEX(". ",temp_buf[100]);
//		assert(temp_buf[100]==0x22);
//	}
	/* continue writing until almost end of EU*/
	for(i=GET_RECLAIMED_OFFSET();CALC_OFFSET_IN_EU(i)<NAND_MAX_EU_OFFSET;i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0));
	}

	/* create a copy back EU*/
	org_phy_addr_eu_start = CALC_EU_START(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
	cb_eu_addr = GET_COPYBACK_EU();
	last_addr  = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	max_offset = CALC_OFFSET_IN_EU(GET_RECLAIMED_OFFSET());

//	{
//	INIT_FLAGS_STRUCT_AND_PTR(flags);
//	nandReadPageFlags(flags, 578);
//	PRINT_MSG_AND_NUM("\nb4 test cb is page 578 used?=",IS_PAGE_USED(flags));
//
//	nandReadPageFlags(flags, 64546);
//	PRINT_MSG_AND_NUM(" 64546 used?=",IS_PAGE_USED(flags));
//	}

	for(i=0; i <= max_offset ; i++){
//		PRINT_MSG_AND_NUM("\ncopying to cb EU from address ", org_phy_addr_eu_start+i);
		VERIFY(!copyBackPage(cb_eu_addr+i, org_phy_addr_eu_start+i, 1, SEQ_PHY_ADDRESS_EMPTY));
	}

//	{
//	INIT_FLAGS_STRUCT_AND_PTR(flags);
//	nandReadPageFlags(flags, 578);
//	PRINT_MSG_AND_NUM("\nafter test cb is page 578 used?=",IS_PAGE_USED(flags));
//
//	nandReadPageFlags(flags, 64546);
//	PRINT_MSG_AND_NUM(" 64546 used?=",IS_PAGE_USED(flags));
//	}

//	PRINT("\nabout to erase original EU");
	/* erase original EU*/
	VERIFY(!nandErase(org_phy_addr_eu_start));

//	{
//		uint8_t temp_buf[NAND_TOTAL_SIZE], i;
//		nandReadPageTotal(temp_buf, 577);
//		PRINT_MSG_AND_NUM("\n",i);
//		PRINT_MSG_AND_HEX(". ",temp_buf[100]);
//		assert(temp_buf[100]==0x22);
//	}
//	expected_free_count = (SEQ_SEGMENTS_COUNT-2) * (SEQ_PAGES_PER_SLOT-cp_size) + SEQ_PAGES_PER_SLOT-offset;

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nsuccess booting #2");
	for(i=0;i<=GET_RECLAIMED_SEGMENT();i++){
		VERIFY(COMPARE(get_counter_level(i),obs_level));
	}

//	PRINT("\nverified obs counters");
	VERIFY(!IS_STATE_RECLAMATION());
//	PRINT("\nverified state not reclamation");

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nverified reserve headers");
	SET_LOGICAL_SEGMENT(log_addr, 1);
	SET_LOGICAL_OFFSET(log_addr, 0);

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverified cp flag");
//	PRINT_MSG_AND_NUM("\nnsegments=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 2));
//	PRINT("\nverified nsegments");
	VERIFY(COMPARE(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr), 1));
//	PRINT("\nverified new slot id");
	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(), 1));
//	PRINT("\nverified rec seg");

	/* verify we returned to after the commit*/
//	PRINT_MSG_AND_NUM("\nrec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nshould be=",offset);
//	PRINT_MSG_AND_NUM("\nexpected phy_addr=",phy_addr);
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),offset));
//	PRINT("\nverified rec offset");

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());
//	PRINT("\nverified no copyback EU");

	/* verify no resreve flags*/
	VERIFY(!COMPARE(IS_EU_RESERVE_TRUE,GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)));
	VERIFY(COMPARE(SEQ_PHY_ADDRESS_EMPTY,GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr)));

	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 0));
	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),1));

	/* verify obs and free counters*/
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));

	return 1;
}

/**
 * @brief
 * we crashed during copyback, during regular allocation. last written EU is reserve, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate()
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest18(){
	uint32_t offset, i;
	uint8_t byte = 'r';
	uint32_t reserve_addr, cb_eu_addr, org_phy_addr = CALC_ADDRESS(1,2,0), last_addr, max_offset, org_phy_addr_eu_start;
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t expected_free_count, expected_obs_count, cp_size;
	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	markEuAsMockBad(org_phy_addr);
//	PRINT_MSG_AND_NUM("\nsequencingBootingTest18 - b4 booting #1 is org_phy_addr valid?=",nandCheckEuStatus(org_phy_addr));
	VERIFY(!nandCheckEuStatus(org_phy_addr));
	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #1 success");
//	PRINT_MSG_AND_NUM("\nsequencingBootingTest18 - after booting #1 is org_phy_addr valid?=",nandCheckEuStatus(org_phy_addr));
	VERIFY(!nandCheckEuStatus(org_phy_addr));
	/* do slot size allocations so the next segment must be allocated*/
	for(i=0; i<SEQ_PAGES_PER_SLOT; i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0));
	}

	/* keep writing until EU #2*/
	for(i=GET_RECLAIMED_OFFSET();i<NAND_PAGES_PER_ERASE_UNIT*2;i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0));
	}
//	PRINT_MSG_AND_NUM("\nsequencingBootingTest18 - b4 commit is org_phy_addr valid?=",nandCheckEuStatus(org_phy_addr));
	VERIFY(!nandCheckEuStatus(org_phy_addr));

	/* do another commit*/
	VERIFY(!(commit(fs_data,MOCK_FS_DATA_SIZE, 0)));
	expected_obs_count  = GET_OBS_COUNT();
	expected_free_count = GET_FREE_COUNTER();
	offset = GET_RECLAIMED_OFFSET();
//	PRINT_MSG_AND_NUM("\nsequencingBootingTest18 - after commit is org_phy_addr valid?=",nandCheckEuStatus(org_phy_addr));
	VERIFY(!nandCheckEuStatus(org_phy_addr));

	/* save reserve EU address*/
	reserve_addr = GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr);
	VERIFY(!IS_PHY_ADDR_EMPTY(reserve_addr));
//	PRINT_MSG_AND_NUM("\nwe have a reserve eu for org_phy_addr ",reserve_addr);
//	PRINT_MSG_AND_NUM("\nsequencingBootingTest18 - b4 more allocs is org_phy_addr valid?=",nandCheckEuStatus(org_phy_addr));
	VERIFY(!nandCheckEuStatus(org_phy_addr));

	/* continue writing until almost end of EU*/
	for(i=CALC_OFFSET_IN_EU(GET_RECLAIMED_OFFSET());i<NAND_MAX_EU_OFFSET;i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0));
	}
//	PRINT_MSG_AND_NUM("\n finished more allocs. rec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nsequencingBootingTest18 -  is org_phy_addr valid?=",nandCheckEuStatus(org_phy_addr));
	VERIFY(!nandCheckEuStatus(org_phy_addr));

	/* create a copy back EU*/
	org_phy_addr_eu_start = CALC_EU_START(org_phy_addr);
	cb_eu_addr = GET_COPYBACK_EU();
	last_addr  = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
	max_offset = CALC_OFFSET_IN_EU(GET_RECLAIMED_OFFSET());

//	PRINT_MSG_AND_NUM("\ncopying to cb EU. cb eu=", cb_eu_addr);
//	PRINT_MSG_AND_NUM(" org_phy_addr=", org_phy_addr);
//	PRINT_MSG_AND_NUM(" org_phy_addr_eu_start=", org_phy_addr_eu_start);
//	PRINT_MSG_AND_NUM(" rec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" max_offset= ", max_offset);
	for(i=0; i <= max_offset ; i++){
//		PRINT_MSG_AND_NUM("\ncopying to cb EU from address ", reserve_addr+i);
//		PRINT_MSG_AND_NUM(" org phy_addr= ", org_phy_addr_eu_start+i);
//		PRINT_MSG_AND_NUM(" max_offset= ", max_offset);
		VERIFY(!copyBackPage(cb_eu_addr+i, reserve_addr+i, 1, org_phy_addr_eu_start));
	}

//	PRINT("\nabout to erase original EU");
	/* erase reserve EU (the replacement for the bad original*/
	VERIFY(!nandErase(reserve_addr));

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");
	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nverified reserve seg headers");
	SET_LOGICAL_SEGMENT(log_addr, 1);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 2));

	/* verify we returned to after the commit*/
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(),offset));
//	PRINT("\nverified offset");

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());
//	PRINT("\nverified no cb");

	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 0));
//	PRINT("\nverified pending VOTs");

	/* verify new slot id*/
	VERIFY(COMPARE(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr),1));;

	/* verify nsegemnts*/
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr),2));;

	/* verify reserve flags*/
	VERIFY(COMPARE(GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr), reserve_addr));
//	PRINT("\nverified reserve address");
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_TRUE));
//	PRINT("\nverified reserve flags");

	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 0));

	VERIFY(COMPARE(GET_RECLAIMED_SEGMENT(),1));
	/* verify obs and free counters*/
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	/* notice - 0 obsolete pages, becuase they were all marked after we wrote last checkpoint...*/
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));
	return 1;
}

/**
 * @brief
 * we crashed during copyback, during reclamation. last written EU is regular, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate()
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest19(){
	uint32_t i, obs_level = OBS_COUNT_LEVEL_1;
	uint8_t byte = 'r', buf[NAND_TOTAL_SIZE];
	uint32_t obs_page = 10, org_phy_addr, reserve_eu_addr, last_addr, max_offset, org_phy_addr_eu_start;
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t expected_free_count, expected_obs_count, cp_size;
	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

	/* mark obs counters levels*/
	set_obs_counters(obs_level);

	/* write SEQ_PAGES_PER_SLOT pages*/
	for(i=0;i < SEQ_PAGES_PER_SLOT;i++){
		init_buf(buf);
		fsMemset(buf,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, buf, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}

//	PRINT("\nallocs success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());

	/* write headers for all segments*/
	writeSimpleSegmentHeaders(2);

	/* mark 3 pages as obsolete in all segments */
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		SET_LOGICAL_OFFSET(log_addr, obs_page);
		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+1);
		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+2);
		markAsObsolete(log_addr, 0);
	}

	/* write last EU written regularely - forcing us to start reclaiming*/
	SET_RECLAIMED_SEGMENT(SEQ_SEGMENTS_COUNT-1);
	SET_RECLAIMED_OFFSET(SEQ_PAGES_PER_SLOT-1);

	init_buf(buf);
	fsMemset(buf,byte,NAND_PAGE_SIZE);
	VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_addr, &cpWritten, checkpointWriter,0));
	VERIFY(IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM("\nafter final allocs rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(", seg=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(", slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(", new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));

	/* create a copy back EU */
	/* first, set logical address to start of eu to be copybacked */
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr) & NAND_EU_MASK);

	org_phy_addr    = logicalAddressToPhysical(log_addr);
//	PRINT_MSG_AND_NUM("\ncreate copy back for eu ",org_phy_addr);

	reserve_eu_addr = GET_COPYBACK_EU();
//	PRINT_MSG_AND_NUM(" cb EU=",reserve_eu_addr);
	VERIFY(!IS_PHY_ADDR_EMPTY(reserve_eu_addr));
	last_addr  = logicalAddressToPhysical(log_addr);
	max_offset = CALC_OFFSET_IN_EU(GET_RECLAIMED_OFFSET());
	org_phy_addr_eu_start = CALC_EU_START(org_phy_addr);

	for(i=0; i <= max_offset ; i++){
//		PRINT_MSG_AND_NUM("\ncopying to reserve EU from address ", org_phy_addr_eu_start+i);
		VERIFY(!copyBackPage(reserve_eu_addr+i, org_phy_addr_eu_start+i, 1, SEQ_PHY_ADDRESS_EMPTY));
	}
//	PRINT("\nabout to erase original EU");
	/* erase original EU*/
	VERIFY(!nandErase(org_phy_addr));

	expected_free_count = GET_FREE_COUNTER();
	expected_obs_count  = GET_OBS_COUNT();

	/* reboot*/
//	PRINT("\nb4 sequencingBooting #2");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(", rec seg=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(", rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(", new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(", state rec=",IS_STATE_RECLAMATION());
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nsequencingBooting #2 success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(", rec seg=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(", rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(", new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(", state rec=",IS_STATE_RECLAMATION());
	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 1);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));
//	PRINT("\nverify_seg_header success");

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
//	PRINT_MSG_AND_NUM("\nGET_LOGICAL_OFFSET(log_addr)=", GET_LOGICAL_OFFSET(log_addr));
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
//	PRINT("\nread success");
	VERIFY(IS_STATE_RECLAMATION());
//	PRINT("\nstate rec success");
	VERIFY(COMPARE(	GET_RECLAIMED_OFFSET(),obs_page));
//	PRINT("\nverified reclamation");
	VERIFY(COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\ncp flag success");

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,1));
//	PRINT("\nverified pending VOTS");

	/* reserve flags*/
	VERIFY(COMPARE(GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr), SEQ_PHY_ADDRESS_EMPTY));
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_FALSE));
//	PRINT("\nverified reserve flags");

	/* verify obs and free counters*/
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());

	/* notice - 0 obsolete pages, becuase they were all marked after we wrote last checkpoint...*/
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during copyback, during reclamation. last written EU is reserve, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate()
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest20(){
	uint32_t i, obs_level = OBS_COUNT_LEVEL_1;
	uint8_t byte = 'r', buf[NAND_TOTAL_SIZE];
	uint32_t reserve_eu, bad_eu_addr = CALC_ADDRESS(SEQ_SEGMENTS_COUNT,0,0), obs_page = 10;
	uint32_t org_phy_addr, cb_eu_addr, last_addr, max_offset, org_phy_addr_eu_start;
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t expected_free_count, expected_obs_count, cp_size;
	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

//	PRINT_MSG_AND_NUM("\nbad_eu_addr=", bad_eu_addr);
	markEuAsMockBad(bad_eu_addr);
//	PRINT_MSG_AND_NUM("\nafter marking check bad_eu_addr eu status bad=", !nandCheckEuStatus(bad_eu_addr));
	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

	/* mark obs counters levels*/
	set_obs_counters(obs_level);

	/* write SEQ_PAGES_PER_SLOT pages*/
	for(i=0;i < NAND_PAGES_PER_ERASE_UNIT;i++){
		init_buf(buf);
		fsMemset(buf,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, buf, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}

//	PRINT("\nallocs success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" bad_eu_addr eu status bad=", !nandCheckEuStatus(bad_eu_addr));
	/* write headers for all segments*/
	writeSimpleSegmentHeaders(1);

	/* mark 3 pages as obsolete in all segments */
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		SET_LOGICAL_OFFSET(log_addr, obs_page);
		markAsObsolete(log_addr, 0);
//		SET_LOGICAL_OFFSET(log_addr, obs_page+1);
//		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+2);
		markAsObsolete(log_addr, 0);
	}

	/* write last EU written regularely - forcing us to start reclaiming*/
	SET_RECLAIMED_SEGMENT(SEQ_SEGMENTS_COUNT-1);
	SET_RECLAIMED_OFFSET(SEQ_PAGES_PER_SLOT-1);

//	PRINT("\nprepare to do last regular allocation. ");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" state reclaim=",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" bad_eu_addr eu status bad=", !nandCheckEuStatus(bad_eu_addr));
	init_buf(buf);
	fsMemset(buf,byte,NAND_PAGE_SIZE);
	VERIFY(!allocAndWriteBlock(log_addr, buf, 0, prev_addr, &cpWritten, checkpointWriter,0));

//	PRINT_MSG_AND_NUM("\nafter final allocs offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" state reclaim=",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" bad_eu_addr eu status bad=", !nandCheckEuStatus(bad_eu_addr));
//	PRINT_MSG_AND_NUM(" NAND_BAD_EU_FLAG_BYTE_NUM=", NAND_BAD_EU_FLAG_BYTE_NUM);

	VERIFY(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)==IS_EU_RESERVE_TRUE);
	reserve_eu = GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr);

	/* create a copy back EU*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr) & NAND_EU_MASK);

	org_phy_addr    = logicalAddressToPhysical(log_addr);
//	PRINT_MSG_AND_NUM("\ncreate copy back for eu ",org_phy_addr);

	cb_eu_addr = GET_COPYBACK_EU();
	VERIFY(!IS_PHY_ADDR_EMPTY(cb_eu_addr));
	last_addr  = logicalAddressToPhysical(log_addr);
	max_offset = CALC_OFFSET_IN_EU(GET_RECLAIMED_OFFSET());
	org_phy_addr_eu_start = CALC_EU_START(org_phy_addr);

	for(i=0; i <= max_offset ; i++){
//		PRINT_MSG_AND_NUM("\ncopying to reserve EU from address ", reserve_eu+i);
		if(copyBackPage(cb_eu_addr+i, reserve_eu+i, 1, bad_eu_addr))
			return 1;
	}

//	PRINT("\nabout to erase original EU");
	/* erase original EU*/
	VERIFY(!nandErase(reserve_eu));
	expected_obs_count  = GET_OBS_COUNT();
	expected_free_count = GET_FREE_COUNTER();
	/* REBOOT */
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 1);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, and that a checkpoint was written before it*/
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));

	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
	VERIFY(IS_STATE_RECLAMATION());
	VERIFY(COMPARE(	GET_RECLAIMED_OFFSET(),obs_page));
//	PRINT("\nverified reclamation");
	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());

	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs,1));
//	PRINT("\nverified pending VOTS");

	/* reserve flags*/
	VERIFY(COMPARE(GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr), reserve_eu));
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_TRUE));
//	PRINT("\nverified reserve flags");

	/* verify obs and free counters*/
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));

	/* notice - 0 obsolete pages, becuase they were all marked after we wrote last checkpoint...*/
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}


/**
 * @brief
 * we crashed during copyback, during regular allocation of very first EU. last written EU is regular, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate()
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest21(){
	uint32_t i, max_write_offset = NAND_PAGES_PER_ERASE_UNIT-2, org_phy_addr, cb_eu_addr, max_offset = NAND_MAX_EU_OFFSET,
	org_phy_addr_eu_start, initial_rec_offset;
	uint8_t byte = 'r';
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t expected_free_count, expected_obs_count, cp_size;
	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
	initial_rec_offset = GET_RECLAIMED_OFFSET();

	/* do some allocations */
	for(i=GET_RECLAIMED_OFFSET(); i< max_offset; i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0));

	}

	max_offset = GET_RECLAIMED_OFFSET()-1;

	/* create a copy back EU*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	org_phy_addr    = logicalAddressToPhysical(log_addr);
	cb_eu_addr = GET_COPYBACK_EU();

	org_phy_addr_eu_start = CALC_EU_START(org_phy_addr);

	for(i=0; i <= max_write_offset ; i++){
//		PRINT_MSG_AND_NUM("\ncopying to cb EU from address ", org_phy_addr_eu_start+i);
		VERIFY(!copyBackPage(cb_eu_addr+i, org_phy_addr_eu_start+i, 1, SEQ_PHY_ADDRESS_EMPTY));
	}

//	PRINT("\nabout to erase original EU");
	/* erase original EU*/
	VERIFY(!nandErase(org_phy_addr));

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");
	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));
//	PRINT("\nverified seg header");
	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
//	PRINT("\nverified read");
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverified cp flag");
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 1));
//	PRINT("\nverified nsegments");
	VERIFY(COMPARE(initial_rec_offset, GET_RECLAIMED_OFFSET()));
//	PRINT("\nverified seg map fields");
	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());
//	PRINT("\nverified no cb");
	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 0));

	/* reserve flags*/
	VERIFY(COMPARE(GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr), SEQ_PHY_ADDRESS_EMPTY));
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_FALSE));

	expected_free_count = SEQ_SEGMENTS_COUNT * (SEQ_PAGES_PER_SLOT-cp_size);
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during copyback, during regular allocation of very first EU. last written EU is reserve, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate()
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest22(){
	uint32_t i, reserve_eu_phy_addr, max_write_offset = NAND_PAGES_PER_ERASE_UNIT-2, org_phy_addr, cb_eu_addr,
	max_offset = NAND_MAX_EU_OFFSET,	org_phy_addr_eu_start, initial_rec_offset;
	uint8_t byte = 'r';
	bool_t cpWritten, pendingVOTs = 1;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t expected_free_count, expected_obs_count, cp_size;
	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* mark first eu as bad */
	markEuAsMockBad(0);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
	initial_rec_offset = GET_RECLAIMED_OFFSET();
//	PRINT("\nbooting #1 success");

	/* save reserve addr of eu 0*/
	reserve_eu_phy_addr = GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr);
	VERIFY(!IS_PHY_ADDR_EMPTY(reserve_eu_phy_addr))
//	PRINT("\nverified reserve address");
	/* do some allocations */
	for(i=GET_RECLAIMED_OFFSET(); i< max_offset; i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0));
	}
//	PRINT("\nverified allocs");
	max_offset = GET_RECLAIMED_OFFSET()-1;

	/* create a copy back EU*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	org_phy_addr    = logicalAddressToPhysical(log_addr);
	VERIFY(COMPARE(CALC_EU_START(org_phy_addr), reserve_eu_phy_addr));
//	PRINT("\nverified org_phy_addr");
	cb_eu_addr = GET_COPYBACK_EU();

	org_phy_addr_eu_start = CALC_EU_START(org_phy_addr);

	for(i=0; i <= max_write_offset ; i++){
//		PRINT_MSG_AND_NUM("\ncopying to cb EU from address ", org_phy_addr_eu_start+i);
		VERIFY(!copyBackPage(cb_eu_addr+i, org_phy_addr_eu_start+i, 1, 0));
	}

//	PRINT("\nabout to erase original EU");
	/* erase original EU*/
	VERIFY(!nandErase(org_phy_addr));

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nbooting #2 success");
	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}

	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page*/
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(verify_seg_header(0, SEG_TYPE_USED));
//	PRINT("\nverified seg header");
	/* verify previous page is of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);

	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
//	PRINT("\nverified read");
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverified cp flag");
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), 1));
//	PRINT("\nverified nsegments");
	VERIFY(COMPARE(initial_rec_offset, GET_RECLAIMED_OFFSET()));
//	PRINT("\nverified seg map fields");
	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());
//	PRINT("\nverified no cb");
	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 0));

	/* reserve flags*/
	VERIFY(COMPARE(GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr), reserve_eu_phy_addr));
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_TRUE));

	expected_free_count = SEQ_SEGMENTS_COUNT * (SEQ_PAGES_PER_SLOT-cp_size);
	expected_obs_count  = 0;
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}




/**
 * @brief
 * we crashed during wear leveling. in the middle of copying the original slot, we reboot.
 * last EU we copied to is regular.
 * booting should:
 * - detect it
 * - delete incomplete slot
 * - find segment to allocate
 * - prepare for next allocation
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest23(){
	uint32_t i, obs_page = NAND_PAGES_PER_ERASE_UNIT - 3, obs_level=OBS_COUNT_LEVEL_1;
	uint32_t slot_to_level = 0, max_offset = NAND_PAGES_PER_ERASE_UNIT * 2, rec_offset;	uint8_t byte = 'r';
	bool_t cpWritten, pendingVOTs = 1;
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	uint32_t expected_free_count, expected_obs_count, cp_size;
	cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nsuccess booting #1");

#ifndef Profiling
	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));
//	PRINT("\nrec offset verified");
#endif

	/* save expected offset*/
	rec_offset = GET_RECLAIMED_OFFSET();

	/* mark obs counters levels*/
	set_obs_counters(obs_level);
//	PRINT("\nset obs counters");

	/* write SEQ_PAGES_PER_SLOT pages*/
	for(i=0;i < SEQ_PAGES_PER_SLOT*2;i++){
//		PRINT_MSG_AND_NUM("\nallocAndWriteBlock() #", i);
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}

//	PRINT("\nallocs success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());

	/* write headers for all segments*/
	writeSimpleSegmentHeaders(3);
	expected_free_count = GET_FREE_COUNTER();
	expected_obs_count  = GET_OBS_COUNT();

	/* save expected offset after next booting*/
	rec_offset = GET_RECLAIMED_OFFSET();

	/* mark 3 pages as obsolete in all segments */
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		SET_LOGICAL_OFFSET(log_addr, obs_page);
		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+1);
		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+2);
		markAsObsolete(log_addr, 0);
	}


	/* set new slot*/
	SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, SEQ_SEGMENTS_COUNT);
	/* perform wear leveling until half way through*/
	VERIFY(!performMockWearLeveling(slot_to_level, max_offset));
//	PRINT("\nsuccess mock wear leveling");

	/* reboot*/
#ifdef Profiling
	init_acces_acounters();
#endif
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
#ifdef Profiling
	return 1;
#endif
//	PRINT("\nsuccess booting #2");

	/* verify we are not in reclamation*/
	VERIFY(!IS_STATE_RECLAMATION());

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nsuccess reserve headers");
	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, in all segments*/

	SET_LOGICAL_OFFSET(log_addr, 0);
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
		VERIFY(verify_seg_header(i, SEG_TYPE_USED));
	}
//	PRINT("\nsuccess regular headers");
	VERIFY(COMPARE(rec_offset, GET_RECLAIMED_OFFSET()));

	/* verify slot we are allocating from*/
	/* verify previous page is not of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());

//	PRINT_MSG_AND_NUM("\nrec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nrec seg=", GET_RECLAIMED_SEGMENT());

	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
//	PRINT_MSG_AND_NUM("\nlog_addr offset=", GET_LOGICAL_OFFSET(log_addr));
//	PRINT_MSG_AND_NUM("\nlog_addr seg=", GET_LOGICAL_SEGMENT(log_addr));
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
//	printBlock(sequencing_buffer);
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(flags), CP_NOT_PART_OF_CP));
//	PRINT("\nsuccess cp flag");

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());
//	PRINT("\nsuccess no cb");

	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 1));
//	PRINT("\nsuccess pendingVOTs");

	/* verify nsegments*/
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), SEQ_SEGMENTS_COUNT));
//	PRINT("\nnsegments success");

	/* verify obs level*/
	VERIFY(verify_obs_counters(obs_level));

	/* verify reserve flags*/
	VERIFY(COMPARE(GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr), SEQ_PHY_ADDRESS_EMPTY));
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_FALSE));
//	PRINT("\nreserve flags success");

	/* verify the slot we tried to wear level to is erased
	 * and that the slot we tried to wear level is not*/
	VERIFY(verifySlotIsErased(SEQ_SEGMENTS_COUNT));
	VERIFY(!verifySlotIsErased(slot_to_level));

//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during wear leveling. in the middle of copying the original slot, we reboot.
 * last EU we copied to is reserve.
 * booting should:
 * - detect it
 * - delete incomplete slot
 * - find segment to allocate
 * - prepare for next allocation
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest24(){
	uint32_t i, obs_page = NAND_PAGES_PER_ERASE_UNIT - 3, obs_level=OBS_COUNT_LEVEL_1;
	uint32_t slot_to_level = 0, max_offset = NAND_PAGES_PER_ERASE_UNIT * 2, rec_offset;
	uint8_t byte = 'r';
	bool_t cpWritten, pendingVOTs = 1;
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	uint32_t cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	uint32_t expected_free_count, expected_obs_count;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* mark the last EU we will copy to as bad*/
	markEuAsMockBad(CALC_ADDRESS(SEQ_SEGMENTS_COUNT,0,max_offset));

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nsuccess booting #1");

	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

	/* save expected offset*/
	rec_offset = GET_RECLAIMED_OFFSET();

	/* mark obs counters levels*/
	set_obs_counters(obs_level);

	/* write SEQ_PAGES_PER_SLOT pages*/
	for(i=0;i < SEQ_PAGES_PER_SLOT*2;i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}

//	PRINT("\nallocs success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());

	/* write headers for all segments*/
	writeSimpleSegmentHeaders(3);
	expected_free_count = GET_FREE_COUNTER();
	expected_obs_count  = GET_OBS_COUNT();

	/* save expected offset after next booting*/
	rec_offset = GET_RECLAIMED_OFFSET();

	/* mark 3 pages as obsolete in all segments */
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		SET_LOGICAL_OFFSET(log_addr, obs_page);
		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+1);
		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+2);
		markAsObsolete(log_addr, 0);
	}

	/* set new slot*/
	SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, SEQ_SEGMENTS_COUNT);
	/* perform wear leveling until half way through*/
	VERIFY(!performMockWearLeveling(slot_to_level, max_offset));
//	PRINT("\nsuccess mock wear leveling");

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nsuccess booting #2");

	/* verify we are not in reclamation*/
	VERIFY(!IS_STATE_RECLAMATION());

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nsuccess reserve headers");
	SET_LOGICAL_SEGMENT(log_addr, 0);
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, in all segments*/

	SET_LOGICAL_OFFSET(log_addr, 0);
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
		VERIFY(verify_seg_header(i, SEG_TYPE_USED));
	}
//	PRINT("\nsuccess regular headers");
	VERIFY(COMPARE(rec_offset, GET_RECLAIMED_OFFSET()));
	/* verify previous page is not of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());

	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(flags), CP_NOT_PART_OF_CP));
//	PRINT("\nsuccess cp flag");

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());
//	PRINT("\nsuccess no cb");

	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 1));
//	PRINT("\nsuccess pendingVOTs");

	/* verify nsegments*/
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), SEQ_SEGMENTS_COUNT));
//	PRINT("\nnsegments success");

	/* verify obs level*/
	VERIFY(verify_obs_counters(obs_level));

	/* verify reserve flags*/
	VERIFY(COMPARE(GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr), SEQ_PHY_ADDRESS_EMPTY));
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_FALSE));

	/* verify the slot we tried to wear level to is erased
	 * and that the slot we tried to wear level is not*/
	VERIFY(verifySlotIsErased(SEQ_SEGMENTS_COUNT));
	VERIFY(!verifySlotIsErased(slot_to_level));

//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during erasure of old generation of reclaimed segment.
 * we crashed just before erasing a regular EU
 * booting should:
 * - detect it
 * - delete incomplete slot
 * - find segment to allocate
 * - prepare for next allocation
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest25(){
	uint32_t i, seg_to_reclaim = 0, obs_page = NAND_PAGES_PER_ERASE_UNIT - 3;
	uint32_t obs_level = OBS_COUNT_LEVEL_1, rec_offset, max_offset = NAND_PAGES_PER_ERASE_UNIT * 2;
	uint8_t byte = 'r';
	bool_t cpWritten, pendingVOTs;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	uint32_t expected_free_count, expected_obs_count;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nsuccess booting #1");

	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

	/* save expected offset*/
	rec_offset = GET_RECLAIMED_OFFSET();

	/* mark obs counters levels*/
	set_obs_counters(obs_level);

	/* write SEQ_PAGES_PER_SLOT pages*/
	for(i=0;i < SEQ_PAGES_PER_SLOT*2;i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}

//	PRINT("\nallocs success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());

	/* write headers for all segments*/
	writeSimpleSegmentHeaders(3);

//	PRINT("\nwrote segmnet headers");
//	PRINT_MSG_AND_NUM(" offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" state rec? ",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" new slot id=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));

	/* save expected offset after next booting*/
	rec_offset = GET_RECLAIMED_OFFSET();

	/* mark 3 pages as obsolete in all segments, and also the last page */
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		SET_LOGICAL_OFFSET(log_addr, obs_page);
		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+1);
		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+2);
		markAsObsolete(log_addr, 0);

		/* obsolete last page*/
		SET_LOGICAL_OFFSET(log_addr, SEQ_MAX_LOGICAL_OFFSET);
		markAsObsolete(log_addr, 0);
	}
	/* set new slot, and write it's header*/
	SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, SEQ_SEGMENTS_COUNT);
	SET_RECLAIMED_SEGMENT(seg_to_reclaim);
	SET_RECLAIMED_OFFSET(0);


	/* write reclaimed segmnet header*/
	expected_obs_count  = GET_OBS_COUNT();
	expected_free_count = GET_FREE_COUNTER();
	writeSegmentHeader(checkpointWriter);
//	PRINT("\nwrote rec segmnet header");
//	PRINT_MSG_AND_NUM(" offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" state rec? ",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" new slot id=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));

	/* write to fill segment*/
	while(GET_RECLAIMED_OFFSET() < SEQ_MAX_LOGICAL_OFFSET){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}
//	PRINT_MSG_AND_NUM("\nwrote to rec segmnet. offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" state rec? ",IS_STATE_RECLAMATION());

//	assert(0);
	/* write last page in new generation*/
	init_buf(sequencing_buffer);
	fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);
	VERIFY(!nandProgramTotalPage(sequencing_buffer,CALC_ADDRESS(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr),0,SEQ_MAX_LOGICAL_OFFSET)));

	/* and now erase old generation partially */
	for(i=CALC_ADDRESS(seg_to_reclaim,0,SEQ_PAGES_PER_SLOT-1);i-max_offset>NAND_PAGES_PER_ERASE_UNIT;i-=NAND_PAGES_PER_ERASE_UNIT){
		VERIFY(!nandErase(i))
	}

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nsequncing #2 success");
	/* verify we are not in reclamation*/
//	PRINT_MSG_AND_NUM("\n new slot id= ",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" nsegments= ",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec slot= ",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" rec segmnet= ",GET_RECLAIMED_SEGMENT());
	VERIFY(!IS_STATE_RECLAMATION());

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nverified reserve headers");
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, in all segments*/

	SET_LOGICAL_OFFSET(log_addr, 0);
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
		VERIFY(verify_seg_header(i, SEG_TYPE_USED));
	}
//	PRINT("\nverified regular headers");
	/* verify slot we are allocating from*/
	/* verify previous page is not of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());

	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverified cp flag");
	/* verify the segment we should have completed erasing is the one we write to now*/
	VERIFY(COMPARE(SEQ_SEGMENTS_COUNT, GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr)));
//	PRINT("\nverified new slot id");

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());
//	PRINT("\nsuccess no cb");

	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 1));
//	PRINT("\nsuccess pendingVOTs");

	/* verify nsegments*/
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), SEQ_SEGMENTS_COUNT));
//	PRINT("\nnsegments success");

	/* verify reserve flags*/
	VERIFY(COMPARE(GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr), SEQ_PHY_ADDRESS_EMPTY));
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_FALSE));

	VERIFY(verifySlotIsErased(0));

	/* verify obs and free counters*/
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}

/**
 * @brief
 * we crashed during erasure of old generation of reclaimed segment.
 * we crashed just before erasing a reserve EU
 * booting should:
 * - detect it
 * - delete incomplete slot
 * - find segment to allocate
 * - prepare for next allocation
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest26(){
	uint32_t i, seg_to_reclaim = 0, obs_page = NAND_PAGES_PER_ERASE_UNIT - 3;
	uint32_t obs_level = OBS_COUNT_LEVEL_1, rec_offset, max_offset = NAND_PAGES_PER_ERASE_UNIT * 2;
	uint8_t byte = 'r';
	bool_t cpWritten, pendingVOTs;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	uint32_t cp_size = CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+MOCK_FS_DATA_SIZE)+SEQ_SEG_HEADER_PAGES_COUNT;
	uint32_t expected_free_count, expected_obs_count;

	init_logical_address(log_addr);
	init_logical_address(prev_addr);
	markEuAsMockBad(CALC_ADDRESS(0,0,max_offset));

	/* boot the flash when it's empty*/
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nsuccess booting #1");

	VERIFY(COMPARE(GET_RECLAIMED_OFFSET(), cp_size));

	/* save expected offset*/
	rec_offset = GET_RECLAIMED_OFFSET();

	/* mark obs counters levels*/
	set_obs_counters(obs_level);

	/* write SEQ_PAGES_PER_SLOT pages*/
	for(i=0;i < SEQ_PAGES_PER_SLOT*2;i++){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}

//	PRINT("\nallocs success");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());

	/* write headers for all segments*/
	writeSimpleSegmentHeaders(3);
//	PRINT("\nwrote segmnet headers");
//	PRINT_MSG_AND_NUM(" offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" state rec? ",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" new slot id=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
	/* save expected offset after next booting*/
	rec_offset = GET_RECLAIMED_OFFSET();

	/* mark 3 pages as obsolete in all segments, and also the last page */
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		SET_LOGICAL_OFFSET(log_addr, obs_page);
		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+1);
		markAsObsolete(log_addr, 0);
		SET_LOGICAL_OFFSET(log_addr, obs_page+2);
		markAsObsolete(log_addr, 0);

		/* obsolete last page*/
		SET_LOGICAL_OFFSET(log_addr, SEQ_MAX_LOGICAL_OFFSET);
		markAsObsolete(log_addr, 0);
	}
	/* set new slot, and write it's header*/
	SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, SEQ_SEGMENTS_COUNT);
	SET_RECLAIMED_SEGMENT(seg_to_reclaim);
	SET_RECLAIMED_OFFSET(0);

	/* write reclaimed segmnet header*/
	expected_obs_count  = GET_OBS_COUNT();
	expected_free_count = GET_FREE_COUNTER();
	writeSegmentHeader(checkpointWriter);

//	PRINT("\nwrote rec segmnet header");
//	PRINT_MSG_AND_NUM(" offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" state rec? ",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" new slot id=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));

	/* write to fill segment*/
	while(GET_RECLAIMED_OFFSET() < SEQ_MAX_LOGICAL_OFFSET){
		init_buf(sequencing_buffer);
		fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
		VERIFY(!(allocAndWriteBlock(log_addr, sequencing_buffer, 0, prev_addr, &cpWritten, checkpointWriter,0)));
	}
//	PRINT_MSG_AND_NUM("\nwrote to rec segmnet. offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" state rec? ",IS_STATE_RECLAMATION());

//	assert(0);
	/* write last page in new generation*/
	init_buf(sequencing_buffer);
	fsMemset(sequencing_buffer,byte,NAND_PAGE_SIZE);
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_USED);
	VERIFY(!nandProgramTotalPage(sequencing_buffer,CALC_ADDRESS(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr),0,SEQ_MAX_LOGICAL_OFFSET)));

	/* and now erase old generation partially */
	for(i=CALC_ADDRESS(seg_to_reclaim,0,SEQ_MAX_LOGICAL_OFFSET); i-max_offset>NAND_PAGES_PER_ERASE_UNIT; i-=NAND_PAGES_PER_ERASE_UNIT){
		VERIFY(!nandErase(i))
	}

	/* reboot*/
	mock_reboot();
	VERIFY(!sequencingBooting(fs_data, MOCK_FS_DATA_SIZE, &pendingVOTs, checkpointWriter));
//	PRINT("\nsequncing #2 success");
	/* verify we are not in reclamation*/
//	PRINT_MSG_AND_NUM("\n new slot id= ",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" nsegments= ",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec slot= ",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" rec segmnet= ",GET_RECLAIMED_SEGMENT());
	VERIFY(!IS_STATE_RECLAMATION());

	/* verify reserve segments are written*/
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT;i++){
		VERIFY(verify_seg_header(i, SEG_TYPE_RESERVE));
	}
//	PRINT("\nverified reserve headers");
	SET_LOGICAL_OFFSET(log_addr, 0);
	/* verify first page, in all segments*/

	SET_LOGICAL_OFFSET(log_addr, 0);
	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		SET_LOGICAL_SEGMENT(log_addr, i);
		VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
		VERIFY(verify_seg_header(i, SEG_TYPE_USED));
	}
//	PRINT("\nverified regular headers");
	/* verify slot we are allocating from*/
	/* verify previous page is not of a checkpoint*/
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());

	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
	VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
	VERIFY(!COMPARE(GET_CHECKPOINT_FLAG(seq_flags), CP_NOT_PART_OF_CP));
//	PRINT("\nverified cp flag");

	/* verify the segment we should have completed erasing is the one we write to now*/
	VERIFY(COMPARE(SEQ_SEGMENTS_COUNT, GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr)));
//	PRINT("\nverified new slot id");

	/* verify there is no copy back EU*/
	VERIFY(verify_no_cb_eu());
//	PRINT("\nsuccess no cb");

	/* verify pending VOTs*/
	VERIFY(COMPARE(pendingVOTs, 1));
//	PRINT("\nsuccess pendingVOTs");

	/* verify nsegments*/
	VERIFY(COMPARE(GET_SEG_MAP_NSEGMENTS(seg_map_ptr), SEQ_SEGMENTS_COUNT));
//	PRINT("\nnsegments success");

	/* verify reserve flags*/
	VERIFY(COMPARE(GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr), SEQ_PHY_ADDRESS_EMPTY));
	VERIFY(COMPARE(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr), IS_EU_RESERVE_FALSE));

	VERIFY(verifySlotIsErased(0));

	/* verify obs and free counters*/
//	PRINT_MSG_AND_NUM("\nexpected_free_count=",expected_free_count);
//	PRINT_MSG_AND_NUM("\nactual free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM("\nexpected_obs_count=",expected_obs_count);
//	PRINT_MSG_AND_NUM("\nactual obs count=",GET_OBS_COUNT());
	VERIFY(COMPARE(GET_FREE_COUNTER(),expected_free_count));
	VERIFY(COMPARE(GET_OBS_COUNT(),expected_obs_count));

	return 1;
}
