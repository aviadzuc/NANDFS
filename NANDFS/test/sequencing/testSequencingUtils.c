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

/** @file testSequencingUtils.c  */

#include <src/sequencing/sequencing.h>
#include <src/sequencing/sequencingUtils.h>
#include <test/sequencing/testsHeader.h>
#include <test/sequencing/testSequencingUtils.h>
#include <system.h>
#include <lpc2000/uart.h>
#include <lpc2000/clocks.h>
#include <lpc2000/busywait.h>
#include <utils/print.h>
#include <utils/memlib.h>
#include <test/macroTest.h>
#include <src/sequencing/lfsr.h>

extern segment_map *seg_map_ptr;
extern obs_pages_per_seg_counters *obs_counters_map_ptr;

#ifdef Debug
#ifndef Profiling
extern uint8_t fs_data[MOCK_FS_DATA_SIZE];
	/**
	 * @brief
	 * a mock checkpoint writer function. writes a pseudo file system checkopint data
	 * and the sequencing cp data using commit()
	 * @param isPartOfHeader is the checkpoint written as part of a header
	 * @return 0 if successful, 1 if an error occured in commit
	 */
	error_t checkpointWriter(bool_t isPartOfHeader){
		uint8_t byte = 'c';
	//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - fs_data_size=",MOCK_FS_DATA_SIZE);
	//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - SEQ_CHECKPOINT_SIZE=",SEQ_CHECKPOINT_SIZE);
		fsMemset(fs_data,byte,MOCK_FS_DATA_SIZE);
	//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - fs_data_size=",MOCK_FS_DATA_SIZE);
	//	for(i=0; i< fs_data_size; i++){
	//		PRINT_MSG_AND_NUM("\ncheckpointWriter() - fs_data[",i);
	//		PRINT_MSG_AND_HEX("]=", sequencing_buffer[i]);
	//	}

	//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - the mock checkpoint writer is about to commit, isPartOfHeader=", isPartOfHeader);
		SEQ_VERIFY(!commit(fs_data, MOCK_FS_DATA_SIZE, isPartOfHeader));

	//	PRINT("\ncheckpointWriter() - commit was successful");
		return 0;
	}
#endif
#endif

#ifdef Profiling
extern uint8_t fs_data[MOCK_FS_DATA_SIZE];
	/**
	 * @brief
	 * a mock checkpoint writer function. writes a pseudo file system checkopint data
	 * and the sequencing cp data using commit()
	 * @param isPartOfHeader is the checkpoint written as part of a header
	 * @return 0 if successful, 1 if an error occured in commit
	 */
	error_t checkpointWriter(bool_t isPartOfHeader){
		uint8_t byte = 'c';
	//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - fs_data_size=",MOCK_FS_DATA_SIZE);
	//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - SEQ_CHECKPOINT_SIZE=",SEQ_CHECKPOINT_SIZE);
		fsMemset(fs_data,byte,MOCK_FS_DATA_SIZE);
	//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - fs_data_size=",MOCK_FS_DATA_SIZE);
	//	for(i=0; i< fs_data_size; i++){
	//		PRINT_MSG_AND_NUM("\ncheckpointWriter() - fs_data[",i);
	//		PRINT_MSG_AND_HEX("]=", sequencing_buffer[i]);
	//	}

	//	PRINT_MSG_AND_NUM("\ncheckpointWriter() - the mock checkpoint writer is about to commit, isPartOfHeader=", isPartOfHeader);
		SEQ_VERIFY(!commit(fs_data, MOCK_FS_DATA_SIZE, isPartOfHeader));

	//	PRINT("\ncheckpointWriter() - commit was successful");
		return 0;
	}
#endif

/**
 * @brief
 * initialize read buffer
 * @param flags the read buffer to initialize
 */
void init_flags(spare_area_flags *flags){
	uint8_t i;
//	PRINT("\ninit_flags(flags)");
	for(i=0;i<NAND_SPARE_SIZE; i++){
		CAST_TO_UINT8(flags)[i] = 0xff;
	}
}

/**
 * @brief
 * auxiliary, set segment map to given values
 */
void set_seg_map(segment_map *seg_map_ptr,
				 uint32_t slot_id,
				 uint32_t page_offset,
				 uint32_t seg_num,
				 uint32_t sequence_num,
				 uint32_t new_slot_id,
				 uint32_t previously_written_segment,
				 uint32_t is_eu_reserve,
				 uint32_t nSegments,
				 uint32_t reserve_eu_addr){
	seg_map_ptr->seg_to_slot_map[seg_num] = slot_id;
	SET_RECLAIMED_OFFSET(page_offset);
	SET_RECLAIMED_SEGMENT(seg_num);
	seg_map_ptr->sequence_num  = sequence_num;
	seg_map_ptr->new_slot_id   = new_slot_id;
	seg_map_ptr->previously_written_segment = previously_written_segment;
	seg_map_ptr->is_eu_reserve = is_eu_reserve;
	seg_map_ptr->nSegments     = nSegments;
	seg_map_ptr->reserve_eu_addr = reserve_eu_addr;
}

/**
 * @brief
 * auxiliary, mark reserve segments headers
 */
