/** @file sequencing.c
 * Sequencing layer functions high-level functionss
 */
/* for debugging*/
#ifdef Debug
#include <test/macroTest.h>
#endif

#include <peripherals/nand.h>
#include <src/sequencing/sequencing.h>
#include <src/sequencing/sequencingUtils.h>
#include <src/sequencing/lfsr.h>
#include <utils/memlib.h>
#include <src/fs/fsUtils.h>

extern segment_map *seg_map_ptr;
extern obs_pages_per_seg_counters *obs_counters_map_ptr;

#ifdef Profiling
extern int nand_total_reads;
extern int nand_spare_reads;
extern int nand_total_writes;
extern int nand_spare_writes;
extern int nand_erases;
#endif

#ifdef PROFILING_LATENCY
extern int total_allocs;
extern int allocs_nand_total_reads;
extern int allocs_nand_spare_reads;
extern int allocs_nand_total_writes;
extern int allocs_nand_spare_writes;
extern int allocs_nand_erases;
extern FILE *latency_log_fp;
#endif
/**
 * @brief
 * mark block b as obsolete (second write to spare area)
 *
 * @param b logical addr to mark as obsolete
 * @param is_after_reboot indicator whether we are handling vots after reboot.
 *                        1) YES - a page that is already marked as obsolete should update obs count
 *                        2) NO  - obs count was already uncremeneted. don't do it again
 * @return 0 if successful, 1 otherwise
 */
error_t markAsObsolete(logical_addr_t b, bool_t is_after_reboot){
//	FENT();
//	PRINT_MSG_AND_NUM(", seg=", GET_LOGICAL_SEGMENT(b));
//	PRINT_MSG_AND_NUM(", offset=", GET_LOGICAL_OFFSET(b));
//	PRINT_MSG_AND_NUM(", slot=", GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, GET_LOGICAL_SEGMENT(b)));
//	PRINT_MSG_AND_NUM(", new slot id=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
	uint32_t phy_addr = logicalAddressToPhysical(b);

	if(IS_PHY_ADDR_EMPTY(phy_addr)){
		PRINT("\nfailure - SEQ_PHY_ADDRESS_EMPTY");
		return 1;
	}

	if(is_page_marked_obsolete(phy_addr)){
		/* increase obs count (if we got here, we are maring pages as obsolete after reboot)*/
#ifdef Debug
//		L("page already marked as obsolete. return 0");
//		PRINT_MSG_AND_NUM(" seg=", GET_LOGICAL_SEGMENT(b));
//		PRINT_MSG_AND_NUM(" offset=", GET_LOGICAL_OFFSET(b));
//		PRINT_MSG_AND_NUM(" slot=", GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, GET_LOGICAL_SEGMENT(b)));
#endif
		if(is_after_reboot){
			INCREASE_OBS_COUNT();
		}
		return 0;
	}
//	if((*((uint32_t*)(b)) & 0xff000000) >> 24 != 0)
//	PRINT_MSG_AND_NUM("\nmarkAsObsolete() - mark as obs page phy_addr=",phy_addr);
//	PRINT_MSG_AND_HEX(" obsolete_byte=",obsolete_byte);
	// program the obsolete flag to flash
	if(nandProgramPageC(&obsolete_byte, phy_addr, OBSOLETE_BYTE_OFFSET, 1)){
		L("error program page ");
		return 1;
	}

//	if(!is_page_marked_obsolete(phy_addr)){
//		PRINT_MSG_AND_NUM("\nmarkAsObsolete() - page not marked obsolete as expected ", phy_addr);
//		exit(1);
//	}

	/* marking succeeded. increment counter*/
	INCREASE_OBS_COUNT();
//	L("increased obs count to %d", GET_OBS_COUNT());
//	printf("\nmarkAsObsolete() - obs_counters[%d]=%d", GET_LOGICAL_SEGMENT(b), obs_counters[GET_LOGICAL_SEGMENT(b)]);

/* if we have probabilsti counters*/
#ifndef EXACT_COUNTERS
	/* check if we mark the first obsolete*/
	if(COMPARE(OBS_COUNT_NO_OBSOLETES, get_counter_level(GET_LOGICAL_SEGMENT(b)))){
		change_counter_level(OBS_COUNT_LEVEL_0,GET_LOGICAL_SEGMENT(b));
//		PRINT("\n chnaged counter level");
		return 0;
	}

	/* "toss dice". if we "lose" we do not increment */
	if(!gen_rand_bit(OBS_NEEDED_PROB)){
//		PRINT(". done");
		return 0;
	}

	//	PRINT("\n incremeneted counter");
	//	print(uart0SendByte,0,"\nincrement counter ");
	increment_counter(GET_LOGICAL_SEGMENT(b));
#else
	INC_EXACT_COUNTER(GET_LOGICAL_SEGMENT(b));
#endif
	return 0;
}

#ifdef PROFILING_LATENCY
uint32_t findNextFreePage_latency_wrapper(bool_t *cpWritten,
										  checkpoint_writer cp_write_p,
										  uint32_t ok_error_code){
	uint32_t write_res = 0;
	int temp_nand_total_reads = 0;
	int temp_nand_spare_reads = 0;
	int temp_nand_total_writes = 0;
	int temp_nand_spare_writes = 0;
	int temp_nand_erases = 0;

	/* save counters before findinf next free page*/
	temp_nand_total_reads  = nand_total_reads;
	temp_nand_spare_reads  = nand_spare_reads;
	temp_nand_total_writes = nand_total_writes;
	temp_nand_spare_writes = nand_spare_writes;
	temp_nand_erases       = nand_erases;

	/* call findNextFreePage()*/
	write_res = findNextFreePage(cpWritten, cp_write_p, write_res);

	/* add counters delta to latency counters*/
	total_allocs++;
	allocs_nand_total_reads  = nand_total_reads  - temp_nand_total_reads;
	allocs_nand_spare_reads  = nand_spare_reads  - temp_nand_spare_reads;
	allocs_nand_total_writes = nand_total_writes - temp_nand_total_writes;
	allocs_nand_spare_writes = nand_spare_writes - temp_nand_spare_writes;
	allocs_nand_erases       = nand_erases       - temp_nand_erases   ;

	/* if the log file is open, log counters*/
	if(latency_log_fp != NULL){
		float time = ((double)allocs_nand_total_reads+(double)allocs_nand_spare_reads)*20.0+
					 ((double)allocs_nand_total_reads*2112.0+(double)allocs_nand_spare_reads*64.0)*0.025+
					 ((double)allocs_nand_total_writes+(double)allocs_nand_spare_writes)*200.0+
					 ((double)allocs_nand_total_writes*2111.0+(double)allocs_nand_spare_writes)*0.025+
					 ((double)allocs_nand_erases)*1500.0;
//		fprintf(latency_log_fp, "\n%d, %d, %d, %d, %d, %d, %f", total_allocs,
//															 allocs_nand_total_reads,
//															 allocs_nand_spare_reads,
//															 allocs_nand_total_writes,
//															 allocs_nand_spare_writes,
//															 allocs_nand_erases,
//															 time);
		fprintf(latency_log_fp, "\n%d,%f", total_allocs, time);
		if (allocs_nand_spare_reads != 0){
//			printf("\n%d, %d, %d, %d, %d, %d, %f", total_allocs,
//												 allocs_nand_total_reads,
//												 allocs_nand_spare_reads,
//												 allocs_nand_total_writes,
//												 allocs_nand_spare_writes,
//												 allocs_nand_erases,
//												 time);
//			printf("\n%d,%f", total_allocs, time);
		}
	}
//	L("new alloc counters are - reads %d, spare %d, writes %, spare %d, erases %d", allocs_nand_total_reads,
//																				    allocs_nand_spare_reads,
//																				    allocs_nand_total_writes,
//																				    allocs_nand_spare_writes,
//																				    allocs_nand_erases);
	/* finally return findNextFreePage() result*/
	return write_res;
}
#endif