void mark_reserve_segment_headers(){
	uint32_t i,j;
//	PRINT("\nmark_reserve_segment_headers() - starting");
//	int k;
//						for(k=0; k< SEQ_OBS_COUNTERS; k++){
//							if((int)GET_EXACT_COUNTER_LEVEL(k) !=0){
//								PRINT_MSG_AND_NUM("\n",k);
//								PRINT_MSG_AND_NUM(". couner=",(int)GET_EXACT_COUNTER_LEVEL(k));
//								exit(-1);
//							}
//						}
	INIT_FLAGS_POINTER_TO_BUFFER(flags,sequencing_buffer);
	page_area_flags *page_hdr_ptr = (page_area_flags*)(sequencing_buffer);
//	PRINT("\nmark_reserve_segment_headers() - b4 init flags and buffer");
//							for(k=0; k< SEQ_OBS_COUNTERS; k++){
//								if((int)GET_EXACT_COUNTER_LEVEL(k) !=0){
//									PRINT_MSG_AND_NUM("\n",k);
//									PRINT_MSG_AND_NUM(". couner=",(int)GET_EXACT_COUNTER_LEVEL(k));
//									exit(-1);
//								}
//							}
	initFlags(flags);

	init_buf(sequencing_buffer);
//	for(i=0; i< NAND_TOTAL_SIZE; i++){
//		PRINT_MSG_AND_NUM("\ni=", i);
//		sequencing_buffer[i] = 'ff';
//										for(k=0; k< SEQ_OBS_COUNTERS; k++){
//											if((int)GET_EXACT_COUNTER_LEVEL(k) !=0){
//												PRINT_MSG_AND_NUM("\n",k);
//												PRINT_MSG_AND_NUM(". couner=",(int)GET_EXACT_COUNTER_LEVEL(k));
//												exit(-1);
//											}
//										}
//	}
//	PRINT("\nmark_reserve_segment_headers() - b4 marking slots");
//					for(k=0; k< SEQ_OBS_COUNTERS; k++){
//						if((int)GET_EXACT_COUNTER_LEVEL(k) !=0){
//							PRINT_MSG_AND_NUM("\n",k);
//							PRINT_MSG_AND_NUM(". couner=",(int)GET_EXACT_COUNTER_LEVEL(k));
//							exit(-1);
//						}
//					}
//	PRINT("\nSET_SLOT_EU_OFFSET(flags, SEQ_RESERVE_EU_IS_HEADER)");
	// mark reserve segment header flags
	SET_SLOT_EU_OFFSET(flags, SEQ_RESERVE_EU_IS_HEADER);
//	PRINT("\nSET_SEG_TYPE_FLAG(flags, SEG_TYPE_RESERVE)");
	SET_SEG_TYPE_FLAG(flags, SEG_TYPE_RESERVE);

	// mark reserve segments
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i < SEQ_N_SLOTS_COUNT;i++){
		//mark in segment map
		seg_map_ptr->seg_to_slot_map[i] = i;
//		PRINT_MSG_AND_NUM("\nmark_reserve_segment_headers() - slot is set to reserve seg ",i);
		page_hdr_ptr->segment_id   = i;
//		PRINT("\nmark_reserve_segment_headers() - increment seg map sequence num");
		INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
//		PRINT_MSG_AND_NUM("\nmark_reserve_segment_headers() - inc page header seq num to ",GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
		page_hdr_ptr->sequence_num = seg_map_ptr->sequence_num;

		/* mark segment as reserve. iterate segment eu's until you
		 * find a valid eu to write a header to */
//		 PRINT_MSG_AND_NUM("\nj=",seg_map_ptr->seg_to_slot_map[i] * SEQ_PAGES_PER_SLOT);
		j = seg_map_ptr->seg_to_slot_map[i] * SEQ_PAGES_PER_SLOT;
		for(; j< (i+1)* SEQ_PAGES_PER_SLOT; j+=NAND_PAGES_PER_ERASE_UNIT){
//			PRINT_MSG_AND_NUM("\ncheck eu status ",j);
			if(nandCheckEuStatus(j)){
//				PRINT_MSG_AND_NUM("\nprogram page ",j);
				if(!nandProgramTotalPage(sequencing_buffer,j)){
//					PRINT_MSG_AND_NUM("\nmark_reserve_segment_headers() - marked segment as reserve in address",j);
					break;
				}
//				PRINT("\n error writing to spare area");
			}
		}
	}

//	initFlags(flags);
//	for(i=SEQ_FIRST_RESERVE_SEGMENT; i<SEQ_N_SLOTS_COUNT; i++){
//		nandReadPageFlags(flags, CALC_ADDRESS(test_seg_map_ptr->seg_to_slot_map[i],0,0));
//		PRINT_MSG_AND_NUM("\nmark_reserve_segment_headers() -segment in address ",CALC_ADDRESS(test_seg_map_ptr->seg_to_slot_map[i],0,0));
//		PRINT_MSG_AND_NUM("\nmark_reserve_segment_headers() -reserve segment type = ",flags->segment_type);
//	}
//	init_buf(sequencing_buffer);
//	INIT_FLAGS_POINTER_TO_BUFFER(flags_buf,sequencing_buffer);
//	for(i=SEQ_FIRST_RESERVE_SEGMENT; i<SEQ_N_SLOTS_COUNT; i++){
//		nandReadPage(sequencing_buffer, CALC_ADDRESS(test_seg_map_ptr->seg_to_slot_map[i],0,0), 0, NAND_TOTAL_SIZE);
//		PRINT_MSG_AND_NUM("\nmark_reserve_segment_headers() -segment in address ",CALC_ADDRESS(test_seg_map_ptr->seg_to_slot_map[i],0,0));
//		PRINT_MSG_AND_NUM("\nmark_reserve_segment_headers() -reserve segment type = ",flags_buf->segment_type);
//	}
}

/**
 * @brief
 * imitate reboot by initializing obs and seg map
 */
void mock_reboot(){
	init_obs_counters();
	init_seg_map();
	init_buf(sequencing_buffer);
}

/**
 * @brief
 * auxiliary function - find the first not occupied reserve unit, and mark it
 * as a replacement for the EU of orig_phy_addr
 * @param orig_phy_addr some physical address in one of the data slots
 * @return the physical address of the newly allocated reserve eu
 * if no reserve EU exists return SEQ_PHY_ADDRESS_EMPTY
 */
uint32_t write_to_reserve_eu(uint32_t orig_phy_addr){
	uint32_t i,j;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	init_flags(flags);

	/* iterate segment eu's until you find a valid eu to write to */
//	print(uart0SendByte,0,"\nfind reserve eu to write to");
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i<SEQ_N_SLOTS_COUNT; i++){
		j = seg_map_ptr->seg_to_slot_map[i] * SEQ_PAGES_PER_SLOT;
		for(; j< (i+1)* SEQ_PAGES_PER_SLOT; j+=NAND_PAGES_PER_ERASE_UNIT){
			// verify eu isn't bad
			if(!nandCheckEuStatus(j))
				continue;

			// read spare area
			nandReadPageSpare(CAST_TO_UINT8(flags), j, 0, NAND_SPARE_SIZE);
//			PRINT("\nGET_SLOT_EU_OFFSET(flags)");
			// check if it is empty
			if(GET_SLOT_EU_OFFSET(flags) != SEQ_RESERVE_EU_NOT_OCCUPIED)
				continue;

			// mark flags to be written with slot id and eu offset of original page
			SET_SLOT_EU_OFFSET(flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(orig_phy_addr));
			SET_SEG_TYPE_FLAG(flags, SEG_TYPE_RESERVE);
			// write header of reserve EU
//			PRINT_MSG_AND_NUM("\nmarkEuAsBad() - mark page as bad ",j);
			if(!nandProgramPageC(CAST_TO_UINT8(flags), j, 0, NAND_SPARE_SIZE)){
//				PRINT_MSG_AND_NUM("\nfinished writing to reserve EU in address ", j);
				return j;
			}
		}
	}

	return 0;
}

/**
 * @brief
 * auxiliary function - find the first not occupied reserve unit, write data to all it's pagesand mark it
 * as a replacement for the EU of orig_phy_addr
 * @param orig_phy_addr some physical address in one of the data slots
 * @param buf data buffer
 * @return the physical address of the newly allocated reserve eu. if a program error occured SEQ_PHY_ADDRESS_EMPTY is returned
 */
uint32_t write_data_to_reserve_eu(uint32_t orig_phy_addr,uint8_t *buf){
	uint32_t i,j;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_FLAGS_POINTER_TO_BUFFER(buf_flags,buf);

	initFlags(flags);

	/* iterate segment eu's until you find a valid eu to write to */
//	print(uart0SendByte,0,"\nfind reserve eu to write to");
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i<SEQ_N_SLOTS_COUNT; i++){
		j = seg_map_ptr->seg_to_slot_map[i] * SEQ_PAGES_PER_SLOT;
		for(; j< (i+1)* SEQ_PAGES_PER_SLOT; j+=NAND_PAGES_PER_ERASE_UNIT){
			// verify eu isn't bad
			if(!nandCheckEuStatus(j))
				continue;

			// read spare area
			nandReadPageSpare(CAST_TO_UINT8(flags), j, 0, NAND_SPARE_SIZE);

			// check if it is empty
			if(GET_SLOT_EU_OFFSET(flags) != SEQ_RESERVE_EU_NOT_OCCUPIED)
				continue;

//			PRINT_MSG_AND_NUM("\nwrite_data_to_reserve_eu() - marking a reserve eu ", j);
//			PRINT_MSG_AND_NUM("to replace EU ",orig_phy_addr);
			// mark flags to be written with slot id and eu offset of original page
			SET_SLOT_EU_OFFSET(buf_flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(orig_phy_addr));
			SET_SEG_TYPE_FLAG(buf_flags, SEG_TYPE_RESERVE);
			// write header of reserve EU
			for(i=0; i< NAND_PAGES_PER_ERASE_UNIT; i++){
				if(nandProgramTotalPage(buf, j+i)){
					return SEQ_PHY_ADDRESS_EMPTY;
				}
			}

			return j;
		}
	}

	return 0;
}

/**
 * auxiliary. get data of expected reserve segment, for some given address
 * and compare resuilt of looking for the reserve and the expected
 */
error_t compare_expected_and_reserve(uint32_t seg_id, uint32_t eu_offset, uint32_t page_offset, uint32_t expected_reserve){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	uint32_t phy_addr;

	init_logical_address(log_address);

	SET_LOGICAL_SEGMENT(log_address, seg_id);
	SET_LOGICAL_OFFSET(log_address, page_offset);

	phy_addr = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg_id],eu_offset, page_offset);

	return COMPARE(logicalAddressToReservePhysical(flags, phy_addr) , expected_reserve + page_offset);
}

/**
 * @brief
 * auxiliary function - get level of segment seg_id from obsolete counters map obs_map
 * @param seg_id segment id
 * @param obs_map pointer to a segment map
 * @return counter level
 */
uint8_t get_counter_level_by_map(uint32_t seg_id, obs_pages_per_seg_counters *obs_map){
#ifndef EXACT_COUNTERS
	if(IS_EVEN(seg_id))
		return (obs_map->counters[seg_id/2]) & 0x0f;
	else
		return ((obs_map->counters[seg_id/2] & 0xf0) >> 4);
#else
	return obs_map->counters[seg_id];
#endif
}

/**
 * @brief
 * auxiliary. read last written checkpoint data to buffers
 * @param phy_addr
 * @param fs_data buffer for file system layer data
 * @param fs_data_size file system layer data size
 * @param segment_map segment map buffer
 * @param cp_obs_map_ptr obsolete counters map buffer
 *
 * @return 0 if successful. if no checkkpoint was found return 1
 */