/**
 * @brief *
 * write the block in data, indicating whether it is VOTs by onlyVOTs, and writing it's previuous
 * block address in the transaction in prev.
 * Currently - write page to next logical address in segment (without checking if we should copy/allocate
 * new segment/reserve EU etc. ignore  prev and onlyVOTs.
 * then advance the logical address.
 *
 * Assumptions -
 * 1. logical address is initialized properly
 * 2. reclaimed logical address in segment map is set to an empty address
 *
 * @param log_addr logical address to write the address to which the data is written
 * @param data the data buffer (NAND_PAGE_SIZE)
 * @param onlyVOTs indicate whther block data contains VOT records
 * @param prev previous logical address in list
 * @param cpWritten did we write a checkpoint during this allocAndWriteBlock (i.e. we moved to a new segment)
 * @param isCommit is the allocation part of a commit
 * allocAndWriteBlock(), we do not continue writing the data.
 * @return 0 if allocation was successful.
 * 		   1 if the write was bad, or an error occured during writing in wear leveling, or when truncating
 *         ERROR_FLASH_FULL if there are no avialable pages.
 * 		   ERROR_NO_RESERVE_EU if we need to replace a bad EU but have no spare one left
 *  	   ERROR_BAD_WEAR_LEVELING if an error occured during wear leveling
 */
error_t allocAndWriteBlock(logical_addr_t log_addr, void* data, bool_t onlyVOTs, logical_addr_t prev, bool_t *cpWritten, checkpoint_writer cp_write_p, bool_t isCommit){
	uint32_t write_res = 0;
	uint32_t phy_addr;
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags,sequencing_buffer);
//	PRINT("\nallocAndWriteBlock() - starting");
//	PRINT_MSG_AND_NUM(" rec seg=", GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" new slot=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" expected addr=", CALC_ADDRESS(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr), 0, GET_RECLAIMED_OFFSET()));
//	PRINT_MSG_AND_NUM(" is reserve eu=", GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)==IS_EU_RESERVE_TRUE);
//	PRINT_MSG_AND_NUM(" isCommit=", isCommit);


//	uint32_t temp;
//	{
//		uint8_t buf[NAND_TOTAL_SIZE];
//		page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//		nandReadPageTotal(buf, 64576);
////		PRINT_MSG_AND_NUM("\n	allocAndWriteBlock() - starting. page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//		temp = GET_HEADER_SEGMENT_ID(page_header_ptr);
//	}

	/* if we're not writing a commit data, use sequrncing buffer
	 * (in commit data=sequencing_buffer)*/
	if(!isCommit){
		init_buf(sequencing_buffer);
		fsMemcpy(sequencing_buffer, data, NAND_PAGE_SIZE);
		initFlags(seq_flags);
	}

	/* if the flash is full we cannot allocate*/
	if(is_flash_full()){
		L("is_flash_full() error");
		return ERROR_FLASH_FULL;
	}

	/* if we aborted during a reserve EU addres allocation becuase of a lack of such EU's*/
	if(IS_NO_RESERVE_EUS()){
		L("no reserve EU's");
		return ERROR_NO_RESERVE_EU;
	}

	/* set flags to VOTs true if necessary*/
	if(onlyVOTs){
		SET_VOTS_FLAG(seq_flags, VOTS_FLAG_TRUE);
	}

	/* if the address should be in a reserve eu, physical addres to write to is the reserve eu
	 * + the offset in the eu*/
//	if(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr) == 21 && CALC_EU_START(GET_RECLAIMED_OFFSET())== 1344){
//		PRINT("\n\n\n@@@@@@@@@@@allocAndWriteBlock() - writing to bad eu, slot 21 in offset 1344");
//	}
	phy_addr = calc_target_phy_addr();
//	PRINT_MSG_AND_NUM("\n@@@@@@@@@ allocAndWriteBlock() - writing block to phy_addr=",phy_addr);
//	PRINT_MSG_AND_NUM(" is reserve eu=", GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)==IS_EU_RESERVE_TRUE);
//	PRINT_MSG_AND_HEX("\nallocAndWriteBlock() - writing block to phy_addr=",phy_addr);
//	PRINT_MSG_AND_NUM("\nallocAndWriteBlock() - first byte = ",CAST_TO_UINT8(data)[0]);
//	PRINT_MSG_AND_NUM("\nallocAndWriteBlock() - second byte = ",CAST_TO_UINT8(data)[1]);

	copyPrevAddressToFlags(seq_flags, prev);

//	uint32_t temp = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
//	if(temp==33047){
//		PRINT_MSG_AND_NUM("\nallocAndWriteBlock() - b4 writeBlock() of cp to 33047 cp flag=", GET_CHECKPOINT_FLAG(seq_flags));
//	}

//	PRINT("\nallocAndWriteBlock - copyPrevAddressToFlags finished");
	/* write the data to flash, and verify it was indeed written (logical address was set)*/
//	PRINT("\nallocAndWriteBlock - do writeBlock()");
	writeBlock(log_addr, sequencing_buffer, phy_addr);
//	{
//		uint8_t buf[NAND_TOTAL_SIZE];
//		page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//		nandReadPageTotal(buf, 64576);
//		if(temp!=GET_HEADER_SEGMENT_ID(page_header_ptr)){
//			PRINT_MSG_AND_NUM("\n	allocAndWriteBlock() - finished writeBlock. page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//		}
//	}
//	if(temp == 33047){
//		uint8_t buf[NAND_TOTAL_SIZE];
//		INIT_FLAGS_POINTER_TO_BUFFER(flags, buf);
//		nandReadPageTotal(buf, 33047);
//		printSeqBlock(buf);
//		PRINT_MSG_AND_NUM("\nafter writeBlock() cp flag for page 33047=",GET_CHECKPOINT_FLAG(flags));
//		assert(0);
//	}
//	L(" wrote block to segment %d (slot %d) page_offset %d",GET_LOGICAL_SEGMENT(log_addr), GET_SLOT_BY_SEGMENT(GET_LOGICAL_SEGMENT(log_addr)), GET_LOGICAL_OFFSET(log_addr));

	/* if not in reclamation, we have one less free page.
	 * NOTICE - checkpoint writes aren't counted!*/
//	PRINT_MSG_AND_NUM("\nallocAndWriteBlock - state reclamation?",IS_STATE_RECLAMATION());
	if(!IS_STATE_RECLAMATION()){
		DECREMENT_FREE_COUNT();
//		L("decremented free count to %d", GET_FREE_COUNTER());
	}