error_t read_cp_to_buffers(uint8_t *fs_data, uint32_t fs_data_size, segment_map *cp_seg_map_ptr, obs_pages_per_seg_counters *cp_obs_map_ptr){
	uint32_t i,j;
//	PRINT("\nread_cp_to_buffers() - start");
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(cp_log_addr);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_FLAGS_POINTER_TO_BUFFER(buf_flags, sequencing_buffer);

	init_logical_address(cp_log_addr);
	initFlags(flags);

	SET_LOGICAL_SEGMENT(cp_log_addr, GET_RECLAIMED_SEGMENT());
	SET_LOGICAL_OFFSET(cp_log_addr, GET_RECLAIMED_OFFSET());



//	PRINT("\nread_cp_to_buffers() - find cp start");
	do{
		nandReadPageFlags(flags, logicalAddressToPhysical(cp_log_addr));
		if(GET_CHECKPOINT_FLAG(flags) == CP_LOCATION_FIRST)
			break;

		/* try previous page*/
		SET_LOGICAL_OFFSET(cp_log_addr, GET_LOGICAL_OFFSET(cp_log_addr)-1);
	}while(GET_LOGICAL_OFFSET(cp_log_addr) > 0);

//	PRINT("\nread_cp_to_buffers() - found cp start");
	if(GET_LOGICAL_OFFSET(cp_log_addr) == 0)
		return 1;

	/* init fs_data before reading to it*/
	fsMemset(fs_data, 0xff, fs_data_size);

//	PRINT("\nread_cp_to_buffers() - read cp byte by byte");
	// read checkpoint
	for(i=0; i< SEQ_CHECKPOINT_SIZE + fs_data_size;){
		/* read, and prepare for next read*/
		nandReadPage(sequencing_buffer, logicalAddressToPhysical(cp_log_addr), 0, NAND_TOTAL_SIZE);
//		PRINT("\nread_cp_to_buffers() - read page");
		SET_LOGICAL_OFFSET(cp_log_addr, GET_LOGICAL_OFFSET(cp_log_addr)+1);
		if(GET_CHECKPOINT_FLAG(buf_flags) == CP_NOT_PART_OF_CP){
			continue;
		}

		j=0;
		do{
			/* if we are reading file system data to buffer */
			if(i < fs_data_size)
				fs_data[i] = sequencing_buffer[j];
			/* if we are writing obs counters map data to buffer */
			else
				CAST_TO_UINT8(cp_obs_map_ptr)[i - fs_data_size] = sequencing_buffer[j];

			j++;
			i++;
		}while(j < NAND_PAGE_SIZE && i<SEQ_CHECKPOINT_SIZE + fs_data_size );
	}
//	PRINT("\nread_cp_to_buffers() - assert we read cp as expected");
	/* sanity check - we should be in the last part of the checkpoint*/
	assert(GET_CHECKPOINT_FLAG(buf_flags) == CP_LOCATION_LAST || SEQ_CHECKPOINT_SIZE+fs_data_size < NAND_PAGE_SIZE);
//	PRINT("\nread_cp_to_buffers() - finished");
	return 0;
}

/**
 * @brief
 * auxiliary, verify segment map values comapred with expected
 * @return 1 if successful, 0 if any of the comparisons failes
 */
error_t verify_seg_map(segment_map *seg_map_to_verify,
					   uint32_t sequence_num,
					   uint32_t new_slot_id,
					   uint32_t previously_written_segment,
					   uint32_t is_eu_reserve,
					   uint32_t nSegments){
	return 1;
	VERIFY(COMPARE(seg_map_to_verify->sequence_num, sequence_num));
//	print(uart0SendByte,0,"\nsuccess sequence_num ");
	VERIFY(COMPARE(seg_map_to_verify->new_slot_id, new_slot_id));
//	print(uart0SendByte,0,"\nsuccess new_slot_id ");
	VERIFY(COMPARE(seg_map_to_verify->previously_written_segment, previously_written_segment));
//	print(uart0SendByte,0,"\nsuccess previously_written_segment ");
	VERIFY(COMPARE(seg_map_to_verify->is_eu_reserve, is_eu_reserve));
//	print(uart0SendByte,0,"\nsuccess is_eu_reserve ");
	VERIFY(COMPARE(seg_map_to_verify->nSegments, nSegments));
//	print(uart0SendByte,0,"\nsuccess nSegments ");

	return 1;
}

/**
 * @brief
 * @param level obs counter level to set
 * set obsolete counters to a given value
 */
void set_obs_counters(uint8_t level){
	uint32_t i;

	for(i=0; i< SEQ_OBS_COUNTERS; i++){
		change_counter_level(level, i);
	}
}

/**
 * @brief
 * set obsolete counters to a given value
 * @param level expected obs counter level
 * @return 1 if verified, 0 otherwise
 */
error_t verify_obs_counters(uint8_t level){
	uint32_t i;

	for(i=0; i< SEQ_OBS_COUNTERS; i++){
		if(get_counter_level(i) !=	level){
//			PRINT_MSG_AND_NUM("\nverify_obs_counters() - counter not verified ",i);
//			PRINT_MSG_AND_NUM(" expected level ",level);
//			PRINT_MSG_AND_NUM(" found level ",get_counter_level(i));
			return 0;
		}
	}

	return 1;
}

/**
 * @brief
 * verify there are no copy back eus
 * @return 1 if none, 0 otherwise
 */