//	/* a checkpoint is actually a free page in the system*/
//	if(isCommit){
//		INCREMENT_FREE_COUNT();
//	}

	/* advance segment map pointer to next address*/
//	PRINT_MSG_AND_NUM("\nb4 increment rec offset=", GET_RECLAIMED_OFFSET());
	INCREMENT_RECLAIMED_OFFSET();
//	PRINT_MSG_AND_NUM("\nafter increment rec offset=", GET_RECLAIMED_OFFSET());

	/* verify the write was performed*/
	if(IS_ADDRESS_EMPTY(log_addr)){
		L("write_res error 1");
		write_res = 1;
	}

	/* if we wrote last part of checkpoint don't findNextFreePage()*/
	if(isCommit == IS_COMMIT_LAST){
		return 0;
	}

	/* finished writing data.
	 * iterate all segments until we find an empty page for writing.
	 * to prepare it for the next allocAndWrite()*/
//	PRINT("\nallocAndWriteBlock() - call findNextFreePage()");
#ifdef PROFILING_LATENCY
	write_res = findNextFreePage_latency_wrapper(cpWritten, cp_write_p, write_res);
#else
	write_res = findNextFreePage(cpWritten, cp_write_p, write_res);
#endif
	init_buf(sequencing_buffer);
	if(write_res){
//		L("write_res error=",write_res);
		return write_res;
	}
//	PRINT("\nallocAndWriteBlock() - finished");
//	PRINT_MSG_AND_NUM(" rec seg=", GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" new slot=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" expected addr=", CALC_ADDRESS(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr), 0, GET_RECLAIMED_OFFSET()));
//	PRINT_MSG_AND_NUM(" is reserve eu=", GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)==IS_EU_RESERVE_TRUE);
//	PRINT_MSG_AND_NUM(" isCommit=", isCommit);
	return 0;
}

/**
 * @brief
 * write checkpoint to flash.
 * checkpoint includes:
 * - segment map
 * - obsolete counters map
 * - file system snapshot and checkpoint
 *
 * if another checkpoint was written during the writing of a part of the checkpoint return successfuly
 * @param data checkpoint data of the file system (upper layer)
 * @param nBlocks size (in bytes) of fs_cp_data
 * @param isPartOfHeader are we commiting as part of a transaction, or of writing a segment header
 * @return 0 if successful.
 * if an error occured when marking old checkpoint as obsolete return 1.
 * if an error occured in allocAndWrite return it's error.
 * if an error during programming occurs - return 1
 */