bool_t verify_no_cb_eu(){
	uint32_t i,org_phy_addr;
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags,sequencing_buffer);

	for(i= SEQ_FIRST_RESERVE_SEGMENT; i<SEQ_N_SLOTS_COUNT;i++){
		for(org_phy_addr = CALC_ADDRESS(i,0,0); org_phy_addr<CALC_ADDRESS(i+1,0,0);org_phy_addr+=NAND_PAGES_PER_ERASE_UNIT){
			if(nandCheckEuStatus(org_phy_addr)){
				VERIFY(!nandReadPageTotal(sequencing_buffer, org_phy_addr));
				VERIFY(COMPARE(GET_COPY_BACK_FLAG(seq_flags),COPY_FLAG_FALSE));
			}
		}
	}

	return 1;
}

/**
 * @brief
 * auxiliary. check if any copy back eu's remain in flash
 * @return 1 if there is a reserve EU marked copy back, 0 if none
 */
error_t isThereCopyBackEU(){
	uint32_t i,j;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	initFlags(flags);

	/* iterate reserve segment eu's and test if it is copy back */
	for(i=SEQ_FIRST_RESERVE_SEGMENT; i<SEQ_N_SLOTS_COUNT; i++){
		j = seg_map_ptr->seg_to_slot_map[i] * SEQ_PAGES_PER_SLOT;
		for(; j< (i+1)* SEQ_PAGES_PER_SLOT; j+=NAND_PAGES_PER_ERASE_UNIT){
			// verify eu isn't bad
			if(!nandCheckEuStatus(j))
				continue;

			// read spare area
			nandReadPageSpare(CAST_TO_UINT8(flags), j, 0, NAND_SPARE_SIZE);

			// check if it is empty
			if(GET_COPY_FLAG(flags) == COPY_BACK_FLAG_TRUE)
				return 1;
		}
	}

	return 0;
}

/* re-introduce reserve segments*/
void simpleRemarkReserveSegments(){
	uint32_t i;

	for(i=SEQ_FIRST_RESERVE_SEGMENT;i<SEQ_N_SLOTS_COUNT;i++){
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,i,i);
	}
}

/**
 * @brief
 * auxiliary. verify a page is empty of any data
 * @param log_address logical address of a page
 * @return 1 if empty, 0 if not
 */
error_t verifyPageIsEmpty(logical_addr_t log_address){
	uint32_t i;

	nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(log_address));
	for(i=0; i< NAND_TOTAL_SIZE; i++){
//		PRINT_MSG_AND_NUM("\nsequencing_buffer[",i);
//		PRINT_MSG_AND_NUM("]=",sequencing_buffer[i]);
		VERIFY(COMPARE(0xff, sequencing_buffer[i]));
	}

	return 1;
}

/**
 * @brief
 * - write mock VOTs
 * - write data
 * - write mock VOTs
 * in the end the logical address to which the block was written to is copied to prev
 * @param log_addr logical address to use in functions
 * @param prev_log_addr previous logical address
 * @param vots mark flags as vots or not
 * @return 1 if all allocations were successful. 0 otherwise
 */
error_t writeMockPage(logical_addr_t log_addr, logical_addr_t prev_log_addr, uint8_t vots){
	uint8_t byte = 'c';
	bool_t cpWritten;

	fsMemset(sequencing_buffer,byte, NAND_PAGE_SIZE);

	/* write buffer marked as vot's to flash*/
	VERIFY(!allocAndWriteBlock(log_addr, sequencing_buffer, vots, prev_log_addr, &cpWritten, checkpointWriter,0));

	/* the address now becomes previous adderss*/
	copyLogicalAddress(prev_log_addr, log_addr);

	init_buf(sequencing_buffer);

	return 1;
}

/**
 * @brief
 * - write mock VOTs
 * - write data
 * - write mock VOTs
 * @param log_addr logical address to use in functions
 * @return 1 if all allocations were successful. 0 otherwise
 */
error_t writeMockVOTsPage(logical_addr_t log_addr, logical_addr_t prev_log_addr){
	return writeMockPage(log_addr, prev_log_addr, 1);
}

/**
 * @brief
 * write mock data page filled with byte
 */
error_t writeMockDataPage(logical_addr_t log_addr, logical_addr_t prev_log_addr){
	return writeMockPage(log_addr, prev_log_addr, 0);
}

/**
 * @brief
 * mark data segment Sequentially
 */
void markDataSegementsSequentially(){
	uint32_t i;
	for(i=0;i<SEQ_SEGMENTS_COUNT;i++){
		seg_map_ptr->seg_to_slot_map[i] = i;
	}
}

/**
 * @brief
 * verify all pages in the EU at address eu_start_phy_addr are filled with byte
 * @return 1 if successful, 0 if not
 */
error_t verifyEUWithByte(uint32_t eu_start_phy_addr, uint8_t byte){
	uint32_t j,i;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);
	initFlags(flags);

	SET_LOGICAL_OFFSET(log_addr, CALC_OFFSET_IN_SLOT(eu_start_phy_addr));

	/* if the EU is bad, find it's reserve*/
	if(!nandCheckEuStatus(eu_start_phy_addr)){
		/* if we are checking erasure, then the bye is 0xff and there is no replcament*/
		if(byte == 0xff){
			return 1;
		}

//		PRINT_MSG_AND_NUM("\nverifyEUWithByte() - find reserve for ",eu_start_phy_addr);
		eu_start_phy_addr = logicalAddressToReservePhysical(flags, eu_start_phy_addr);
	}

	for(j=CALC_OFFSET_IN_EU(eu_start_phy_addr);j<NAND_PAGES_PER_ERASE_UNIT; j++){
		init_buf(sequencing_buffer);
		nandReadPage(sequencing_buffer, eu_start_phy_addr+j, 0, NAND_TOTAL_SIZE);

//		if(j==0){
//			PRINT_MSG_AND_NUM("\nverifyEUWithByte() - page#", eu_start_phy_addr);
//			PRINT_MSG_AND_HEX(" sequencing_buffer[0]=",sequencing_buffer[0]);
//			PRINT_MSG_AND_HEX(" byte=",byte);
//		}
		for(i=0; i< NAND_PAGE_SIZE; i++){
			if(sequencing_buffer[i]!=byte){
//				PRINT_MSG_AND_NUM("\nverifyEUWithByte() - page#=",eu_start_phy_addr+j);
//				PRINT_MSG_AND_NUM(" byte#=",i);
//				PRINT_MSG_AND_HEX(" sequencing_buffer[i]=",sequencing_buffer[i]);
//				PRINT_MSG_AND_HEX(" expected byte=",byte);
			}
			VERIFY(COMPARE(byte, sequencing_buffer[i]));
		}
	}

	return 1;
}