error_t commit(void* data, uint32_t nBytes, bool_t isPartOfHeader){
	uint32_t phy_addr, res, cp_bytes_index, buffer_byte_index, cp_page_idx = 0, cp_page_idx_max;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(alloc_log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	bool_t cpWrittenDuringAllocation = 0;
	uint32_t cp_size = SEQ_CHECKPOINT_SIZE + nBytes;
	uint8_t byte_to_write;
	bool_t isCommit = IS_COMMIT_NOT;

	init_logical_address(prev_log_addr);
	init_logical_address(alloc_log_addr);

	/* calculate how many pages the checkpoint requires*/
	cp_page_idx_max = CALCULATE_IN_PAGES(cp_size)-1;
//
//	PRINT("\ncommit() - starting. ");
//	PRINT_MSG_AND_NUM(", nBytes=",nBytes);
//	PRINT_MSG_AND_NUM(", isPartOfHeader=",isPartOfHeader);
//	PRINT_MSG_AND_NUM(", cp_size=",cp_size);
//	PRINT_MSG_AND_NUM(", cp_page_idx_max=",cp_page_idx_max);
//	PRINT_MSG_AND_NUM(", nBytes=",nBytes);

	init_buf(sequencing_buffer);

	/* if we write more then one page for the checkopint */
	if(IDX_OVERFLOWS_PAGE_SIZE(cp_size)){
		/* iterate checkpoint bytes one by one.
		 * write them to page size units, and program to flash.
		 * if a checkpoint was written in any of the programs finish*/
		cp_bytes_index=0;
		while(cp_bytes_index < cp_size){
			/* b4 writing another checkpoint page, increment free count in the system*/
			if(!isPartOfHeader){
				INCREMENT_FREE_COUNT();
			}

//			PRINT_MSG_AND_NUM("\ncommit() - iteration ",cp_bytes_index/ NAND_PAGE_SIZE );
			/* write bytes to buffer as long as we dont exceed NAND_PAGE_SIZE */
			buffer_byte_index=0;
			init_buf(sequencing_buffer);
			do{
				if(cp_bytes_index < nBytes){
//					PRINT_MSG_AND_NUM("\ncommit() - setting byte ",buffer_byte_index );
//					PRINT_MSG_AND_HEX(" to ",CAST_TO_UINT8(data)[cp_bytes_index]);
					/* if we are writing file system data to buffer */
					byte_to_write = CAST_TO_UINT8(data)[cp_bytes_index];
				}
				else if(cp_bytes_index < sizeof(obs_counters_map) + nBytes){
//					PRINT_MSG_AND_NUM("\ncommit() - setting byte ",buffer_byte_index );
//					PRINT_MSG_AND_NUM("\ncommit() - setting obs map byte #",cp_bytes_index-nBytes);
//					PRINT_MSG_AND_HEX(" to ",CAST_TO_UINT8(obs_counters_map_ptr)[cp_bytes_index-nBytes] );

//					PRINT_MSG_AND_HEX(", cp_bytes_index=",cp_bytes_index);
//					PRINT_MSG_AND_HEX(", nBytes=",nBytes);
					/* if we are writing obs counters map data to buffer */
					byte_to_write = CAST_TO_UINT8(obs_counters_map_ptr)[cp_bytes_index-nBytes];
				}
				else{
					byte_to_write = CAST_TO_UINT8(&lfsr_state)[cp_bytes_index-nBytes-sizeof(obs_counters_map)];
				}
				/* write byte to buffer*/
				sequencing_buffer[buffer_byte_index] = byte_to_write;

				buffer_byte_index++;
				cp_bytes_index++;
			}while(!IDX_OVERFLOWS_PAGE_SIZE(buffer_byte_index) &&
				    cp_bytes_index < cp_size);

//			PRINT_MSG_AND_NUM("\ncommit() - cp_page_idx=",cp_page_idx);
			/* change flags of page to indicate it's part of a checkpoint*/
			initFlags(flags);
			set_cp_page_flag(flags, cp_page_idx, cp_page_idx_max);

			/* write and verify*/
			if(!isPartOfHeader){
//				PRINT("\ncommit() - not part of header. initializing sequencing_buffer");
				setIsCommit(&isCommit, !(cp_bytes_index < cp_size));
//				PRINT_MSG_AND_NUM("\ncommit() - after setIsCommit() isCommit=",isCommit);
				res = allocAndWriteBlock(alloc_log_addr, sequencing_buffer, 0, prev_log_addr, &cpWrittenDuringAllocation, fsCheckpointWriter,isCommit);
				init_buf(sequencing_buffer);

				if(res){
//					print(uart0SendByte,0,"\nfailed allocating ");
					return 1;
				}
			}
			else{
//				PRINT("\ncommit() - is part of header. ");
				/* we are writing checkpoint as part of a segment header. no need to worry
				 * about copying pages, available space etc. simply program and move pointer*/
				phy_addr = calc_target_phy_addr();

//				PRINT_MSG_AND_NUM("\ncommit() - write checkpoint of header to phy_addr=",phy_addr);
				writeBlock(alloc_log_addr, sequencing_buffer, phy_addr);

				init_buf(sequencing_buffer);
				/* verify no error*/
				SEQ_VERIFY(!IS_ADDRESS_EMPTY(alloc_log_addr));
//				PRINT("\ncommit() - write success");
				INCREMENT_RECLAIMED_OFFSET();
			}
//			PRINT_MSG_AND_NUM("\ncommit() - wrote to page ", logicalAddressToPhysical(alloc_log_addr));
			/* prepare for next write*/
			cp_page_idx++;
			/* a checkpoint was written during the allocAndWriteBlock operation
			 * (i.e. another segment was allocated) no need to continue*/
			if(cpWrittenDuringAllocation){
				init_buf(sequencing_buffer);
				return 0;
			}

//			/* when writing as part of an ongoing transaction
//			 * if we haven't written last checkpont page yet, advance to next page
//			 * (after last page, further writing is done only if required by the calling transaction)*/
//			if(!isPartOfHeader && cp_bytes_index < cp_size){
//				SEQ_VERIFY(!findNextFreePage(cpWritten, cp_write_p, write_res));
//			}
		}
	}
	else{
		/* b4 writing another checkpoint page, increment free count in the system*/
		if(!isPartOfHeader){
			INCREMENT_FREE_COUNT();
		}

		/* TODO: this check can be ommited later*/
		if(nBytes > 0) fsMemcpy(sequencing_buffer,data, nBytes);
		fsMemcpy(&(sequencing_buffer[nBytes]),obs_counters_map_ptr, sizeof(obs_counters_map));

//		PRINT_MSG_AND_NUM("\ncommit() - setting obs map byte #",64);
//		PRINT_MSG_AND_HEX(" to ",CAST_TO_UINT8(obs_counters_map_ptr)[64] );
		/* mark buffer as last of checkpoint */
		SET_CHECKPOINT_FLAG(flags, CP_LOCATION_FIRST);
//		PRINT_MSG_AND_NUM("\ncommit() - checkpoint was written to ", GET_RECLAIMED_OFFSET());

		/* write and verify*/
		if(!isPartOfHeader){
//			uint32_t temp = GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr)*SEQ_PAGES_PER_SLOT+GET_RECLAIMED_OFFSET();
//			if(temp==33047){
//				PRINT_MSG_AND_NUM("\ncommit() -allocAndWriteBlock cp. cp flag=", GET_CHECKPOINT_FLAG(flags));
//			}
//			PRINT_MSG_AND_NUM("\ncommit() - write cp (not header) to offset ", GET_RECLAIMED_OFFSET());
//			PRINT_MSG_AND_NUM("\ncommit() - write cp (not header) to addr=", logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
			res = allocAndWriteBlock(alloc_log_addr, sequencing_buffer, 0, prev_log_addr, &cpWrittenDuringAllocation, fsCheckpointWriter,IS_COMMIT_LAST);
//			if(temp==33047){
//				{
//					PRINT_MSG_AND_NUM("\ncommit() -allocAndWriteBlock res=", res);
//					uint8_t buf[NAND_TOTAL_SIZE];
//					INIT_FLAGS_POINTER_TO_BUFFER(flags, buf);
//					nandReadPageTotal(buf, 33047);
//			//		printSeqBlock(buf);
//					PRINT_MSG_AND_NUM("\ncp flag for page 33047=",GET_CHECKPOINT_FLAG(flags));
////					assert(0);
//				}
//			}
//			PRINT_MSG_AND_NUM("\ncommit() - done. res=",res);
			if(res){
//				PRINT_MSG_AND_NUM("\nfailed allocating. error code ", res);
				return 1;
			}
		}
		else{
			/* we are writing checkpoint as part of a segment header. no need to worry
			 * about copying pages, available space etc. simply program and move pointer*/
//			PRINT("\ncommit() - logicalAddressToPhysical");
			phy_addr = calc_target_phy_addr()/*logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR())*/;
//			PRINT_MSG_AND_NUM("\ncommit() - writing header cp to phy_addr=", phy_addr);
//			PRINT_MSG_AND_NUM(". new slot id=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
			writeBlock(alloc_log_addr, sequencing_buffer, phy_addr);
			INCREMENT_RECLAIMED_OFFSET();
		}
	}
//	PRINT("\ndone commiting");
	init_buf(sequencing_buffer);

	return 0;
}


/**
 * @brief
 * Call on system boot
 * find latest checkpoint, mark all following blocks as obsolete (truncation).
 * return whether the transaction that commited in the checkpoint might still have pending VOTs -
 * If we wrote data after commit then the sequencing layer assumes that the transaction
 * that commited has no VOTs.
 * on successful return reclaimed offset points at the empty page after the checkpoint
 *
 * Assumptions:
 * 1. we erased last written eu into validity
 * 2. segment map is located, latest location, slot id and reserve flag are indicated
 * 3. last reclaimed address is not pointing at an empty page, but rather the last written page.
 * reason - we're unable to write complete segment header (including checkpoint) if last written
 * page was the last in a segment.
 * 4. booting has finished and the flash is in stable state - reclaimed address points at a page with valid data
 *
 * @return 0 if successful. if an error occured when erasing EU's return 1.
 * if no checkpoint is found, return ERROR_NO_CP
 */
error_t findCheckpointAndTruncate(void* data, uint32_t nBytes, bool_t* pending_VOTs){
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(last_valid_log_addr);
	bool_t cp_more_than_one_page;
	uint32_t last_valid_offset,res, cp_size;

	init_logical_address(log_addr);
	init_logical_address(last_valid_log_addr);

	copyLogicalAddress(last_valid_log_addr, GET_RECLAIMED_ADDRESS_PTR());
	SET_LOGICAL_OFFSET(last_valid_log_addr, GET_LOGICAL_OFFSET(last_valid_log_addr)-1);
	cp_more_than_one_page = IS_CHECKPOINT_MORE_THAN_ONE_PAGE(nBytes);
	last_valid_offset = GET_RECLAIMED_OFFSET()-1;
	cp_size = SEQ_SEG_HEADER_PAGES_COUNT+CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+nBytes);

//	PRINT("\nfindCheckpointAndTruncate() - starting");
//	PRINT_MSG_AND_NUM(", cp_size= ",cp_size);
//	PRINT_MSG_AND_NUM(", nBytes= ",nBytes);

	/* assume we have pending VOTs. if there arent any, we will discover it later*/
	*pending_VOTs = 1;

	/* check if we have any segments allocated*/
	if(IS_SEG_EMPTY(GET_RECLAIMED_SEGMENT()))
		return ERROR_NO_CP;

	/* if we have only 1 segment allocated
	 * check if we wrote checkpoint size blocks after header
	 * if not - truncate the segment*/
	if(GET_SEG_MAP_NSEGMENTS(seg_map_ptr) == 1){
		if(last_valid_offset < cp_size-1){
			truncateCurrentSegment();
			*pending_VOTs = 0;
			return ERROR_NO_CP;
		}
	}

	/* init logical address and start iterating backwards*/
	SET_LOGICAL_SEGMENT(log_addr, GET_RECLAIMED_SEGMENT());
	SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(last_valid_log_addr));

//	isOffsetLessThanCheckpoint = GET_LOGICAL_OFFSET(log_addr) < cp_size-1;

//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - b4 findCheckpointInSegment rec offset ",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment ",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot ",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" rec offset=", GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" log_addr->segment_num ",GET_LOGICAL_SEGMENT(log_addr));
//	PRINT_MSG_AND_NUM(" log_addr->offset ",GET_LOGICAL_OFFSET(log_addr));
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - about to find cp in seg. cp_more_than_one_page=",cp_more_than_one_page);

	/* if we didn't find a checkpoint 1 is returned*/
	res = findCheckpointInSegment(flags, log_addr, cp_more_than_one_page);

//	PRINT("\nfindCheckpointAndTruncate() - finished first attempt of trying to find cp in seg. ");
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - logical offset returned=",GET_LOGICAL_OFFSET(log_addr));
	/* return to the previously written segment if
	 * segment header write was not completed (checkpoint incomplete)
	 */
	if(res){
//		PRINT("\nfindCheckpointAndTruncate() - return to prev seg. truncate segment ");
		res = truncateCurrentSegment();

//		PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate - seg_map_ptr->new_slot_id ",seg_map_ptr->new_slot_id);
		/* if we are not in reclamation state then we should decrement segments count */
		if(!IS_STATE_RECLAMATION()){
//			PRINT("\nfindCheckpointAndTruncate() - state is not reclamation, decrement nSegments");
			seg_map_ptr->nSegments -= 1;
		}

//		PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate - previously_written_segment=",seg_map_ptr->previously_written_segment);
//		PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate - it's slot ",GET_PREVIOUSLY_RECLAIMED_SEGMENT_SLOT());
		/* mark the previous segment as the one we are currently reclaiming */
		SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, GET_PREVIOUSLY_RECLAIMED_SEGMENT_SLOT());
//		PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate - new slot id is now prev seg slot=",GET_PREVIOUSLY_RECLAIMED_SEGMENT_SLOT());
		SET_RECLAIMED_SEGMENT(GET_SEG_MAP_PREV_SEG(seg_map_ptr));
		SET_SEG_MAP_PREV_SEG(seg_map_ptr, SEQ_NO_SEGMENT_NUM);

//		PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate - after switch prev seg=",seg_map_ptr->previously_written_segment);
//		if(!IS_SEG_EMPTY(GET_SEG_MAP_PREV_SEG(seg_map_ptr)))
//			PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate - it's slot ",GET_PREVIOUSLY_RECLAIMED_SEGMENT_SLOT());

		if(GET_SEG_MAP_NSEGMENTS(seg_map_ptr)==0){
//			PRINT("\nfindCheckpointAndTruncate() - no segments left");
			SET_RECLAIMED_OFFSET(SEQ_NO_PAGE_OFFSET);
			SET_RECLAIMED_SEGMENT(SEQ_NO_SEGMENT_NUM);
			return ERROR_NO_CP;
		}

		/* page offset is now the last page in segment */
//		SET_RECLAIMED_OFFSET(SEQ_PAGES_PER_SLOT);
//		PRINT_MSG_AND_HEX("\nb4 set rec addr log addr hex=", *CAST_TO_UINT32(GET_RECLAIMED_ADDRESS_PTR()));
		SET_BYTE(GET_RECLAIMED_ADDRESS_PTR(),0, SEQ_PAGES_PER_SLOT & 0xff);
//		PRINT_MSG_AND_HEX("\nafter setting byte 0 log addr hex=", *CAST_TO_UINT32(GET_RECLAIMED_ADDRESS_PTR()));
		SET_BYTE(GET_RECLAIMED_ADDRESS_PTR(),1, (((SEQ_PAGES_PER_SLOT & 0xf00) >> 8) | (GET_BYTE(GET_RECLAIMED_ADDRESS_PTR(),1) & 0xf0)));
//		PRINT_MSG_AND_HEX("\nafter setting byte 1 log addr hex=", *CAST_TO_UINT32(GET_RECLAIMED_ADDRESS_PTR()));
//		PRINT_MSG_AND_NUM("\nafter setting max offset, rec offset=", GET_RECLAIMED_OFFSET());
//		PRINT_MSG_AND_HEX("\nSEQ_PAGES_PER_SLOT=", SEQ_PAGES_PER_SLOT);

		/* we got to the previous segment. here we definitely have a checkpoint .
		 * iterate until we find it*/
		SET_LOGICAL_SEGMENT(log_addr, GET_RECLAIMED_SEGMENT());
		SET_LOGICAL_OFFSET(log_addr, GET_RECLAIMED_OFFSET()-1);

		/* dont forget to mark last valid log addr, so we'd know we moved
		 * to prev seg*/
		copyLogicalAddress(last_valid_log_addr, log_addr);

//		PRINT("\nfindCheckpointAndTruncate() - try again to find cp ");
//		PRINT_MSG_AND_HEX("\nlog addr hex=", *CAST_TO_UINT32(log_addr));

//		PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate - log_addr->page_offset ",log_addr->page_offset);
//		PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate - log_addr->segment_num ",log_addr->segment_num);
//		PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate - it's slot is ",seg_map_ptr->seg_to_slot_map[log_addr->segment_num]);

		res = findCheckpointInSegment(flags, log_addr, cp_more_than_one_page);
//		PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - findCheckpointInSegment res=", res);
		/* must have cp*/
		assert(!res);
	}
//	assert(0);
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - do we have prev seg? ",!IS_SEG_EMPTY(GET_SEG_MAP_PREV_SEG(seg_map_ptr)));
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - prev seg= ",GET_SEG_MAP_PREV_SEG(seg_map_ptr));
	/* if we iterated backwards to find the checkpoint
	 * and it is not the segment's first checkpoint then
	 * we can assume we have no pending VOTs*/
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - b4 setting pendingVOTs log addr offset=",GET_LOGICAL_OFFSET(log_addr));
//	PRINT_MSG_AND_NUM(", nsegments=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM(", cp_size=",cp_size);
	if(GET_LOGICAL_OFFSET(log_addr) < SEQ_MAX_LOGICAL_OFFSET &&
	   GET_LOGICAL_OFFSET(log_addr) > cp_size-1){
		*pending_VOTs = 0;
	}
	/* if we are at the very first checkpoint, no VOTs for sure*/
	else if(GET_SEG_MAP_NSEGMENTS(seg_map_ptr) == 1 &&
			GET_LOGICAL_OFFSET(log_addr) <  SEQ_MAX_LOGICAL_OFFSET &&
	   		GET_LOGICAL_OFFSET(log_addr) == cp_size-1){
			*pending_VOTs = 0;
	}

	/* NOTICE - we do not continue to search for checkpoints in previous segments.
	 * This is approximately as costly and time consuming as marking VOTs again*/

	/* TODO: possible optimization - if the cp is the header's, and we iterated backwards
	 * check (if exists) previous segment for checkpoints.
	 * if it has one that is not in the header, then we can repeat check*/