/**
 *@brief
 * find the segment this slot belongs to
 * @param slot_id physical slot id
 * @return segment number.
 */
uint32_t find_seg_for_slot(uint32_t slot_id){
	uint32_t i;

	for(i=0;i<SEQ_N_SLOTS_COUNT; i++){
		if(seg_map_ptr->seg_to_slot_map[i] == slot_id)
			return i;
	}

	return SEQ_NO_SEGMENT_NUM; /* always should be found*/
}

/**
 * @brief
 * check if a slot is completely erased
 * @param slot_id physical slot id
 * @return 1 if yes, 0 otherwise
 */
error_t verifySlotIsErased(uint32_t slot_id){
	uint32_t i,j,k, res;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_STRUCT_AND_PTR(flags);

	init_logical_address(log_addr);

	res = find_seg_for_slot(slot_id);
	if(!IS_SEG_EMPTY(res))
		SET_LOGICAL_SEGMENT(log_addr,res);


	initFlags(flags);
	/* verify the expected slot is now erased*/
	for(i=CALC_ADDRESS(slot_id,0,0); i< CALC_ADDRESS((slot_id+1),0,0); i+=NAND_PAGES_PER_ERASE_UNIT){
		init_buf(sequencing_buffer);
//		PRINT_MSG_AND_NUM("\nverifySlotIsErased() - verifying erasure of eu ",CALC_EU_OFFSET(i));
		if(!nandCheckEuStatus(i) && !IS_SEG_EMPTY(res)){
//			PRINT_MSG_AND_NUM("\nverifySlotIsErased() - bad eu ",CALC_EU_OFFSET(i));
			SET_LOGICAL_OFFSET(log_addr, CALC_OFFSET_IN_SLOT(i));

			if(IS_PHY_ADDR_EMPTY(logicalAddressToReservePhysical(flags, i))){
				continue;
			}
		}

		if(IS_SEG_EMPTY(res)){
			continue;
		}

		for(j=0;j<NAND_PAGES_PER_ERASE_UNIT;j++){
			nandReadPage(sequencing_buffer, i+j, 0, NAND_TOTAL_SIZE);
			for(k=0; k< NAND_TOTAL_SIZE; k++){
//				if(!COMPARE(0xff, sequencing_buffer[k])){
//					PRINT_MSG_AND_NUM("\nverifySlotIsErased() - non-empty page ", i+j);
//					int32_t i;
//
//					for(i=0; i < NAND_TOTAL_SIZE; i++){
//						if(sequencing_buffer[i] == 0xff)
//							continue;
//
//						PRINT_MSG_AND_NUM("\n", i);
//						PRINT_MSG_AND_HEX(". ", sequencing_buffer[i]);
//					}
//				}

				VERIFY(COMPARE(0xff, sequencing_buffer[k]));
			}
		}
	}

	return 1;
}

/**
 * @brief
 * replace headers of two segments, one reserve one of data.
 *
 * Assumptions:
 * 1. no data no both of them
 *
 * @param reserve_slot_id physical reserve slot id
 * @param data_slot_id physical data slot id
 * @return 0 if a read/program error occured. 1 if succesful
 */