//	PRINT("\nfindCheckpointAndTruncate() - about to delete after checkpoint ");
//	PRINT_MSG_AND_NUM("\n last_valid_log_addr offset=", GET_LOGICAL_OFFSET(last_valid_log_addr))
//	PRINT_MSG_AND_NUM(" seg=", GET_LOGICAL_SEGMENT(last_valid_log_addr))
//	PRINT_MSG_AND_NUM("\n log_addr offset=", GET_LOGICAL_OFFSET(log_addr))
//	PRINT_MSG_AND_NUM(" seg=", GET_LOGICAL_SEGMENT(log_addr))
//	PRINT_MSG_AND_NUM("\nlog_addr->page_offset ",log_addr->page_offset);

	/* delete everything after the checkpoint*/
	while (!IS_PAGES_IN_IDENTICAL_EU(GET_LOGICAL_OFFSET(last_valid_log_addr), GET_LOGICAL_OFFSET(log_addr))){
//		PRINT("\nfindCheckpointAndTruncate() - pages not identical");
		/* erase eu */
//		PRINT("\nfindCheckpointAndTruncate() - logicalAddressToPhysical");
		SEQ_VERIFY(!nandErase(logicalAddressToPhysical(last_valid_log_addr)));
//		PRINT_MSG_AND_NUM("\n erased EU in =", GET_LOGICAL_OFFSET(last_valid_log_addr))
		/* iterate last valid address to prev EU end */
		SET_LOGICAL_OFFSET(last_valid_log_addr, GET_LOGICAL_OFFSET(last_valid_log_addr)- NAND_PAGES_PER_ERASE_UNIT);
		SET_LOGICAL_OFFSET(last_valid_log_addr, SEQ_SHIFT_PAGE_OFFSET_TO_EU_END(GET_LOGICAL_OFFSET(last_valid_log_addr)));

		/* don't forget to set reclaimed address one page ahead (for reclaimed slots)*/
		SET_RECLAIMED_OFFSET(GET_LOGICAL_OFFSET(last_valid_log_addr)+1);
	}

//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - log_addr->page_offset ",GET_LOGICAL_OFFSET(log_addr));
//	PRINT_MSG_AND_NUM(" rec offset ",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM("\nmatching slot num is ",seg_map_ptr->seg_to_slot_map[log_addr->segment_num]);
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - 10. slot of SEQ_FIRST_RESERVE_SEGMENT =",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,SEQ_FIRST_RESERVE_SEGMENT));
	/* at this point the checkpoint and last written page are in the same eu */
	/* - if they are identical - we are done*/
	if(GET_LOGICAL_OFFSET(last_valid_log_addr)  != GET_LOGICAL_OFFSET(log_addr)){
		/* - not identical - we should erase everything after the last checkpoint */
//		PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - about to copyBackEU until offset ",GET_LOGICAL_OFFSET(log_addr));
//		PRINT_MSG_AND_NUM(" seg ",GET_LOGICAL_SEGMENT(log_addr));
//		PRINT_MSG_AND_NUM(" slot ",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,GET_LOGICAL_SEGMENT(log_addr)));
		SEQ_VERIFY(!copyBackEU(log_addr));
	}

	SET_RECLAIMED_OFFSET(GET_LOGICAL_OFFSET(log_addr)+1);
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - b4 readCheckpointToMemory rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));

//	PRINT("\nfindCheckpointAndTruncate() - about ot readCheckpointToMemory ");
//	PRINT_MSG_AND_NUM("\nlog_addr->page_offset ",log_addr->page_offset);
//	PRINT_MSG_AND_NUM("\nlog_addr->segment_num ",log_addr->segment_num);
//	PRINT_MSG_AND_NUM("\nmatching slot num is ",seg_map_ptr->seg_to_slot_map[log_addr->segment_num]);
//	PRINT_MSG_AND_NUM("\nfindCheckpointAndTruncate() - pending_VOTs=", *pending_VOTs);

	SEQ_VERIFY(!readCheckpointToMemory(data, nBytes, flags, log_addr));
//	PRINT("\nfindCheckpointAndTruncate() - finished readCheckpointToMemory ");
	init_buf(sequencing_buffer);
	return 0;
}

/**
 * @brief
 * read the block in logical address b to data
 * @param b logical address to read
 * @param data buffer toread data to
 * @return 0 if successful, ERROR_IO if a read error occurs
 */
error_t readBlock(logical_addr_t b, void *data){
	uint32_t phy_addr;
//	for(phy_addr = 0; phy_addr<NAND_PAGE_SIZE; phy_addr++		){
//		PRINT_MSG_AND_NUM("\nbefore writing byte ",phy_addr);
//		PRINT_MSG_AND_NUM(" = ",CAST_TO_UINT8(data)[phy_addr]);
//	}
//	PRINT("\nreadBlock() - starting. about to do logicalAddressToPhysical()");
//	PRINT_MSG_AND_NUM(" offset=", GET_LOGICAL_OFFSET(b));
//	PRINT_MSG_AND_NUM(" seg=", GET_LOGICAL_SEGMENT(b));
//	PRINT_MSG_AND_NUM(" slot=", GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, GET_LOGICAL_SEGMENT(b)));
	phy_addr = logicalAddressToPhysical(b);
//	PRINT_MSG_AND_NUM("\nreadBlock() - about to read from page ",phy_addr);
//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nreadBlock() - flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
	assert(!IS_PHY_ADDR_EMPTY(phy_addr));
//	PRINT("\nreadBlock() - read page");
	if(nandReadPageTotal(sequencing_buffer, phy_addr)){
		return ERROR_IO;
	}
//	PRINT("\nreadBlock() - read success");
	fsMemcpy(data, sequencing_buffer, NAND_PAGE_SIZE);
//	for(phy_addr = 0; phy_addr<NAND_PAGE_SIZE; phy_addr++		){
//		PRINT_MSG_AND_NUM("\nreadBlock() - byte[",phy_addr);
//		PRINT_MSG_AND_NUM("]= ",CAST_TO_UINT8(data)[phy_addr]);
//	}
//	PRINT("\nreadBlock() - finished");
	init_buf(sequencing_buffer);
	return 0;
}

/**
 * @brief
 * traverse list of VOTs of transaction.
 * read block from flash, and if it contains VOTs, copy it's contents to RAM at VOTs.
 * prev is set to the address of the block written before b by it's transaction.
 *
 * @param b logical address of a block to read
 * @param VOTs buffer in RAM to read VOT's block to
 * @param prev logical address of block before b in the linked list of VOT's
 * @param isVOTs indicator whether the block contains vots
 * @return 0 if successful. if failed to read the block 1 is returned.
 */
error_t readVOTsAndPrev(logical_addr_t b, void* VOTs, logical_addr_t prev, bool_t *isVOTs){
	INIT_FLAGS_STRUCT_AND_PTR(flags);
//	PRINT("\nreadVOTsAndPrev() - logicalAddressToPhysical");
	uint32_t phy_addr;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(prev_addr);

	phy_addr = logicalAddressToPhysical(b);
	init_logical_address(prev_addr);

	*isVOTs = 0;
	assert(!IS_PHY_ADDR_EMPTY(phy_addr));
//	PRINT("\nreadVOTsAndPrev() - read flags");
	nandReadPageFlags(flags, phy_addr);

//	PRINT_MSG_AND_NUM("\nreadVOTsAndPrev() - read block from offset", b->page_offset);
//	PRINT_MSG_AND_NUM("\nreadVOTsAndPrev() - and segment", b->segment_num);
	/* if the page is not vots, return */

	extract_prev_address(flags, prev);

//	PRINT_MSG_AND_NUM("\nreadVOTsAndPrev() - vots flag true?", GET_VOTS_FLAG(flags) == VOTS_FLAG_TRUE);
	/* if this is a vots buffer, read the block */
	if(GET_VOTS_FLAG(flags) == VOTS_FLAG_TRUE){
		*isVOTs = 1;
		return readBlock(b, VOTs);
	}

	return 0;
}




/**
 * @brief
 * booting.
 * find reserve segments - find the first valid EU in every slot. REASON - if its reserve slot, we have to do this.
 *
 * for every segment with a header, verify validity of header:
 * if (not valid)/empty  - erase it (to recover from possible failed program).
 * we now have the reserve segments allocated
 *
 * check that number of reserve segments is as expected:
 * if not - continue allocating them, and continue to allocating first segment
 *
 * if yes - for every slot whose first EU is bad, find the EU we would have allocated as reserve for it, and check if
 * it contains it's header. if not/invalid header - erase it. otherwise mark in seg map.
 *
 * we should have marked by now in seg map the new slot. check state
 * a. regular allocation
 * b. reclamation - maybe we failed during weare leveling/changing segment
 * and continue accordingly
 *
 * Assumptions:
 * 1. all slots have at least 2 valid EU. otherwise, the flash is really not usable
 * @param data file system data
 * @param nBytes size of file system data
 * @param cp_write_p checkpoint writer
 * @return 1 if n error occured. 0 if successful
 */
error_t sequencingBooting(void* data, uint32_t nBytes, bool_t *pendingVOTs, checkpoint_writer cp_write_p){
	uint32_t doublyAllocatedSegment, secondSlot;
	uint32_t last_valid;
	uint32_t cp_pages_size = (CALCULATE_IN_PAGES(nBytes+SEQ_CHECKPOINT_SIZE))+1;
	bool_t copyBackHandled = 0, badFirstEU = 0, cpWritten;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	*pendingVOTs = 1;

	/* by default we have no reserve segment in wear leveling*/
	doublyAllocatedSegment = SEQ_NO_SEGMENT_NUM;
	secondSlot = SEQ_NO_SLOT;
//
//	PRINT("\nsequencingBooting() - start");

	init_seg_map();
	init_obs_counters();

//	PRINT_MSG_AND_NUM("\nsequencingBooting() - b4 handleReserveSegments() new_slot_id=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
	/* get all reserve segmetns markings*/
	SEQ_VERIFY(!handleReserveSegments(&badFirstEU, &doublyAllocatedSegment, &secondSlot));
//	PRINT("\nsequencingBooting() - handleReserveSegments success");
//	PRINT_MSG_AND_NUM("\nsequencingBooting() - after handleReserveSegments GET_RECLAIMED_SEGMENT()=",GET_RECLAIMED_SEGMENT());

//	PRINT_MSG_AND_NUM("\nsequencingBooting() - after handle reserve setgs, first reserve slot=",GET_SLOT_BY_SEGMENT(SEQ_FIRST_RESERVE_SEGMENT));
//	PRINT_MSG_AND_NUM(" is eu reserve=",GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" reserve eu=",GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr));

	/* we now definitely have all reserve segments allocated properly.
	 * check if aborted in the middle of wear leveling a RESERVE slot */
	if(doublyAllocatedSegment != SEQ_NO_SEGMENT_NUM){
		SEQ_VERIFY(!handleDoubleReserveSlot(doublyAllocatedSegment, GET_SLOT_BY_SEGMENT(doublyAllocatedSegment), secondSlot));
	}
//	PRINT("\nsequencingBooting() - finished handleDoubleReserveSlot success");
	/* now that we have reserve segments, we can look for copy back EU*/
	/* get the second */

	SEQ_VERIFY(!handleCopyBack(&copyBackHandled));

//	PRINT("\nsequencingBooting() - finished handleCopyBack success");
//	PRINT_MSG_AND_NUM(" is eu reserve=",GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" reserve eu=",GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr));
	/* TODO: possible optimiaztion - only activate handleRegularSegments() if during handleReserveSegments()
	 * we found a slot whose first EU is bad.
	 * Dosen't work, try in the future*/
	/* if we encountered any bad first EU's then we need to
	 * mark data segments again, since only now we know where are the reserve segments*/
//	if(badFirstEU){
//		PRINT("\nsequencingBooting() - there was a bad first eu. handle regular segments");
//		/* now we can find header for every data segment*/
//		handleRegularSegments(nBytes);
//	}

//	PRINT("\nsequencingBooting() - execute handleRegularSegments()");
	/* now we can find header for every data segment*/
	SEQ_VERIFY(!handleRegularSegments(nBytes));

//	PRINT("\nsequencingBooting() - finished handleRegularSegments success");
//	PRINT_MSG_AND_NUM("\nsequencingBooting() - after handleRegularSegments rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	if(!IS_SEG_EMPTY(GET_RECLAIMED_SEGMENT())){
//		PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//		PRINT_MSG_AND_NUM(" reclamation? ",IS_STATE_RECLAMATION());
//	}
//	PRINT_MSG_AND_NUM(" new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" is eu reserve=",GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" reserve eu=",GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr));

//	{
//		INIT_FLAGS_STRUCT_AND_PTR(flags_net);
//		initFlags(flags_net);
//		nandReadPageFlags(flags_net, CALC_ADDRESS(126, 2, 0));
//		PRINT_MSG_AND_HEX("\nsequencingBooting() -  b4 handleFlashIsEmpty() flags of page 64608: slot eu offset=", GET_SLOT_EU_OFFSET(flags_net));
//		PRINT_MSG_AND_HEX(" copyback flag=", GET_COPY_BACK_FLAG(flags_net));
//	}
//	PRINT_MSG_AND_NUM("\nDATA_SEGMENTS_COUNT()=",DATA_SEGMENTS_COUNT());
	/* if we haven't allocated regular segments -
	 * logical address is pointing at seg 0, offset 0. set slot 0 to seg 0.*/
	if(DATA_SEGMENTS_COUNT() == 0){
//		PRINT("\nsequencingBooting() - handle flash is empty");
		*pendingVOTs = 0;
//		PRINT_MSG_AND_NUM("\nsequencingBotting() - cp_pages_size=",cp_pages_size);
		/* set counters*/
		SET_OBS_COUNT(0);
		SET_FREE_COUNTER((SEQ_SEGMENTS_COUNT)*(SEQ_PAGES_PER_SLOT-cp_pages_size));
		return handleFlashIsEmpty(cp_write_p);
	}

//	PRINT("\nsequencingBooting() - starting findLastValidPageInSlot");
	/* we have more than one segment, so we definitely have a reclaimed one.
	 * there is at least one valid page written in the segment*/
	SEQ_VERIFY(!findLastValidPageInSlot(&last_valid, GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr)));