error_t replaceReserveSlotWithDataSlot(uint32_t reserve_slot_id, uint32_t data_slot_id){
	uint32_t isDataHeaderInReserve,i, data_seg_first_page, reserve_seg_first_page = 0;
	uint32_t reserve_seg = find_seg_for_slot(reserve_slot_id);
	uint32_t data_seg    = find_seg_for_slot(data_slot_id);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(flags,sequencing_buffer);

	/* get physical address of first page of data segment */
	SET_LOGICAL_SEGMENT(log_addr, data_seg);
	SET_LOGICAL_OFFSET(log_addr, 0);
	data_seg_first_page = logicalAddressToPhysical(log_addr);

	init_logical_address(log_addr);
//	PRINT("\nreplaceReserveSlotWithDataSlot() - starting")
//	PRINT_MSG_AND_NUM(" reserve_slot_id=",reserve_slot_id);
//	PRINT_MSG_AND_NUM(" data_slot_id=",data_slot_id);
	/* check if the reserve header is not in the first page of the slot*/
	for(i=CALC_ADDRESS(reserve_slot_id,0,0) ; i<CALC_ADDRESS(reserve_slot_id+1,0,0);i+=NAND_PAGES_PER_ERASE_UNIT){
		if(nandCheckEuStatus(i)){
			reserve_seg_first_page = i;
			break;
		}
	}
//	PRINT_MSG_AND_NUM("\nreplaceReserveSlotWithDataSlot() - reserve_seg_first_page=",reserve_seg_first_page);

	/* read data segment header*/
	VERIFY(!nandReadPageTotal(sequencing_buffer, data_seg_first_page));
//	PRINT_MSG_AND_NUM("\nreplaceReserveSlotWithDataSlot() - success read data header page ",data_seg_first_page);
	/* if header is in a reserve EU*/
	if(GET_SLOT_EU_OFFSET(flags) != SEQ_NO_SLOT_AND_EU_OFFSET){
		isDataHeaderInReserve = 1;
	}

	/* write it to second page of reserve segment first EU*/
	VERIFY(!nandProgramTotalPage(sequencing_buffer, reserve_seg_first_page+1));
//	PRINT_MSG_AND_NUM("\nreplaceReserveSlotWithDataSlot() - success program header page to",reserve_seg_first_page+1);

	/* copy reserve header to data segment*/
	VERIFY(!nandErase(data_seg_first_page));
	VERIFY(!copyPage(data_seg_first_page,
				     reserve_seg_first_page,
				     0, /* t oaddress is not resereve*/
				     CALC_ADDRESS(data_slot_id,0,0),
				     0)); /* the slot we are copying from is not reserve*/

//	PRINT_MSG_AND_NUM("\nreplaceReserveSlotWithDataSlot() - success copyPage ",data_seg_first_page);

	/* replace references in segment map*/
	seg_map_ptr->seg_to_slot_map[data_seg]    = reserve_slot_id;
	seg_map_ptr->seg_to_slot_map[reserve_seg] = data_slot_id;

	/* read copy of data segment header, and erase the EU containg it*/
	VERIFY(!nandReadPageTotal(sequencing_buffer, reserve_seg_first_page+1));
	VERIFY(!nandErase(reserve_seg_first_page));
//	PRINT("\nreplaceReserveSlotWithDataSlot() - success erasing eu containing data seg header");

	/* find EU for reserve seg header*/
	if(!nandCheckEuStatus(reserve_seg_first_page)){
		INIT_FLAGS_STRUCT_AND_PTR(temp_flags);
		initFlags(temp_flags);
		//reserve_seg_first_page = allocReserveEU(temp_flags, reserve_seg_first_page);
		reserve_seg_first_page = get_valid_eu_addr_in_location(reserve_seg_first_page,1);
	}

	/* copy data header to reserve header physical address*/
	VERIFY(!nandProgramTotalPage(sequencing_buffer, reserve_seg_first_page));
//	PRINT("\nreplaceReserveSlotWithDataSlot() - success programming data seg header");
	return 1;
}

/**
 * @brief
 * perform wear leveling only half way through for random_slot until page max_offset
 * @param random_slot the slot physical address we wear level
 * @param max_offset maximum offset until which we wear level in the slot
 * @return 0 if successful. return 1 if a read/write/erase error has occured
 */
error_t performMockWearLeveling(uint32_t random_slot, uint32_t max_offset){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	uint32_t org_to_phy_addr,from_phy_addr, to_phy_addr;
	uint8_t isSlotReserve = 0, isToReserve = 0;
	uint32_t page_offset = 0;

	init_logical_address(log_addr);

	to_phy_addr   = CALC_ADDRESS(seg_map_ptr->new_slot_id,0,0);
	/* from the chosen random slot*/
	from_phy_addr = CALC_ADDRESS(random_slot,0,0);

	/* copy segment, from start to end, so if we reboot and have
	 * 2 identical segments with identical sequencing number, we'd know what to do.
	 * we also copy segment header - we can't change this, since this will
	 * alter the sequence of actual segment writing. */
	while(page_offset<max_offset){
		org_to_phy_addr = CALC_ADDRESS(seg_map_ptr->new_slot_id,0,page_offset);
		/* if we are not at the start of an EU, then the address we copy from
		 * and the address we copy to need no change*/
		if(!(CALC_OFFSET_IN_EU(page_offset) == 0)){
			copyBackPage(to_phy_addr,from_phy_addr,isToReserve, SEQ_PHY_ADDRESS_EMPTY);
			page_offset++;
			to_phy_addr++;
			from_phy_addr++;

			continue;
		}

		/* we are at the start of an EU. assume by default we read original EU's, not reserve ones*/
		to_phy_addr   = CALC_ADDRESS(seg_map_ptr->new_slot_id,0,page_offset);
		from_phy_addr = CALC_ADDRESS(random_slot,0,page_offset);
//		PRINT_MSG_AND_NUM("\nperformMockWearLeveling() - to_phy_addr= ",to_phy_addr);
//		PRINT_MSG_AND_NUM("\nperformMockWearLeveling() - from_phy_addr= ",from_phy_addr);
		/* must handle possible situations -
		 * a. from_phy_addr was to reserve EU
		 * a.1 the segment we are copying from is a reserve segment, so we dont care about bad EU's in it*/
		if(!nandCheckEuStatus(from_phy_addr)){
			/* if we are at the start of a bad EU in a reserve segment
			 * we dont need to look for a reserve to the reserve...
			 * we can simply move on to the next EU*/
//			 PRINT_MSG_AND_NUM("\nperformMockWearLeveling=",isSlotReserve);
			if(isSlotReserve){
				page_offset += NAND_PAGES_PER_ERASE_UNIT;
				continue;
			}

			/* the slot is not used for a reserve segment.*/
			SET_LOGICAL_OFFSET(log_addr, page_offset);
			/* locate the reserve EU*/
			from_phy_addr = logicalAddressToReservePhysical(flags, from_phy_addr);
//			PRINT_MSG_AND_NUM("\nperformMockWearLeveling() - from EU is bad. reserve one is in ",from_phy_addr);
			assert(from_phy_addr != SEQ_PHY_ADDRESS_EMPTY);
		}
		else{

		}

		/* b. to_phy_addr is bad. need to replace it*/
		if(!nandCheckEuStatus(to_phy_addr)){
//			PRINT_MSG_AND_HEX("\nperformMockWearLeveling() - invalid to_phy_addr ",to_phy_addr);
			isToReserve = 1;
			to_phy_addr = allocReserveEU(flags, to_phy_addr);
//			PRINT_MSG_AND_NUM("\nperformMockWearLeveling() - replacing it with ",to_phy_addr);
			/* if there are no more vacant reserve EU's return error*/
			 if(to_phy_addr == SEQ_PHY_ADDRESS_EMPTY){
			 	/* erase original slot, from end to start*/
				if(truncateSlot(seg_map_ptr->new_slot_id, isSlotReserve))
					return 1;

				return 0;
			 }
		}
		else{
			isToReserve = 0;
		}

//		PRINT_MSG_AND_NUM("\nperformWearLeveling() - copying page from ",from_phy_addr);
//		PRINT_MSG_AND_NUM(" to ",to_phy_addr);
		copyPage(to_phy_addr,
				 from_phy_addr,
				 isToReserve,
				 org_to_phy_addr,
				 isSlotReserve);
		/* incremeent page offset */
		to_phy_addr++;
		from_phy_addr++;
		page_offset++;
	}

	return 0;
}

/**
 * @brief
 * auxiliary function. find the first valid EU in a reserve slot - it contains it's header
 * @param reserve_slot_id physical address of a reserve slot
 * @return 1 successful, 0 otherwise
 */
uint32_t readReserveSegmentHeader(uint32_t reserve_slot_id){
	uint32_t reserve_slot_address = CALC_ADDRESS(reserve_slot_id,0,0);

	reserve_slot_address = get_valid_eu_addr_in_location(reserve_slot_id,1);
			/* read header*/
	if(nandReadPage(sequencing_buffer,reserve_slot_address,0,NAND_TOTAL_SIZE))
		return 0;

	return 1;
}

/**
 * @brief
 * verify segment headet data is as expeceted
 * @param seg_id segment id
 * @param seg_type expected segmnet type
 * @return 0 if failed, 1 if successful
 */
error_t verify_seg_header(uint32_t seg_id, uint32_t seg_type){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	init_logical_address(log_addr);

	SET_LOGICAL_OFFSET(log_addr,0);
	SET_LOGICAL_SEGMENT(log_addr,seg_id);
//	PRINT("\nverify_seg_header() - start");
//	PRINT_MSG_AND_NUM(" seg_id=",seg_id);
//	PRINT_MSG_AND_NUM(" slot=",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, seg_id));
//	PRINT_MSG_AND_NUM(" seg_type=",seg_type);
//
	switch(seg_type){
		case SEG_TYPE_RESERVE:
			VERIFY(readReserveSegmentHeader(seg_id));
			break;
		case SEG_TYPE_USED:
			VERIFY(!nandReadPageTotal(sequencing_buffer,logicalAddressToPhysical(log_addr)));
			break;
		default:
			return 0;
	}

//	PRINT_MSG_AND_NUM("\nGET_HEADER_SEGMENT_ID(sequencing_buffer)=",GET_HEADER_SEGMENT_ID(sequencing_buffer));
	VERIFY(COMPARE(GET_HEADER_SEGMENT_ID(sequencing_buffer),seg_id));
//	PRINT("\nverify_seg_header() - seg id success");
	VERIFY(!COMPARE(GET_HEADER_SEQUENCE_NUM(sequencing_buffer),SEQ_NO_SEQUENCE_NUM));
//	PRINT("\nverify_seg_header() - sequence num success");
	VERIFY(COMPARE(GET_SEG_TYPE_FLAG(seq_flags), seg_type));

	return 1;
}

/**
 * @brief
 * write simple segment headers from

 * @return 0 if successful, 1 in case of segment write error
 */
error_t writeSimpleSegmentHeaders(uint32_t first_seg_id){
	uint32_t i, slot_phy_addr;
	INIT_FLAGS_STRUCT_AND_PTR(flags);

	/* write headers for all segments*/
	for(i=first_seg_id;i<SEQ_SEGMENTS_COUNT;i++){
//		PRINT_MSG_AND_NUM("\nwriteSimpleSegmentHeaders() - i=", i);
		SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr, IS_EU_RESERVE_FALSE);
		SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, SEQ_PHY_ADDRESS_EMPTY);
		SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, i);
//		PRINT_MSG_AND_NUM("\nwriting header to slot ",i);
		slot_phy_addr = CALC_ADDRESS(i,0,0);
		SET_RECLAIMED_SEGMENT(i);
		SET_RECLAIMED_OFFSET(0);
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,i,i);
//		PRINT_MSG_AND_NUM("\nset slot ", i);
//		PRINT_MSG_AND_NUM(" for seg ", i);
//		PRINT_MSG_AND_NUM(" verification=", i==GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,i));
		if(!nandCheckEuStatus(slot_phy_addr)){
			slot_phy_addr = allocReserveEU(flags, slot_phy_addr);
			VERIFY(!COMPARE(slot_phy_addr, SEQ_PHY_ADDRESS_EMPTY));
			SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr, IS_EU_RESERVE_TRUE);
			SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, slot_phy_addr);
		}
//		PRINT_MSG_AND_NUM("\nwrite header for slot ",i);
		VERIFY(!writeSegmentHeader(checkpointWriter));
		INCREMENT_SEG_MAP_NSEGMENTS(seg_map_ptr);
//		PRINT_MSG_AND_NUM("\nwriteSimpleSegmentHeaders() - b4 finishin iteration verification=", i==GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,i));
	}


	return 0;
}