//	PRINT_MSG_AND_NUM("\nsequencingBooting() - finished findLastValidPageInSlot success. last_valid=",last_valid);
//	PRINT_MSG_AND_NUM(" last valid reserve?=",isAddressReserve(last_valid));
//	PRINT_MSG_AND_NUM(" is eu reserve=",GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" reserve eu=",GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr));
	/* set reclaimed offset to page after last valid*/
	if(!isAddressReserve(last_valid)){
		SET_RECLAIMED_OFFSET(CALC_OFFSET_IN_SLOT(last_valid));
	}
	else{
		SET_RECLAIMED_OFFSET(calcOriginalSlotOffset(flags, last_valid));
		initFlags(flags);
	}

//	PRINT_MSG_AND_NUM("\nsequencingBooting() - b4 handleInvalidPrograms(), rec offset=",GET_RECLAIMED_OFFSET());
////	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	if(!IS_SEG_EMPTY(GET_RECLAIMED_SEGMENT())){
//		PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//		PRINT_MSG_AND_NUM(" reclamation? ",IS_STATE_RECLAMATION());
//	}
//	PRINT_MSG_AND_NUM(" new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" is eu reserve=",GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" reserve eu=",GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr));

	SEQ_VERIFY(!handleInvalidPrograms(last_valid, cp_pages_size));

//	PRINT_MSG_AND_NUM("\nsequencingBooting() - after handleInvalidPrograms() rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	if(!IS_SEG_EMPTY(GET_RECLAIMED_SEGMENT())){
//		PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//		PRINT_MSG_AND_NUM(" reclamation? ",IS_STATE_RECLAMATION());
//	}
//	PRINT_MSG_AND_NUM(" new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" nsegments=",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" prev seg=",GET_SEG_MAP_PREV_SEG(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" is eu reserve=",GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" reserve eu=",GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr));
//	{
//		uint8_t buf1[NAND_TOTAL_SIZE], byte = 'a';
//		int32_t j;
//		nandReadPageTotal(buf1, 63650);
//		for(j=0; j< FS_BLOCK_SIZE; j++){
//			if(!COMPARE(buf1[j], byte)){
//				PRINT("\nbefore findCheckpointAndTruncate() block 63650 is not as expected");
//				printBlock(buf1);
//				break;
//			}
//		}
//	}
	SEQ_VERIFY(!findCheckpointAndTruncate(data, nBytes, pendingVOTs));

	/* if we are not in reclamation, and the last checkpoint is not the header checkpoint
	 * then we should undo the incrementing of free pages count performed by last commit()*/
	if(!IS_STATE_RECLAMATION() && GET_RECLAIMED_OFFSET() > cp_pages_size){
		DECREMENT_FREE_COUNT();
//		L("decremented free count to %d", GET_FREE_COUNTER());
	}
//	PRINT_MSG_AND_NUM("\nsequencingBooting() - after findCheckpointAndTruncate() rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" obs count=",GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" free count=",GET_FREE_COUNTER());
//
//	if(!IS_SEG_EMPTY(GET_RECLAIMED_SEGMENT())){
//		PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//		PRINT_MSG_AND_NUM(" reclamation? ",IS_STATE_RECLAMATION());
//	}
//	PRINT_MSG_AND_NUM(" new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" is eu reserve=",GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" reserve eu=",GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr));
	/* now that we have address marked etc. do findNextFreePage()
	 * in case before rebooting we failed in the middle of findNextFreePage() */
//	PRINT_MSG_AND_NUM("\nsequencingBooting() - before findNextFreePage() rec offset=",GET_RECLAIMED_OFFSET());
//	{
//		uint8_t buf1[NAND_TOTAL_SIZE], byte = 'a';
//		int32_t j;
//		nandReadPageTotal(buf1, 63650);
//		for(j=0; j< FS_BLOCK_SIZE; j++){
//			if(!COMPARE(buf1[j], byte)){
//				PRINT("\nbefore findNextFreePage block 63650 is not as expected");
//				nandReadPageTotal(buf1, CALC_ADDRESS(49, 0, 2));
//				for(j=0; j< FS_BLOCK_SIZE; j++){
//					if(!COMPARE(buf1[j], byte)){
//						PRINT("\neven in reclaimed page it is not as expected");
//						break;
//					}
//				}
//
//				if(j==FS_BLOCK_SIZE){
//					PRINT("\nin reclaimed page it is as expected")
//				}
//				break;
//			}
//		}
//	}
//	PRINT_MSG_AND_NUM(" rec segmnet ",GET_RECLAIMED_SEGMENT());
//	if(!IS_SEG_EMPTY(GET_RECLAIMED_SEGMENT())){
//		PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//		PRINT_MSG_AND_NUM(" reclamation? ",IS_STATE_RECLAMATION());
//	}
//	PRINT_MSG_AND_NUM(" new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
	/* if we are not in reclamation.
	 * free pages is
	 * total free pages in data segemnts (-pages to be occupied by segments headers, in not yet written segments)
	 * +
	 * free pages in reclaimed slot
	 */
	SEQ_VERIFY(!findNextFreePage(&cpWritten, cp_write_p, 0));
//	PRINT_MSG_AND_NUM("\nsequencingBooting() - after findNextFreePage() rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" obs count=",GET_OBS_COUNT());
//	PRINT_MSG_AND_NUM(" free count=",GET_FREE_COUNTER());
//	PRINT_MSG_AND_NUM(" is state reclaamtion?",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" free counter=",GET_FREE_COUNTER());

//	{
//		uint8_t buf1[NAND_TOTAL_SIZE], byte = 'a';
//		int32_t j;
//		nandReadPageTotal(buf1, 63650);
//		for(j=0; j< FS_BLOCK_SIZE; j++){
//			if(!COMPARE(buf1[j], byte)){
//				PRINT("\nafter findNextFreePage block 63650 is not as expected");
//				nandReadPageTotal(buf1, CALC_ADDRESS(49, 0, 2));
//				for(j=0; j< FS_BLOCK_SIZE; j++){
//					if(!COMPARE(buf1[j], byte)){
//						PRINT("\neven in reclaimed page it is not as expected");
//						break;
//					}
//				}
//
//				if(j==FS_BLOCK_SIZE){
//					PRINT("\nin reclaimed page it is as expected")
//				}
//				break;
//			}
//		}
//	}
	init_buf(sequencing_buffer);

//	{
//		int32_t i;
//		for(i=0; i< SEQ_N_SLOTS_COUNT; i++){
//			PRINT_MSG_AND_NUM("\nseg #", i);
//			PRINT_MSG_AND_NUM(" slot=", GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, i));
//		}
//	}
	return 0;
}
