/** @file sequencingUtils.c
 * Sequencing layer auxiliary and low-level functions
 */
/* for debugging */
#include <system.h>
#ifdef Debug
#include <test/macroTest.h>
#endif
#include <peripherals/nand.h>

#include <src/sequencing/sequencing.h>
#include <src/sequencing/sequencingUtils.h>
#include <src/sequencing/lfsr.h>
#include <utils/memlib.h>

extern segment_map *seg_map_ptr;
extern obs_pages_per_seg_counters *obs_counters_map_ptr;
extern int valid_copies;

/**
 * @brief
 * set logical offset in a given address
 *
 * @param log_addr logical address
 * @offset the offset to set
 */
void setLogicalOffset(logical_addr_t log_addr, uint32_t offset){
//	PRINT_MSG_AND_NUM("\nsetLogicalOffset() - set offset to ", offset);
	SET_BYTE(log_addr,0, (offset) & 0xff);
//	PRINT_MSG_AND_HEX("\nafter setting byte 0 log addr hex=", *CAST_TO_UINT32(log_addr));
	SET_BYTE(log_addr,1, ((((offset) & 0xf00) >> 8) | (GET_BYTE(log_addr,1) & 0xf0)));
//	PRINT_MSG_AND_HEX("\nafter setting byte 1 log addr hex=", *CAST_TO_UINT32(log_addr));
}

void setLogicalSegment(logical_addr_t log_addr, uint32_t seg_num){
	SET_BYTE(log_addr,1, ((GET_BYTE(log_addr,1) & 0x0f) | (((seg_num) & 0x0f) << 4)));
	SET_BYTE(log_addr,2, ((seg_num) & 0xff0) >> 4);
}

/**
 * Check if an eu is in the valid eus queue
 *
 * @param eu_addr eu address
 * @return 1 if yes, 0 if no
 */
bool_t
seq_is_eu_known_valid(uint32_t eu_addr)
{
	uint32_t i;

	for(i=0; i< SEQ_VALID_EU_Q_SIZE; i++){
		if(GET_SEG_VALID_EUS(seg_map_ptr)[i] == eu_addr) {
			return 1;
		}
	}

	return 0;
}

/**
 * Remeber eu as a known valid eu
 * TODO: very bad programming. fix in the future
 *
 * @param eu_addr valid eu address
 */
void
seq_set_eu_known_valid(uint32_t eu_addr)
{
	uint32_t i;

	/* move whole queue one place forword (and enqueue last) */
	for(i=SEQ_VALID_EU_Q_SIZE-1; i>0 ; i--){
		GET_SEG_VALID_EUS(seg_map_ptr)[i] = GET_SEG_VALID_EUS(seg_map_ptr)[i-1];
	}

	/* insert first */
	GET_SEG_VALID_EUS(seg_map_ptr)[0] = eu_addr;
}

/**
 * Check eu status. First check in-memory records of valid eus
 * If not there check actual memory
 *
 * @param eu_addr eu address
 * @return 1 if eu ok, 0 otherwise
 */
static bool_t
seq_check_eu_status(uint32_t eu_addr)
{
	eu_addr = CALC_EU_START(eu_addr);

//	L("check if addr %u is valid", eu_addr);
	if(seq_is_eu_known_valid(eu_addr)) {
//		L("addr %u is indeed valid according to queue", eu_addr);
		return 1;
	}

//	L("check if addr %u is valid", eu_addr);
	if(nandCheckEuStatus(eu_addr)){
		seq_set_eu_known_valid(eu_addr);
		return 1;
	}

	return 0;
}

/**  @brief
 * Reserve EU search:
 * EU is bad. search for the page in a reserve EU
 * iterate all reserve segments until we find the reserve EU
 * (reserve segments are always the last)
 * @param flags pointer to flags struct to be used in the function
 * @param phy_addr the physical address we search for replacement
 * @return reserve physical address if successful, SEQ_PHY_ADDRESS_EMPTY otherwise
 */
uint32_t logicalAddressToReservePhysical(spare_area_flags *flags, uint32_t phy_addr){
	uint32_t seg_idx;
	uint32_t reserve_slot_address, reserve_slot_eu_offset;
	uint32_t eu_page_offset = CALC_OFFSET_IN_EU(phy_addr);
	uint32_t eu_offset      = CALC_EU_OFFSET(phy_addr);
	uint32_t expected_slot_id_and_eu_offset = CALCULATE_SLOT_ID_AND_EU_OFFSET(phy_addr);

//	PRINT_MSG_AND_HEX("\nlogicalAddressToReservePhysical() - expected_slot_id_and_eu_offset=",expected_slot_id_and_eu_offset);
	for(seg_idx= SEQ_FIRST_RESERVE_SEGMENT; seg_idx<SEQ_N_SLOTS_COUNT;seg_idx++){
		/* iterate all EUs in segment until we find reserve EU
		 * start from EU in parallel offset, and continue cycilcally */
		reserve_slot_eu_offset = eu_offset;

		reserve_slot_address   = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg_idx],reserve_slot_eu_offset,0);
//		PRINT_MSG_AND_NUM("\nlogicalAddressToReservePhysical() - iterating reserve segment - ",seg_idx);
//		PRINT_MSG_AND_NUM(" in address ",reserve_slot_address);

		do{
//			PRINT_MSG_AND_NUM("\nlogicalAddressToReservePhysical() - iterating it's reserve eu - ",reserve_slot_eu_offset);
//			PRINT_MSG_AND_NUM(" reserve_slot_address=",reserve_slot_address);

			/* if the eu is valid, check if it is the one we want
			 * and that it is not the copyback eu */
			if(nandCheckEuStatus(reserve_slot_address)){				//
				initFlags(flags);
				nandReadPageFlags(flags, reserve_slot_address);//

//				PRINT_MSG_AND_HEX(" flags slot_id_and_eu_offset=",GET_SLOT_EU_OFFSET(flags));
//				PRINT_MSG_AND_HEX(" cb flag=",GET_COPY_BACK_FLAG(flags));
				/* verify it is the reserve we're looking for, and not a copyback*/
				if(GET_SLOT_EU_OFFSET(flags)  == expected_slot_id_and_eu_offset &&
				   GET_COPY_BACK_FLAG(flags)  == COPY_BACK_FLAG_FALSE)

				   /* return the address (eu+offset in eu) */
				   return reserve_slot_address+eu_page_offset;
			}

			/* not verified. advance to next EU address*/
			reserve_slot_eu_offset +=1;
			reserve_slot_address   += NAND_PAGES_PER_ERASE_UNIT;

			/* if overflowed the slot pages, return to start (cyclic)*/
			if(reserve_slot_eu_offset == SEQ_EUS_PER_SLOT){
				reserve_slot_address   = reserve_slot_address - SEQ_PAGES_PER_SLOT;
				reserve_slot_eu_offset = 0;
			}
		}while (reserve_slot_eu_offset != eu_offset);
	}

//	PRINT("\nlogicalAddressToReservePhysical() couldnt find reserve");
	return SEQ_PHY_ADDRESS_EMPTY;
}

/**  @brief
 * translate logical address to physical.
 * - check if address is in the reclaimed slot
 * - check if it is in a reserve eu
 * - if in reserve eu call logicalAddressToReservePhysical
 * @return reserve physical address if successful, SEQ_PHY_ADDRESS_EMPTY otherwise
 */
uint32_t logicalAddressToPhysical(logical_addr_t logical_addr){
	uint32_t phy_page_addr;
	INIT_FLAGS_STRUCT_AND_PTR(flags);

//	if(GET_LOGICAL_OFFSET(logical_addr) == 2 && GET_LOGICAL_SEGMENT(logical_addr) ==51){
//	PRINT("\nlogicalAddressToPhysical() - starting");
//	PRINT_MSG_AND_HEX(" hex addr=",((uint32_t*)(logical_addr)));
//	PRINT_MSG_AND_NUM(" seg=",GET_LOGICAL_SEGMENT(logical_addr));
//	PRINT_MSG_AND_NUM(" slot=",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,GET_LOGICAL_SEGMENT(logical_addr)));
//	PRINT_MSG_AND_NUM(" offset=",GET_LOGICAL_OFFSET(logical_addr));
//	}
	/* verify there was any physical address */
	assert(IS_SLOT_ALLOCATED_FOR_SEGMENT(seg_map_ptr,GET_LOGICAL_SEGMENT(logical_addr)));
	/* verify offset doesn't overflow slot */
//	if(GET_LOGICAL_OFFSET(logical_addr) == 2 && GET_LOGICAL_SEGMENT(logical_addr) ==51){
//	PRINT_MSG_AND_NUM("\nlogical_addr offset logical_addr=",GET_LOGICAL_OFFSET(logical_addr));
//	PRINT_MSG_AND_NUM("\nSEQ_PAGES_PER_SLOT=",SEQ_PAGES_PER_SLOT);
//	PRINT_MSG_AND_NUM("\nGET_LOGICAL_OFFSET(logical_addr) < SEQ_PAGES_PER_SLOT?=",GET_LOGICAL_OFFSET(logical_addr) < SEQ_PAGES_PER_SLOT);
//	}
	assert(GET_LOGICAL_OFFSET(logical_addr) < SEQ_PAGES_PER_SLOT);
//	if(GET_LOGICAL_OFFSET(logical_addr) == 2 && GET_LOGICAL_SEGMENT(logical_addr) ==51){
//	PRINT_MSG_AND_NUM("\nseg_map_ptr->nSegments=",seg_map_ptr->nSegments);
//	PRINT_MSG_AND_NUM("\nSEQ_SEGMENTS_COUNT=",SEQ_SEGMENTS_COUNT);
//
//	PRINT_MSG_AND_NUM("\nseg_map_ptr->new_slot_id=",seg_map_ptr->new_slot_id);
//	PRINT_MSG_AND_NUM("\nlogicalAddressToPhysical() - rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" new slot=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM("\nlogicalAddressToPhysical() - starting. state rec? ",IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM("\naddress seg is reclaimed? ",IS_ADDRESS_IN_RECLAIMED_SEGMENT(logical_addr, seg_map_ptr));
//	PRINT_MSG_AND_NUM("\naddress offset is reclaimed? ",IS_ADDRESS_IN_RECLAIMED_PAGE(logical_addr, seg_map_ptr));
//	PRINT_MSG_AND_NUM("\nis state reclaim? ",IS_STATE_RECLAMATION());
//	}
	/* check if we are in reclamation, the address is in the reclaimed segment
	   and if we are in the part already reclaimed*/

	if(IS_STATE_RECLAMATION() &&
	   IS_ADDRESS_IN_RECLAIMED_SEGMENT(logical_addr, seg_map_ptr) &&
	   IS_ADDRESS_IN_RECLAIMED_PAGE(logical_addr, seg_map_ptr)){
//	   	if(GET_LOGICAL_OFFSET(logical_addr) == 2 && GET_LOGICAL_SEGMENT(logical_addr) ==51){
//	   	PRINT("\nlogicalAddressToPhysical() - we are in reclamation state ");
//	   	}
		phy_page_addr = CALC_ADDRESS(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr),0,GET_LOGICAL_OFFSET(logical_addr));
	}
	else{
		/* this is supposed to be the page */
		phy_page_addr = CALC_ADDRESS(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,GET_LOGICAL_SEGMENT(logical_addr)),0,GET_LOGICAL_OFFSET(logical_addr));
//		PRINT("\nlogicalAddressToPhysical() - we are not in reclamation state ");
	}
//	PRINT_MSG_AND_NUM("\nseg=", GET_LOGICAL_SEGMENT(logical_addr));
//	PRINT_MSG_AND_NUM(" slot=", GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,GET_LOGICAL_SEGMENT(logical_addr)));
//	PRINT_MSG_AND_NUM(" offset=", GET_LOGICAL_OFFSET(logical_addr));
//	PRINT_MSG_AND_NUM(" new slot id=", GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" state rec=", IS_STATE_RECLAMATION());
//	PRINT_MSG_AND_NUM(" rec seg=", GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec offset=", GET_RECLAIMED_OFFSET());
//	if(GET_LOGICAL_OFFSET(logical_addr) == 2 && GET_LOGICAL_SEGMENT(logical_addr) ==51){
//	PRINT_MSG_AND_NUM("\nlogicalAddressToPhysical() - phy_page_addr= ", phy_page_addr);
//	}

	/* however it may actually be in a reserve EU.
	 * if the EU is ok, return the address */
	if(seq_check_eu_status(CALC_EU_START(phy_page_addr))){
		return phy_page_addr;
	}

//	if(GET_LOGICAL_OFFSET(logical_addr) == 2 && GET_LOGICAL_SEGMENT(logical_addr) ==51){
//	PRINT_MSG_AND_NUM("\nlogicalAddressToPhysical - need to find a reserve for phy_addr ", phy_page_addr);
//	}
	/* get actual eu offset in the slot (not shifted by the page offset bits)
	 * and pass it to function */
	return logicalAddressToReservePhysical(flags,  /* pass this to save memory */
									       phy_page_addr);
}

/**
 * @brief
 * check if a page is already marked as obsolete
 * @return 1 if yes, 0 if no
 */
error_t is_page_marked_obsolete(uint32_t phy_addr){
//	PRINT_MSG_AND_NUM("\nis_page_marked_obsolete() - phy_addr=",phy_addr);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	nandReadPageFlags(flags, phy_addr);

//	{
//		uint8_t buf[NAND_TOTAL_SIZE];
//		nandReadPageTotal(buf, phy_addr);
//		PRINT("\nprint block");
//		printBlock(buf);
//	}
//	PRINT_MSG_AND_HEX("\n byte 15=",flags->bytes[OBSOLETE_BYTE_OFFSET]);
	return IS_PAGE_OBSOLETE(flags);
}

uint32_t get_counter_level(uint32_t seg_id){
#ifndef EXACT_COUNTERS
	if(IS_EVEN(seg_id))
		return obs_counters_map_ptr->counters[seg_id/2] & 0x0f;
	else
		return ((obs_counters_map_ptr->counters[seg_id/2] & 0xf0) >> 4);
#else
	return GET_EXACT_COUNTER_LEVEL(seg_id);
#endif
}

/**
 * @brief
 * update the level of the obsolete counter of segment seg_id
 * @param level current level of segment
 * @param seg_id segment id
 */
void change_counter_level(uint32_t level, uint32_t seg_id){
	uint32_t seg_counter_location = DIV_BY_2(seg_id);
#ifndef EXACT_COUNTERS
	 if(IS_EVEN(seg_id)){
	   obs_counters_map_ptr->counters[seg_counter_location] = ((obs_counters_map_ptr->counters[seg_counter_location] & 0xf0) | level);
	 }
	 else{
	   obs_counters_map_ptr->counters[seg_counter_location] = (obs_counters_map_ptr->counters[seg_counter_location] & 0x0f) | (level << 4);
	 }
#else
	 SET_EXACT_COUNTER_LEVEL(seg_id, level);
#endif
}

/**
 * @brief
 * increment probabailsticaly obsolete counter of segment seg_id
 * @param seg_id segment id
 */
void increment_counter(uint32_t seg_id){
#ifndef EXACT_COUNTERS
	uint32_t  counter_level       = get_counter_level(seg_id);


	/* check if we can increment */
	if(COMPARE(OBS_COUNT_LEVEL_7, get_counter_level(seg_id)))
		return;

	change_counter_level(counter_level+1,seg_id);
#else
	INC_EXACT_COUNTER(seg_id);
#endif

}

/**
 * @brief
 * initialize a buffer in a given size
 * @param buf buffer to initialize
 * @param size buffer size
 */
void init_struct(void *buf, uint32_t size){
	uint8_t *ptr = CAST_TO_UINT8(buf);
	uint32_t i;

	for(i=0; i < size;i++){
		ptr[i] = 0xff;
	}
}

///**
// * @brief
// * auxiliary function, for initializing the sequencing layer buffer
// */
//void init_buf(uint8_t *buf){
//	uint32_t i;
//
//	for(i=0;i<NAND_TOTAL_SIZE; i++){
//		buf[i] = 0xff;
//	}
//}

/**
 * @brief
 * auxiliary, copy logical address details
 * @param to target logical address
 * @param from original logical address
 */
void copyLogicalAddress(logical_addr_t to, logical_addr_t from){
//	PRINT_MSG_AND_NUM("\ncopyLogicalAddress() - from->page_offset=",from->page_offset);
//	PRINT_MSG_AND_NUM("\ncopyLogicalAddress() - from->segment_num=",from->segment_num);
	SET_LOGICAL_OFFSET(to, GET_LOGICAL_OFFSET(from));
	SET_LOGICAL_SEGMENT(to, GET_LOGICAL_SEGMENT(from));
	SET_LOGICAL_SPARE(to, GET_LOGICAL_SPARE(from));
//	PRINT_MSG_AND_NUM("\ncopyLogicalAddress() - to->page_offset=",to->page_offset);
//	PRINT_MSG_AND_NUM("\ncopyLogicalAddress() - to->segment_num=",to->segment_num);
}

/**
 * @brief
 * auxiliary, copy logical address details
 * @param to target logical address
 * @param from original logical address
 */
void copyTruncatedLogicalAddress(logical_addr_t to, logical_addr_t from){
//	PRINT_MSG_AND_NUM("\ncopyLogicalAddress() - from->page_offset=",from->page_offset);
//	PRINT_MSG_AND_NUM("\ncopyLogicalAddress() - from->segment_num=",from->segment_num);
	SET_LOGICAL_OFFSET(to, GET_LOGICAL_OFFSET(from));
	SET_LOGICAL_SEGMENT(to, GET_LOGICAL_SEGMENT(from));
//	PRINT_MSG_AND_NUM("\ncopyLogicalAddress() - to->page_offset=",to->page_offset);
//	PRINT_MSG_AND_NUM("\ncopyLogicalAddress() - to->segment_num=",to->segment_num);
}

/**
 * @brief
 * auxiliary function for allocAndWriteBlock - write a block of data to the next physical address,
 * and increment logical offset. if we are not given a physical address, then we can simply use
 * the next (incremeneted) physical address
 *
 * @param log_addr logical address to which the block is written to
 * @param data buffer
 * @param phy_addr physical address to write data. if 0, use the one from seg_map
 */
void writeBlock(logical_addr_t log_addr, void* data, uint32_t phy_addr){
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, data);
	init_logical_address(log_addr);

//	L("phy_addr %d", phy_addr);

//	PRINT("\nwriteBlock() - before programing check flags");
//	/* verify the flag was written*/
//	PRINT_MSG_AND_HEX("\nwriteBlock() - flags->vots_data_flag",flags->vots_data_flag);
//	PRINT_MSG_AND_HEX("\nwriteBlock() - flags->slot_id_and_eu_offset",flags->slot_id_and_eu_offset);

//	PRINT_MSG_AND_HEX("\nwriteBlock() - data[0]=",CAST_TO_UINT8(data)[0]);
//	PRINT_MSG_AND_NUM("\nwriteBlock() - before marking seg type GET_SEG_TYPE_FLAG(seq_flags)=",GET_SEG_TYPE_FLAG(seq_flags));
	/* set reserve flags if required. must be here, since not all writes are performed using allocAndWriteBlock -
	 * commit(): may be writing using writeBlock() directly */
	if(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr) == IS_EU_RESERVE_TRUE){
//		L("set address as reserve");
		SET_SLOT_EU_OFFSET(seq_flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(CALC_ADDRESS(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr), 0, GET_RECLAIMED_OFFSET())));
		SET_SEG_TYPE_FLAG(seq_flags,SEG_TYPE_RESERVE);
		phy_addr = GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr)+CALC_OFFSET_IN_EU(phy_addr);
//		PRINT_MSG_AND_NUM("\nwriteBlock() - write data to ",phy_addr);
//		PRINT_MSG_AND_NUM("\nwriteBlock() - setting slot_eu_offset to ",GET_SLOT_EU_OFFSET(seq_flags));
	}
	else{
		SET_SEG_TYPE_FLAG(seq_flags,SEG_TYPE_USED);
	}

//	L("writing to page %d", phy_addr);
//	PRINT_MSG_AND_NUM("\nwriteBlock() - is page used?=",IS_PAGE_USED(seq_flags));

//	PRINT_MSG_AND_NUM("\nwriteBlock() -  writing to page",phy_addr);
//	PRINT_MSG_AND_NUM("\nwriteBlock() - cp flag=",GET_CHECKPOINT_FLAG(seq_flags));
//	{
//		int32_t i;
//		for(i=0;i<NAND_TOTAL_SIZE;i++){
////			if(sequencing_buffer[i] == 0xff || sequencing_buffer[i] == 0x72)
////				continue;
//
//			PRINT_MSG_AND_NUM("\n",i);
//			PRINT_MSG_AND_HEX(".", sequencing_buffer[i]);
//		}
//	}

//	if(33047==phy_addr){
//		PRINT_MSG_AND_NUM("\nwriteBlock() - b4 writeBlock() of cp to 34583 cp flag=", GET_CHECKPOINT_FLAG(seq_flags));
//	}
//	L("call nandProgramTotalPage() on phy_addr %d", phy_addr);
	if (nandProgramTotalPage(data, phy_addr)){
//		PRINT("\nwriteBlock() error");
		return;
	}
//	L("writing success");
	/* and take the logical address in the segment map (which now has data) and copy it
	 * to the given logical address struct */
	copyLogicalAddress(log_addr, GET_RECLAIMED_ADDRESS_PTR());
//	PRINT_MSG_AND_NUM("\nwriteBlock() - log_addr->page_offset=",log_addr->page_offset);
//	PRINT_MSG_AND_NUM("\nwriteBlock() - log_addr->segment_num=",log_addr->segment_num);
}

/**
 * @brief
 * auxiliary function to commit.
 * set flags of a page allocated for part of a checkpoint to indicate in which
 * part of the checkpoint it is located
 * NOTICE - if the checkpoint is in size of less than one page it is marked with CP_LOCATION_FIRST
 * and no CP_LOCATION_LAST is programmed afterwords
 * @param flags pointer to the page flags
 * @param cp_page_idx index of the page in the checkpoint pages
 */
void set_cp_page_flag(spare_area_flags *flags, uint32_t cp_page_idx, uint32_t cp_page_idx_max){
	if(cp_page_idx==0){
		SET_CHECKPOINT_FLAG(flags, CP_LOCATION_FIRST);
	}
	else if(cp_page_idx == cp_page_idx_max){
		SET_CHECKPOINT_FLAG(flags, CP_LOCATION_LAST);
	}
	else{
		SET_CHECKPOINT_FLAG(flags, CP_LOCATION_MIDDLE);
	}

//	PRINT_MSG_AND_NUM("\nset_cp_page_flag() - flags are set to ", flags->cp_flag);
}



/**
 * @brief
 * auxiliary, delete current segment until its beginning
 * @return 0 if successful. if an erase fails return 1
 */
error_t truncateCurrentSegment(){
//	PRINT_MSG_AND_NUM("\ntruncateCurrentSegment() - truncating segmnet ", GET_RECLAIMED_SEGMENT());
	do{
//		PRINT_MSG_AND_NUM("\ntruncateCurrentSegment() - in offset ", GET_RECLAIMED_OFFSET());
//		PRINT("\ntruncateCurrentSegment() - logicalAddressToPhysical");
		if(nandErase(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()))){
			SET_RECLAIMED_SEGMENT(SEQ_NO_SEGMENT_NUM);
			SET_RECLAIMED_OFFSET(SEQ_NO_PAGE_OFFSET);
			return 1;
		}

		if(GET_RECLAIMED_OFFSET() >=NAND_PAGES_PER_ERASE_UNIT){
			SET_RECLAIMED_OFFSET(GET_RECLAIMED_OFFSET() -NAND_PAGES_PER_ERASE_UNIT);
		}
		else{
			return 0;
		}
	}while(1);
}

/**
 * @brief
 * auxiliary to performWearLeveling(). copy back one page. mark it as copy back in flags if necessary
 * @param to address to copy to
 * @param from address to copy from
 * @param isSlotReserve is he slot we are copying a reserve slot
 * @return 0 if successful. if a program error occured return 1
 */
error_t copyPage(uint32_t to, uint32_t from, bool_t isToReserve, uint32_t originalToAddress, bool_t isSlotReserve){
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
//	PRINT_MSG_AND_NUM("\ncopyPage() - copying from slot ",from / SEQ_PAGES_PER_SLOT);
//	PRINT_MSG_AND_NUM(" page ",from);
	if(nandReadPageTotal(sequencing_buffer, from)){
		return 1;
	}

//	PRINT_MSG_AND_NUM("\ncopying page, whose first bye is ",sequencing_buffer[0]);
	/* if we are copying an original regular eu to a copy reserve eu, mark it in flags*/
	if(isToReserve){
		SET_SLOT_EU_OFFSET(seq_flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(originalToAddress));
	}
	/* if we are copying an original reserve eu to a copy reserve eu -
	 * we are copying an original regular eu to a copy regular eu, mark it in flags*/
	else if(!isSlotReserve){
		SET_SLOT_EU_OFFSET(seq_flags, SEQ_NO_SLOT_AND_EU_OFFSET);
	}
//	L("call nandProgramTotalPage() on phy_addr %d", to);
	SEQ_VERIFY(!nandProgramTotalPage(sequencing_buffer, to));

	return 0;
}

/**
 * @brief
 * auxiliary. copy back one page. mark it as copy back in flags if necessary.
 * if we copy back a reserve EU (org_from != SEQ_PHY_ADDRESS_EMPTY), then we
 * must maintain slot_eu_flag of the original physical address and not change it,
 * even after copy back
 *
 * @param to address to copy to
 * @param from address to copy from
 * @param withCopyBackFlags
 * @param org_from indicate whther from is a reserve EU. if not org_from is SEQ_PHY_ADDRESS_EMPTY
 * @withCopyBackFlags should we mark copy back flag
 */
error_t copyBackPage(uint32_t to, uint32_t from, bool_t withCopyBackFlags, uint32_t org_from){
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
//	PRINT_MSG_AND_NUM("\ncopyBackPage() - copying from slot ",from / SEQ_PAGES_PER_SLOT);
//	PRINT_MSG_AND_NUM(" in offset ",from % SEQ_PAGES_PER_SLOT);
//	PRINT_MSG_AND_NUM(" to slot ",to / SEQ_PAGES_PER_SLOT);
//	PRINT_MSG_AND_NUM(" in offset ",to % SEQ_PAGES_PER_SLOT);
//	PRINT_MSG_AND_NUM(" from= ",from);
//	PRINT_MSG_AND_NUM(" to= ",to);

	if(nandReadPageTotal(sequencing_buffer, from)){
//		PRINT("\ncopyBackPage() - read error")
		return 1;
	}

	if(!IS_PAGE_USED(seq_flags)){
//		PRINT("\ncopyBackPage() - page is not used. don't bother with copyback");
		return 0;
	}

//	PRINT_MSG_AND_NUM("\ncopying page, whose first bye is ",sequencing_buffer[0]);
	if(withCopyBackFlags){
		SET_COPY_BACK_FLAG(seq_flags, COPY_BACK_FLAG_TRUE);

		/* if we are performing a regular EU copy back, we can mark slot_eu_flag with from*/
		if(IS_PHY_ADDR_EMPTY(org_from)){
//			PRINT("\nregular copy back");
//			PRINT_MSG_AND_HEX("\nfrom slot=", from / SEQ_PAGES_PER_SLOT);
//			PRINT_MSG_AND_HEX("\nfrom offset=", from % SEQ_PAGES_PER_SLOT);
//			PRINT_MSG_AND_HEX("\nset slot eu offset to ", CALCULATE_SLOT_ID_AND_EU_OFFSET(from))

//			PRINT_MSG_AND_HEX("\nslot eu offset=", ((((from) & SLOT_OFFSET_MASK) & NAND_EU_MASK) >> POWER_NAND_PAGES_PER_ERASE_UNIT));
//			PRINT_MSG_AND_HEX("\nslot=", (((from) >> POWER_SEQ_PAGES_PER_SLOT) << NAND_ADDRESS_EU_BITS_COUNT));
//			PRINT_MSG_AND_HEX("\nNAND_ADDRESS_EU_BITS_COUNT=", NAND_ADDRESS_EU_BITS_COUNT);

			SET_SLOT_EU_OFFSET(seq_flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(from));
//			PRINT_MSG_AND_NUM("\nextracted eu=", EXTRACT_EU(GET_SLOT_EU_OFFSET(seq_flags)));
//			PRINT_MSG_AND_NUM("\nextracted slot=", EXTRACT_SLOT(GET_SLOT_EU_OFFSET(seq_flags)));

		}
		else{
//			PRINT("\nreserve copy back");
			SET_SLOT_EU_OFFSET(seq_flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(org_from));
		}
	}
	else{
		SET_COPY_BACK_FLAG(seq_flags, COPY_BACK_FLAG_FALSE);

		/* if we are performing a regular EU copy back, we can erase slot_eu_flag.*/
		if(IS_PHY_ADDR_EMPTY(org_from))
			SET_SLOT_EU_OFFSET(seq_flags, SEQ_SLOT_EU_ID_EMPTY);
	}

	/* mark page as copy back for the given address*/
//	L("call nandProgramTotalPage() on phy_addr %d", to);
	SEQ_VERIFY(!nandProgramTotalPage(sequencing_buffer, to));

	return 0;
}

/**
 * @brief
 * auxiliary, does the actual copy back
 *
 * Assumptions
 * 1. cb_eu_phy_addr is empty and valid
 * 2. org_eu_phy_addr is valid
 *
 * @param cb_eu_phy_addr physical address of copy back eu
 * @param org_eu_phy_addr physical address of eu we want to copy back + maximum offset until which we will copy (<max_offset, not itself)
 * @param isOrgErased indicator if the original eu is erased. if so no need to copy to copy back eu
 * @param real_org_eu original of org_eu_phy_addr (if it is reserve). otherwise = SEQ_PHY_ADDRESS_EMPTY
 * @return 0 if successful, 1 if a program error occured
 */
error_t copyBackSimple(uint32_t cb_eu_phy_addr, uint32_t org_eu_phy_addr, bool_t isOrgErased, uint32_t real_org_eu){
	uint32_t i;
//	PRINT_MSG_AND_NUM("\ncopyBackSimple() - cb_eu_phy_addr=",cb_eu_phy_addr);
//	PRINT_MSG_AND_NUM(" org_eu_phy_addr=",org_eu_phy_addr);
//	PRINT_MSG_AND_NUM(" max_offset=",max_offset);
//	PRINT_MSG_AND_NUM(" isOrgErased=",isOrgErased);
//	PRINT_MSG_AND_NUM(" real_org_eu=",real_org_eu);

	/* ideally passed as an arguement. added to org_eu_phy_addr to avoid the need for 5 arguements (not efficient on ARM)*/
	uint32_t max_offset = CALC_OFFSET_IN_EU(org_eu_phy_addr);
	org_eu_phy_addr = CALC_EU_START(org_eu_phy_addr);

	if(!isOrgErased){
		for(i=0; i < max_offset ; i++){
//			PRINT_MSG_AND_NUM("\ncopyBackSimple() - copying to cb EU from address ", org_eu_phy_addr+i);
			SEQ_VERIFY(!copyBackPage(cb_eu_phy_addr+i, org_eu_phy_addr+i, 1, real_org_eu));
		}
	}

//	PRINT("\nabout to erase original EU");
	/* erase original EU*/
	if(nandErase(org_eu_phy_addr))
		return 1;

//	PRINT("\nabout to copy to original EU");
	/* and copy back*/
	for(i=0; i < max_offset ; i++){
		SEQ_VERIFY(!copyBackPage(org_eu_phy_addr+i, cb_eu_phy_addr+i, 0, real_org_eu));
	}

	nandErase(cb_eu_phy_addr);

	return 0;
}

///**
// * @brief
// * auxiliary, does the actual copy back of a reserve EU
// *
// * Assumptions
// * 1. cb_eu_phy_addr is empty and valid
// * 2. org_eu_phy_addr is not valid
// * 3. reserve_phy_addr is valid
// * @param cb_eu_phy_addr physical address of copy back eu
// * @param org_eu_phy_addr physical address of the eu we want to do a copy back to
// * @param reserve_phy_addr a reserve EU for org_eu_phy_addr
// * @param max_offset maximum offset until which we will copy (not itself)
// * @param isOrgErased indicator if the original eu is erased. if so no need to copy to copy back eu
// * @return 0 if successful, 1 if a program error occured
// */
//error_t copyBackSpecial(uint32_t cb_eu_phy_addr, uint32_t org_eu_phy_addr, uint32_t reserve_phy_addr, uint32_t max_offset, bool_t isReserveErased){
//	uint32_t i;
//	PRINT_MSG_AND_NUM("\ncopyBackSpecial() - max_offset=",max_offset);
//
//	if(!isReserveErased){
//		for(i=0; i < max_offset ; i++){
////			PRINT_MSG_AND_NUM("\ncopyBackSimple() - copying to reserve EU from address ", org_eu_phy_addr+i);
//			if(copyBackPage(cb_eu_phy_addr+i, reserve_phy_addr+i, 1, org_eu_phy_addr))
//				return 1;
//		}
//	}
////	PRINT("\nabout to erase original EU");
//	/* erase original EU*/
//	if(nandErase(reserve_phy_addr))
//		return 1;
//
//	/* and copy back*/
//	for(i=0; i < max_offset ; i++){
//		if(copyBackPage(reserve_phy_addr+i, cb_eu_phy_addr+i, 0, org_eu_phy_addr))
//			return 1;
//	}
//
//	nandErase(cb_eu_phy_addr);
//
//	return 0;
//}

/**
 * @brief
 * - copy the EU log_addr is part of to a reserve EU
 * - erase the original
 * - copy back only until the offset in log_addr
 * @param org_log_addr logical address whose matching physical address EU should be copyback-ed
 * @return 0 if the copyback was successful, 1 if an error occured when copying
 */
error_t copyBackEU(logical_addr_t org_log_addr){
	uint32_t real_org_eu, last_addr, max_offset, cb_eu_start, org_phy_addr;
	/* calculate original physical address*/
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	/* calc max ofset until which to copy */
	max_offset = CALC_OFFSET_IN_EU(GET_LOGICAL_OFFSET(org_log_addr));
	copyLogicalAddress(log_addr, org_log_addr);

	/* set cb eu start address in logical address, in case it is in reclaimed slot*/
	SET_LOGICAL_OFFSET(log_addr, CALC_EU_START(GET_LOGICAL_OFFSET(log_addr)));
//	PRINT_MSG_AND_NUM("\ncopyBackEU() - rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" and now logical seg=",GET_LOGICAL_SEGMENT(log_addr));
//	PRINT_MSG_AND_NUM(" logical offset=",GET_LOGICAL_OFFSET(log_addr));
//	PRINT_MSG_AND_NUM(" max_offset=",max_offset);
	/* NOTICE - we set logical address to physical BEFORE translating to physical.
	 * this is to handle case that reclaimed address is in the middle of the EU.
	 * if we don't do this, we will try to copy back to old generation EU*/
//	PRINT("\ncopyBackEU() - logicalAddressToPhysical");
	org_phy_addr    = logicalAddressToPhysical(log_addr);

	cb_eu_start = GET_COPYBACK_EU();
	assert(!IS_ADDRESS_ERROR(cb_eu_start));
//	PRINT_MSG_AND_NUM("\ncopyBackEU() - org_phy_addr=",org_phy_addr);
//	PRINT_MSG_AND_NUM(" cb_eu_start=",cb_eu_start);
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());

	if(isAddressReserve(org_phy_addr)){
		SEQ_VERIFY(!nandReadPageFlags(flags, org_phy_addr));
//		PRINT_MSG_AND_NUM("\ncopyBackEU() - GET_SLOT_EU_OFFSET(seq_flags)= ",GET_SLOT_EU_OFFSET(flags));
//		PRINT_MSG_AND_NUM(" for page ",org_phy_addr);
		real_org_eu = CALC_EU_START(EXTRACT_ADDR_FROM_SLOT_ID_AND_EU_OFFSET(GET_SLOT_EU_OFFSET(flags)));
//		PRINT_MSG_AND_NUM("\ncopyBackEU() - org_phy_addr is reserve. real_org_eu=",real_org_eu);
	}
	else{
		real_org_eu = SEQ_PHY_ADDRESS_EMPTY;
//		PRINT("\ncopyBackEU() - org_phy_addr is not reserve. real_org_eu empty");
	}

//	PRINT_MSG_AND_NUM("\ncopyBackEU() - about to copy. save until max_offset ", max_offset);

	last_addr  = org_phy_addr;
//	PRINT_MSG_AND_NUM("\ncopyBackEU() - org_phy_addr=", CALC_EU_START(org_phy_addr));
//	PRINT_MSG_AND_NUM("\ncopyBackEU() - cb_eu_start=", cb_eu_start);

	/* if we copy back a regular address, perform simple copy back.*/
//	return copyBackSimple(cb_eu_start, CALC_EU_START(org_phy_addr), max_offset+1, 0, real_org_eu);
	return copyBackSimple(cb_eu_start, CALC_EU_START(org_phy_addr)+max_offset+1, 0, real_org_eu);

	/* we perform a copy back for a reserve EU. We cannot simply */
}

/**
 * @brief
 * auxiliary. iterate while we still haven't reached segment start
 * look for the end of a checkpoint
 *
 * Assumptions:
 * 1. log_addr points at a valid page
 * 2. reclaimed offset > log_addr offset
 *
 * NOTICE - we don't look for CP_LOCATION_FIRST, since finding it doesn't mean we have
 * CP_LOCATION_LAST after it somewhere
 *
 * @param flags spare area flags buffer
 * @param log_addr logical address used to iterate the segment
 * @param nBytes file system layer data structures (for checkpoint) size
 * @return 0 if successful, 1 if none found
 */
error_t findCheckpointInSegment(spare_area_flags *flags, logical_addr_t log_addr, bool_t cp_more_than_one_page){
	uint32_t phy_addr;

//	PRINT_MSG_AND_NUM("\nfindCheckpointInSegment() - cp_more_than_one_page=",cp_more_than_one_page);
//	PRINT_MSG_AND_NUM(" log_addr offset=",GET_LOGICAL_OFFSET(log_addr));
//	PRINT_MSG_AND_NUM(" log_addr segmnet=",GET_LOGICAL_SEGMENT(log_addr));
//	PRINT_MSG_AND_NUM(" log_addr segmnet's slot=",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,GET_LOGICAL_SEGMENT(log_addr)));
//	PRINT_MSG_AND_NUM(" log_addr physical=",logicalAddressToPhysical(log_addr));

	while(GET_LOGICAL_OFFSET(log_addr) >0){
//		PRINT_MSG_AND_NUM("\nfindCheckpointInSegment() - are we in reserve eu?",GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr)==IS_EU_RESERVE_TRUE);
//		PRINT_MSG_AND_NUM("\nfindCheckpointInSegment() - which is reserve eu?",GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr));
//		PRINT("\nfindCheckpointInSegment() - logicalAddressToPhysical");
//		PRINT_MSG_AND_HEX("\nlog addr hex=", *CAST_TO_UINT32(log_addr));
		phy_addr = logicalAddressToPhysical(log_addr);
		assert(!IS_PHY_ADDR_EMPTY(phy_addr));

		nandReadPageFlags(flags, phy_addr);
//		PRINT_MSG_AND_NUM("\nfindCheckpointInSegment() - read from ", phy_addr);
//		PRINT_MSG_AND_NUM("\nfindCheckpointInSegment() - page_offset=", GET_LOGICAL_OFFSET(log_addr));
//		PRINT_MSG_AND_NUM(" segment ", GET_LOGICAL_SEGMENT(log_addr));
//		PRINT_MSG_AND_NUM(" cp flag=", GET_CHECKPOINT_FLAG(flags));
//		PRINT_MSG_AND_NUM(" cp_more_than_one_page=", cp_more_than_one_page);
//		PRINT_MSG_AND_NUM(" page=", logicalAddressToPhysical(log_addr));
		/* a. cp size > one page - check if we got to last page
		 * b. else - check if we got to first page*/
		if((cp_more_than_one_page  && GET_CHECKPOINT_FLAG(flags) == CP_LOCATION_LAST) ||
		   (!cp_more_than_one_page && GET_CHECKPOINT_FLAG(flags) == CP_LOCATION_FIRST)){
		   	if(isAddressReserve(phy_addr)){
		   		/* if checkpoint is in reserve address, mark that in segment map.
		   		 * when we return to findCheckpointAndTruncate, after truncation this will be important*/
		   		SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr, IS_EU_RESERVE_TRUE);
		   		SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, CALC_EU_START(phy_addr));
		   	}
//		   	PRINT_MSG_AND_NUM("\nfindCheckpointInSegment() - foudn cp last page in offset ", GET_LOGICAL_OFFSET(log_addr));
			return 0;
		}

		/* try previous page*/
		SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)- 1);

//		if(!cp_more_than_one_page){
//			PRINT("\nread prev page flags");
//			SEQ_VERIFY(nandReadPageFlags(flags, logicalAddressToPhysical(log_addr)));
//		}
	}
//	PRINT("\nno cp found");
	return 1;
}

/**
 * @brief
 * auxiliary, iterate backwards in current segment until you find the first page marked
 * as checkpoint start.
 * Assumptions:
 * 1. we already found the last part of a checkpoint in the segment, therefore there MUST be a first part
 */
error_t findCheckpointStart(spare_area_flags *flags, logical_addr_t log_addr){
	do{
//		PRINT("\nfindCheckpointStart() - logicalAddressToPhysical");
		nandReadPageFlags(flags, logicalAddressToPhysical(log_addr));
//		PRINT_MSG_AND_NUM("\nfindCheckpointStart() - log_addr->page_offset=",log_addr->page_offset);

//		PRINT_MSG_AND_NUM("\nfindCheckpointStart() - flags->cp_flag=",GET_CHECKPOINT_FLAG(flags));
		if(GET_CHECKPOINT_FLAG(flags) == CP_LOCATION_FIRST)
			break;

		/* try previous page*/
		SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)-1);
	}while(GET_LOGICAL_OFFSET(log_addr) > 0);
//	PRINT_MSG_AND_NUM("\nfindCheckpointStart() - log_addr->page_offset ",log_addr->page_offset);
//	PRINT_MSG_AND_NUM("\nfindCheckpointStart() - log_addr->segment_num ",log_addr->segment_num);
//	PRINT_MSG_AND_NUM("\nfindCheckpointStart() - matching slot num is ",utils_seg_map_ptr->seg_to_slot_map[log_addr->segment_num]);

	return 0;
}

/**
 * @brief
 * find start of checkpoint and read it to memory structures
 *
 * Assumptions:
 * 1. log_addr points at last valid page
 * 2. reclaimed address offset > log_addr offset (required for reclamation state)
 * @param data file system data pointer
 * @param nBytes file system data size
 * @param flags spare area buffer
 * @param temporary logical address index
 * @return 0 if successful, 1 if a read error has occured
 */
error_t readCheckpointToMemory(void *data, uint32_t nBytes, spare_area_flags *flags, logical_addr_t log_addr){
	uint32_t i,j;
	INIT_FLAGS_POINTER_TO_BUFFER(buf_flags, sequencing_buffer);

//	PRINT("\n\nreadCheckpointToMemory() - ");
//	PRINT_MSG_AND_NUM(" read from addr", logicalAddressToPhysical(log_addr));
//	PRINT_MSG_AND_NUM(" nBytes= ",nBytes);
//	PRINT_MSG_AND_NUM(" segment=",GET_LOGICAL_SEGMENT(log_addr));
//	PRINT_MSG_AND_NUM(" offset=",GET_LOGICAL_OFFSET(log_addr));
//	PRINT_MSG_AND_NUM(" slot=",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, GET_LOGICAL_SEGMENT(log_addr)));

	/* if checkpoint is more then one page we should find it's start*/
	if(IS_CHECKPOINT_MORE_THAN_ONE_PAGE(nBytes)){
		findCheckpointStart(flags, log_addr);
	}

//	PRINT_MSG_AND_NUM("\nreadCheckpointToMemory() - cp start is in page ",log_addr->page_offset);
//	assert(0);
	/* init fs_data before reading to it*/
	fsMemset(data, 0xff, nBytes);

	/* read checkpoint*/
	for(i=0; i< SEQ_CHECKPOINT_SIZE + nBytes;){
		/* read, and prepare for next read*/
//		PRINT("\nreadCheckpointToMemory() - read, and prepare for next read");
//		PRINT_MSG_AND_NUM("\nreadCheckpointToMemory() - i= ",i);
//		PRINT_MSG_AND_NUM("\nreadCheckpointToMemory() - log_addr->page_offset ",log_addr->page_offset);
//		PRINT_MSG_AND_NUM("\nreadCheckpointToMemory() - log_addr->segment_num ",log_addr->segment_num);
//		PRINT("\nreadCheckpointToMemory() - logicalAddressToPhysical");
		SEQ_VERIFY(!nandReadPageTotal(sequencing_buffer, logicalAddressToPhysical(log_addr)));
//		PRINT_MSG_AND_NUM("\nreadCheckpointToMemory() - from page ",logicalAddressToPhysical(log_addr));

//		PRINT_MSG_AND_NUM(" segment ",log_addr->segment_num);
//		PRINT_MSG_AND_NUM(" page offset ",log_addr->page_offset);
//		PRINT_MSG_AND_NUM(" i=",i);
//		PRINT_MSG_AND_NUM("\nreadCheckpointToMemory() - is not pasrt of cp (",buf_flags->cp_flag);
		SET_LOGICAL_OFFSET(log_addr, GET_LOGICAL_OFFSET(log_addr)+1);

		if(GET_CHECKPOINT_FLAG(buf_flags) == CP_NOT_PART_OF_CP){
//			PRINT("\nreadCheckpointToMemory() - GET_CHECKPOINT_FLAG(buf_flags) == CP_NOT_PART_OF_CP");
			continue;
		}

//		if(GET_LOGICAL_OFFSET(log_addr)>4)
//			assert(0);
//		{
//			int32_t i;
//			for(i=0; i< NAND_PAGE_SIZE;i++){
//				if(sequencing_buffer[i] == 0xff)
//					continue;
//
//				PRINT_MSG_AND_NUM("\nbyte ",i);
//				PRINT_MSG_AND_NUM(". ",sequencing_buffer[i]);
//			}
//		}
//		PRINT("\nreadCheckpointToMemory() - read another page");
		j=0;
		do{
			/* if we are reading file system data to buffer */
			if(i < nBytes){
//				PRINT_MSG_AND_NUM("\nreadCheckpointToMemory() - file system data in byte",j);
				CAST_TO_UINT8(data)[i] = sequencing_buffer[j];//
			}
			else{
//				PRINT_MSG_AND_NUM("\nreadCheckpointToMemory() - obs map data in byte",j);
//				{
//					uint8_t temp_buf[NAND_TOTAL_SIZE], i;
//					init_buf(temp_buf);
//					nandReadPageTotal(temp_buf, 577);
////					PRINT_MSG_AND_HEX("\nread page 577. temp_buf[100]=",temp_buf[100]);
//					assert(temp_buf[100]==0x22);
//				}
//				if(j==100){
//					PRINT_MSG_AND_HEX("\nsequencing_buffer[",j);
//					PRINT_MSG_AND_HEX("]=",sequencing_buffer[j]);
//				}
				CAST_TO_UINT8(obs_counters_map_ptr)[i - nBytes] = sequencing_buffer[j];
//				PRINT_MSG_AND_NUM("\nCAST_TO_UINT8(obs_counters_map_ptr)[",i);
//				PRINT_MSG_AND_HEX("]=",CAST_TO_UINT8(obs_counters_map_ptr)[i - nBytes]);
			}

//			PRINT_MSG_AND_NUM("\n j= ",j);
			j++;
			i++;
		}while(j < NAND_PAGE_SIZE && i<SEQ_CHECKPOINT_SIZE + nBytes );
	}

//	PRINT("\nreadCheckpointToMemory() - finished reading bytes");
	/* sanity check - we should be in the last part of the checkpoint */
	assert(GET_CHECKPOINT_FLAG(buf_flags) == CP_LOCATION_LAST || !IS_CHECKPOINT_MORE_THAN_ONE_PAGE(nBytes));
//	assert(0);
	return 0;
}

/**
 * @brief
 * auxiliary, check if all segments are amrked as real full
 */
bool_t is_flash_full(){
	uint32_t i;

	for(i=0; i< SEQ_SEGMENTS_COUNT; i++){
		if(!IS_SEGMENT_FULL(i))
			return 0;
	}

	return 1;
}

/**  @brief
 * Reserve EU search:
 * EU is bad. search for the page in a reserve EU
 * iterate all reserve segments until we find the reserve EU
 * (reserve segments are always the last)
 * @param flags spare area flags pointer
 * @param phy_addr the physical address we're for a reserve replcament to
 *
 * always should be one - first EU in first Reserve segemnt
 * @return reserve physical address if successful, SEQ_PHY_ADDRESS_EMPTY otherwise
 */
uint32_t allocReserveEU(spare_area_flags *flags, uint32_t phy_addr){
	uint32_t seg_idx;
	uint32_t reserve_slot_address, reserve_slot_eu_offset;
	uint32_t cb_eu_addr;
	uint32_t eu_offset, eu_page_offset;
//	FENT();
	cb_eu_addr = GET_COPYBACK_EU();
	assert(!IS_PHY_ADDR_EMPTY(cb_eu_addr));

	eu_page_offset = CALC_OFFSET_IN_EU(phy_addr);
	eu_offset      = CALC_EU_OFFSET(phy_addr);
//	PRINT_MSG_AND_NUM("\nallocReserveEU() - phy_addr = ",phy_addr);
//	PRINT_MSG_AND_NUM(" cb_eu_addr = ",cb_eu_addr);
//	PRINT_MSG_AND_NUM("\nallocReserveEU() - eu_page_offset = ",eu_page_offset);
	for(seg_idx= SEQ_FIRST_RESERVE_SEGMENT; seg_idx<SEQ_N_SLOTS_COUNT;seg_idx++){
		/* iterate all EUs in segment until we find reserve EU
		 * start from EU in parallel offset, and continue cycilcally */
		reserve_slot_eu_offset = eu_offset;

		reserve_slot_address   = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg_idx],reserve_slot_eu_offset,0);
//		L("iterating reserve segment %d whose slot is %d ", seg_idx, seg_map_ptr->seg_to_slot_map[seg_idx]);
//		PRINT_MSG_AND_NUM(" slot=", seg_map_ptr->seg_to_slot_map[seg_idx]);
		do{
//			L("iterating reserve eu %d, eu page offset", reserve_slot_eu_offset, eu_page_offset);
			/* if the eu is valid, and it is not the first valid EU in the first reserve segment (reserved for copyback)
			 * check if it is the one we want */

			if(seq_check_eu_status(reserve_slot_address) && cb_eu_addr != reserve_slot_address){
				nandReadPageFlags(flags, reserve_slot_address);//
				// verify the reserve eu isn't bad in itself
				if(GET_SLOT_EU_OFFSET(flags) == SEQ_NO_SLOT_AND_EU_OFFSET){
//					L("finished. reserve eu %d ",reserve_slot_address+eu_page_offset);
				   /* return the address (eu+offset in eu) */
				   return reserve_slot_address+eu_page_offset;
				}
			}

			/* not verified. advance to next EU address */
			reserve_slot_eu_offset +=1;
			reserve_slot_address   += NAND_PAGES_PER_ERASE_UNIT;

			/* if overflowed the slot pages, return to start (cyclic)*/
			if(reserve_slot_eu_offset == SEQ_EUS_PER_SLOT){
				reserve_slot_address   = reserve_slot_address - SEQ_PAGES_PER_SLOT;
				reserve_slot_eu_offset = 0;
			}
		}while (reserve_slot_eu_offset != eu_offset);
	}

//	L("ERROR couldn't find reserve");
	/* we have no reserve EU's!!*/
//	assert(0);
	return SEQ_PHY_ADDRESS_EMPTY;
}

/**
 * @brief
 * auxiliary. check if the current page in the seg map is considered valid for copying during reclamation
 * a page is copied only if it is not obsolete, and not part of a checkpoint
 * @param phy_addr the page to check whther to copy
 * @return 1 if the current paeg should be copied, 0 if not
 */
error_t is_page_to_copy(spare_area_flags *copy_flags, uint32_t phy_addr){
	initFlags(copy_flags);
	/* read page flags*/
	nandReadPageFlags(copy_flags, phy_addr);

//	PRINT_MSG_AND_NUM("\nis_page_to_copy() - GET_OBS_FLAG(copy_flags)=",GET_OBS_FLAG(copy_flags));
//	PRINT_MSG_AND_NUM("\nis_page_to_copy() - GET_CHECKPOINT_FLAG(copy_flags)=",GET_CHECKPOINT_FLAG(copy_flags));
//
//	PRINT_MSG_AND_NUM("\n\nis_page_to_copy() - is page not to copy?",IS_PAGE_OBSOLETE(copy_flags) || GET_CHECKPOINT_FLAG(copy_flags) != CP_NOT_PART_OF_CP);
	/* check conditions*/
	if(IS_PAGE_OBSOLETE(copy_flags) || GET_CHECKPOINT_FLAG(copy_flags) != CP_NOT_PART_OF_CP){
		/* NOTICE - checkpoint pages are not counted as writes */
		if(IS_PAGE_OBSOLETE(copy_flags)){
			DECREMENT_OBS_COUNT();
//			L("decremented obs to %d", GET_OBS_COUNT());
//#ifdef EXACT_COUNTERS
//			DEC_EXACT_COUNTER(GET_RECLAIMED_SEGMENT());
//#endif
		}
		/* overwriting a checkpoint page decrements free page count*/
		else{
			DECREMENT_FREE_COUNT();
//			L("decremented free count to %d", GET_FREE_COUNTER());
		}

		return 0;
	}
	//return check_flags(copy_flags1->obsolete_flag, copy_flags1->cp_flag);
	initFlags(copy_flags);

	return 1;
}

/**
 * @brief
 * auxiliary function for allocAndWriteBlock - copy a page from old generation
 * to new generation. We assume both of them exist, and pointed by the segment map
 *
 * @param phy_addr address to write data to
 * @param org_phy_addr the original physical address which phy_addr replaces. if phy_addr is not reserve, org_phy_addr is 0
 * @return 1 if an error occured during write/read. return 0 if successful
 */
error_t copyValidBlock(uint32_t phy_addr, uint32_t org_phy_addr){
	INIT_FLAGS_POINTER_TO_BUFFER(flags, sequencing_buffer);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

//	PRINT_MSG_AND_NUM("\ncopyValidBlock() - copy to ",phy_addr);
//	PRINT_MSG_AND_NUM(" from ", logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
//	PRINT_MSG_AND_NUM(" org_phy_addr=",org_phy_addr);
//
	/* read the page and change it's copy flag*/
//	PRINT("\ncopyValidBlock() - logicalAddressToPhysical");
	SEQ_VERIFY(!nandReadPage(sequencing_buffer, logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()), 0, NAND_TOTAL_SIZE));

	SET_COPY_FLAG(flags, COPY_FLAG_TRUE);
	/* mark slot id and offset if the original physical address is not empty*/
	if(!IS_PHY_ADDR_EMPTY(org_phy_addr)){
		SET_SLOT_EU_OFFSET(flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(org_phy_addr));
	}
	else{
		SET_SLOT_EU_OFFSET(flags, SEQ_NO_SLOT_AND_EU_OFFSET);
	}

	/* write data*/
	writeBlock(log_addr, sequencing_buffer, phy_addr);
//	{
//		uint8_t buf[NAND_TOTAL_SIZE];
//		page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//		nandReadPageTotal(buf, 64576);
//		if(temp!=GET_HEADER_SEGMENT_ID(page_header_ptr)){
//			PRINT_MSG_AND_NUM("\n	copyValidBlock() - after writeBlock(). page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//			PRINT_MSG_AND_NUM("\ncopyValidBlock() - copy to ",phy_addr);
//			PRINT_MSG_AND_NUM(" from ", logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()));
//		}
//	}
	SEQ_VERIFY(!IS_ADDRESS_EMPTY(log_addr));
	return 0;
}

uint32_t count_valid_eus(uint32_t slot_id){
	uint32_t count = 0, phy_addr = CALC_ADDRESS(slot_id,0,0);

	while(phy_addr< CALC_ADDRESS(slot_id+1,0,0)){
		if(seq_check_eu_status(phy_addr)) count++;

		phy_addr+=NAND_PAGES_PER_ERASE_UNIT;
	}

	return count;
}

/**
 * @brief
 * copy a random slot to the current new slot in the segment map.
 * this is to avoid slots seldomly being reclaimed.
 *
 * Assumptions -
 * 1. we are in reclaamtion state
 * 2. we have enough valid EU's in the empty slot to copy data from old slot
 * @return 0 if successful, 1 if a write error occured
 */
error_t performWearLeveling(){
	uint32_t i, random_slot = seg_map_ptr->new_slot_id, org_seg = 0;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	uint32_t org_to_phy_addr,from_phy_addr, to_phy_addr;
	uint32_t isSlotReserve = 0, isToReserve = 0;
	uint32_t from_page_offset = 0, to_page_offset = 0;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);

	init_logical_address(log_addr);
//	PRINT("\nperformWearLeveling() - starting");
	/* continue searching for a new slot until we decide on one that is not the empty one*/
	while(random_slot == seg_map_ptr->new_slot_id){
#ifdef Debug
#ifndef PROFILING_ARRAYS
		 random_slot = TEMP_RANDOM_SLOT;
#endif
#else
		 random_slot  = get_random_num(SEG_NEEDED_PROB);
#endif
//		printf("\nperformWearLeveling() - random_slot=%d TEMP_RANDOM_SLOT=%d",random_slot, TEMP_RANDOM_SLOT);
		if(random_slot == seg_map_ptr->new_slot_id){
			random_slot = (SEQ_N_SLOTS_COUNT-1) - random_slot;
		}
	}

	/* see if the chosen slot is used for a reserve segment*/
	for(i=0; i<SEQ_N_SLOTS_COUNT;i++){
		if(seg_map_ptr->seg_to_slot_map[i] == random_slot){
			if(i>=SEQ_FIRST_RESERVE_SEGMENT){
				isSlotReserve = 1;
//				PRINT_MSG_AND_NUM("\n\nperformWearLeveling() - we chose a reserve slot to wear level. slot=",random_slot);
			}

			org_seg = i;
//			PRINT_MSG_AND_NUM("\norg_seg = ",org_seg);
			break;
		}
	}

//	PRINT_MSG_AND_NUM("\nperformWearLeveling() slot reserve=",isSlotReserve);

	/* we write to the new slot*/
//	PRINT_MSG_AND_NUM("\nperformWearLeveling() - copying to slot ",seg_map_ptr->new_slot_id);
	to_phy_addr   = CALC_ADDRESS(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr),0,0);

	/* from the chosen random slot*/
	from_phy_addr = CALC_ADDRESS(random_slot,0,0);

	/* if we're wear leveling a reserve slot, then we need to make sure that we have enough
	 * eu's to copy to - we cannot use reserve EU's to reserve EU's*/
	if(isSlotReserve){
		/* if we dont have enough EUs - just give up (we'll get it next time...)*/
		if(count_valid_eus(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr)) < count_valid_eus(random_slot)){
//			PRINT("\nperformWearLeveling() - not enough valid eus");
			/* return "successfuly" although we simply did not perform weare leveling.
			 * Over time this won't matter*/
			return 0;
		}
	}

	/* copy segment, from start to end, so if we reboot and have
	 * 2 identical segments with identical sequencing number, we'd know what to do.
	 * we also copy segment header - we can't change this, since this will
	 * alter the sequence of actual segment writing. */
	while(to_page_offset<SEQ_PAGES_PER_SLOT && from_page_offset < SEQ_PAGES_PER_SLOT){
		org_to_phy_addr = CALC_ADDRESS(seg_map_ptr->new_slot_id,0,to_page_offset);

		/* if we are not at the start of an EU, then the address we copy from
		 * and the address we copy to need no change*/
		if(!(CALC_OFFSET_IN_EU(to_page_offset) == 0)){
			copyPage(to_phy_addr,from_phy_addr,isToReserve, SEQ_PHY_ADDRESS_EMPTY, isSlotReserve);

			from_phy_addr++;
			to_phy_addr++;

			from_page_offset++;
			to_page_offset++;

			continue;
		}

		/* we are at the start of an EU. assume by default we read original EU's, not reserve ones*/
		to_phy_addr   = CALC_ADDRESS(seg_map_ptr->new_slot_id,0,to_page_offset);
		from_phy_addr = CALC_ADDRESS(random_slot,0,from_page_offset);
//		PRINT_MSG_AND_NUM("\nperformWearLeveling() - to_phy_addr= ",to_phy_addr);
//		PRINT_MSG_AND_NUM(", from_phy_addr= ",from_phy_addr);
//		PRINT_MSG_AND_NUM(", to_page_offset= ",to_page_offset);
//		PRINT_MSG_AND_NUM(", from_page_offset= ",from_page_offset);
//		PRINT_MSG_AND_NUM(", isToReserve= ",isToReserve);

		/* must handle possible situations -
		 * a. from_phy_addr was to reserve EU
		 * a.1 the segment we are copying from is a reserve segment, so we dont care about bad EU's in it*/
		if(!seq_check_eu_status(from_phy_addr)){
//			PRINT_MSG_AND_NUM("\n	from_phy_addr invalid ", from_phy_addr);
			/* if we are at the start of a bad EU in a reserve segment
			 * we dont need to look for a reserve to the reserve...
			 * we can simply move on to the next EU*/
//			 PRINT_MSG_AND_NUM("\nisSlotReserve=",isSlotReserve);
			if(isSlotReserve){
				from_page_offset += NAND_PAGES_PER_ERASE_UNIT;
//				PRINT("\nmove to next from EU");
				continue;
			}

			/* the slot is not used for a reserve segment.*/
			SET_LOGICAL_OFFSET(log_addr, to_page_offset);
			/* locate the reserve EU*/
			from_phy_addr = logicalAddressToReservePhysical(flags, from_phy_addr);
//			PRINT_MSG_AND_NUM("\nperformWearLeveling() - from EU is bad. reserve one is in ",from_phy_addr);
			assert(!IS_PHY_ADDR_EMPTY(from_phy_addr));
		}

		/* if we're copying from a reserve slot, don't "copy" unsued EU's -
		 * we'd actually be writing empty data!*/
		if(isSlotReserve){
//			PRINT_MSG_AND_NUM("\nslot is reserve, read from_phy_addr=", from_phy_addr);
			nandReadPageTotal(sequencing_buffer, from_phy_addr);
//			PRINT_MSG_AND_NUM(" is page used=",IS_PAGE_USED(seq_flags));

			/* if the EU is not used simply advance to next EU
			 * and leave next valid EU in to slot empty*/
			if(!IS_PAGE_USED(seq_flags)){
//				PRINT("\nfrom_phy_addr not used");
				from_phy_addr   +=NAND_PAGES_PER_ERASE_UNIT;
				from_page_offset+=NAND_PAGES_PER_ERASE_UNIT;

				while(!seq_check_eu_status(to_phy_addr)){
//					PRINT_MSG_AND_NUM("\ninvalid to_phy_addr=", to_phy_addr);
					to_phy_addr   +=NAND_PAGES_PER_ERASE_UNIT;
					to_page_offset++;
				}

//				PRINT("\ncontinue");
//				PRINT_MSG_AND_NUM("\nperformWearLeveling() - to_phy_addr= ",to_phy_addr);
//				PRINT_MSG_AND_NUM("\nperformWearLeveling() - from_phy_addr= ",from_phy_addr);
				continue;
			}
		}

		/* b. to_phy_addr is bad. need to replace it*/
		if(!seq_check_eu_status(to_phy_addr)){
//			PRINT_MSG_AND_NUM("\n	invalid to_phy_addr ",to_phy_addr);
			isToReserve = 1;

			/* if we are not wear leveling a reserve segment, find a reserve eu*/
			if(!isSlotReserve){
				to_phy_addr = allocReserveEU(flags, to_phy_addr);

				/* if there are no more vacant reserve EU's return error*/
				 if(IS_PHY_ADDR_EMPTY(to_phy_addr)){
				 	/* erase original slot, from end to start*/
					if(truncateSlot(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr), isSlotReserve)){
						return 1;
					}

					return 0;
				 }
			}
			/* we are wear leveling a reserve segment. find next valid eu*/
			else{
				while(!seq_check_eu_status(to_phy_addr)){
					to_phy_addr   +=NAND_PAGES_PER_ERASE_UNIT;
					to_page_offset+=NAND_PAGES_PER_ERASE_UNIT;
				}
				isToReserve = 0;
			}
//			PRINT_MSG_AND_NUM(" replacing it with ",to_phy_addr);
		}
		else{
			isToReserve = 0;
		}

//		PRINT_MSG_AND_NUM("\nperformWearLeveling() - copying page from ",from_phy_addr);
//		PRINT_MSG_AND_NUM(" to ",to_phy_addr);
//		PRINT_MSG_AND_NUM("\ncopyPage from ",from_phy_addr);
//		PRINT_MSG_AND_NUM(" to= ",to_phy_addr);
		copyPage(to_phy_addr,from_phy_addr,isToReserve, org_to_phy_addr, isSlotReserve);
		/* incremeent page offset */
		to_phy_addr++;
		from_phy_addr++;
		from_page_offset++;
		to_page_offset++;
	}

	/* erase original slot, from end to start*/
//	PRINT_MSG_AND_NUM("\nperformWearLeveling() - erase original slot=", random_slot);
	SEQ_VERIFY(!truncateSlot(random_slot, isSlotReserve));

	/* save old new slot id
	 * mark the random slot for writing*/
//	PRINT_MSG_AND_NUM("\nperformWearLeveling() - old new slot id=",seg_map_ptr->new_slot_id);
	i = seg_map_ptr->new_slot_id;
	seg_map_ptr->new_slot_id = random_slot;
	seg_map_ptr->seg_to_slot_map[SEQ_NEW_SEGMENT] = random_slot;
	/* and dont forget to change in the segment map the reference to that slot*/
//	PRINT_MSG_AND_NUM("\nperformWearLeveling() - copy old new_slot to segment=",org_seg);
	SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, i, org_seg);

	return 0;
}

/**
 * @brief
 * erase slot from last EU to start
 * @param slot_id the slot id
 * @return 1 if an error occured during the write, 0 if successful
 *
 */
error_t truncateSlot(uint32_t slot_id, bool_t isSlotReserve){
	uint32_t phy_addr,i, reserve_phy_addr;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);

	/* first address of slot*/
	phy_addr = CALC_ADDRESS(slot_id,0,0);
//	PRINT_MSG_AND_NUM("\ntruncateSlot() - slot_id=",slot_id);
//	PRINT_MSG_AND_NUM("\ntruncateSlot() - phy_addr=",phy_addr);
	/* set i to last address of slot and start traversing backwards */
	for(i = phy_addr + SEQ_MAX_LOGICAL_OFFSET; (i> phy_addr) && (i < NAND_PAGE_COUNT); i -= NAND_PAGES_PER_ERASE_UNIT){
		if(seq_check_eu_status(i)){
//			PRINT_MSG_AND_NUM("\ntruncateSlot() - checked adderss ",i);
			SEQ_VERIFY(!nandErase(i));

			continue;
		}

//		PRINT_MSG_AND_NUM("\ntruncateSlot() - isSlotReserve=",isSlotReserve);
		/* if the slot was used for a reserve segment, we don't care about bad EU's in it -
		 * we don't have reserves for reserves...*/
		if(isSlotReserve){
			continue;
		}

//		PRINT_MSG_AND_NUM("\ntruncateSlot() - EU is bad in address ",i);
		SET_LOGICAL_OFFSET(log_addr, CALC_OFFSET_IN_SLOT(i));
		/* the eu is bad. find the replacement and delete it*/
		reserve_phy_addr = logicalAddressToReservePhysical(flags,i);

		/* if we have a reserve for this address*/
		if(!IS_PHY_ADDR_EMPTY(reserve_phy_addr)){
			SEQ_VERIFY(!nandErase(reserve_phy_addr));
		}
	}

	return 0;
}
//#include <stdio.h>
//FILE *copies_fp;
//
//int open_copies_fp(){
//	copies_fp = fopen("copies.csv", "w");
//
//	if(copies_fp == NULL){
//		return 0;
//	}
//
//	return 1;
//}
/**
 * @brief
 * iterate all levels of obsolete counters starting from obs_counter_level backwards
 * and find a segment with at least that level of obsolete pages, other than last_segment_reclaimed.
 *
 * @return 0 if successful. 0 if no segment was found (all full)
 */
error_t findSegmentToReclaim(){
	uint32_t i, obs_level, min_obs_level, max_obs_level;
#ifndef EXACT_COUNTERS
	min_obs_level = OBS_COUNT_NO_OBSOLETES;
	max_obs_level = OBS_COUNT_LEVEL_7;
#else
	min_obs_level = 0;
	max_obs_level = SEQ_PAGES_PER_SLOT-1;
#endif
//	PRINT("\nfindSegmentToReclaim() - starting");
	for(obs_level = max_obs_level; obs_level > min_obs_level; obs_level--){
		for(i=0; i < SEQ_SEGMENTS_COUNT; i++){
			/* if the segment has a level at least of obs_level
			 * and it is not the one we already reclaimed at this level*/

			if(get_counter_level(i) >= obs_level && i != GET_RECLAIMED_SEGMENT()
#ifndef EXACT_COUNTERS
			   && get_counter_level(i) != OBS_COUNT_FULL
#endif
			   ){
//			   	 PRINT_MSG_AND_NUM("\nfindSegmentToReclaim - get_counter_level(",i);
//			   	 PRINT_MSG_AND_HEX(")=",get_counter_level(i));
//			   	fprintf(copies_fp, "\nnew rec seg obs level, %d, page_copies after reclaiming seg,%d", get_counter_level(i), page_copies);
				SET_RECLAIMED_SEGMENT(i);
				SET_RECLAIMED_OFFSET(0);
//				PRINT_MSG_AND_NUM("\nfindSegmentToReclaim - new GET_RECLAIMED_SEGMENT()=",GET_RECLAIMED_SEGMENT());
//				PRINT("\nfindSegmentToReclaim() - finished");

			   	return 0;
			}
		}
	}

	/* something terrible has happened -?->	assert(0); */
	return 1;
}

/**
 * @brief
 * auxiliary to allocAndWriteBlock(). find the next free page in flash.
 * iterates all segments until one is found
 *
 * Assumptions:
 * 1. rec offset points at an empty page (or overflows seg size)
 *
 * @param cpWritten indicate whther a checkpoint was written in the process of looking for a free page
 * @param cp_write_p checkpoint writer function
 * @param ok_error_code error ok code
 * @return ok_error_code if successful, ERROR_FLASH_FULL if none found, 1 if a write error occured
 */
error_t findNextFreePage(bool_t *cpWritten, checkpoint_writer cp_write_p, uint32_t ok_error_code){
	uint32_t res = 0, nSegmentIterations, target_phy_addr = SEQ_PHY_ADDRESS_EMPTY, phy_addr = 0;
	bool_t moved_seg_flag = 0;
	INIT_FLAGS_STRUCT_AND_PTR(flags_ptr);

//	uint32_t temp;
//	{
//		uint8_t buf[NAND_TOTAL_SIZE];
//		page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//		nandReadPageTotal(buf, 64576);
////		PRINT_MSG_AND_NUM("\n	findNextFreePage() - starting. page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//		temp = GET_HEADER_SEGMENT_ID(page_header_ptr);
//	}
//	PRINT_MSG_AND_NUM("\nfindNextFreePage() - starting. rec slot ",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());

//	if(IS_STATE_RECLAMATION())
//		assert(0);
	for(nSegmentIterations=SEQ_SEGMENTS_COUNT; nSegmentIterations >0 ; nSegmentIterations--){
//		PRINT_MSG_AND_NUM("\nfindNextFreePage() - nSegmentIterations=",nSegmentIterations);
		/* first check if the page that was written was the last in a segment
		 * if so - change reclaimed segments */
//		L("is offset overflow? %d ",IS_SEG_MAP_PAGE_OFFSET_OVERFLOWING());
		if(IS_SEG_MAP_PAGE_OFFSET_OVERFLOWING()){
//			L("seg map overflowing. call moveToNextSegment()");
			res = moveToNextSegment(flags_ptr, cpWritten, cp_write_p);
//			{
//				uint8_t buf[NAND_TOTAL_SIZE];
//				page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//				nandReadPageTotal(buf, 64576);
//				if(GET_HEADER_SEGMENT_ID(page_header_ptr)!= temp){
//					PRINT_MSG_AND_NUM("\n	findNextFreePage() - after moveToNextSegment(). page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//				}
//			}
//			PRINT_MSG_AND_NUM("\nfindNextFreePage() - rec slot ",GET_RECLAIMED_SEGMENT_SLOT());
//			PRINT_MSG_AND_NUM(" re segment ",GET_RECLAIMED_SEGMENT());
//			PRINT_MSG_AND_NUM(" rec offset ",GET_RECLAIMED_OFFSET());
			if(res){
//				PRINT("\nfindNextFreePage() - error finding next segment");
				return res;
			}

			moved_seg_flag = 1;
//			PRINT("\nfindNextFreePage() - we've moved to the next segment");
		}

//		PRINT_MSG_AND_NUM("\nfindNextFreePage() - rec slot ",GET_RECLAIMED_SEGMENT_SLOT());
//		PRINT_MSG_AND_NUM(" re segment ",GET_RECLAIMED_SEGMENT());
//		PRINT_MSG_AND_NUM(" rec offset ",GET_RECLAIMED_OFFSET());
		/* repeat until end of segment*/
		while(!IS_SEG_MAP_PAGE_OFFSET_OVERFLOWING()){
//			PRINT_MSG_AND_NUM("\nfindNextFreePage() - target_phy_addr=",target_phy_addr);
			if(IS_LAST_RECLAIMED_PAGE_START_OF_EU()){
//				PRINT("\nfindNextFreePage() - we are at an EU start");
//				PRINT_MSG_AND_NUM("\nallocAndWriteBlock() - we are at target eu ",target_phy_addr);
				/* by default we expect the new EU to be valid */
				SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr, IS_EU_RESERVE_FALSE);
				SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, SEQ_PHY_ADDRESS_EMPTY);

				/* check if we stepped into a new EU in the slot we write to*/
				target_phy_addr = calc_target_phy_addr();

				/* check EU status*/
//				PRINT_MSG_AND_NUM("\nfindNextFreePage() - check eu status in target_phy_addr address=",target_phy_addr);
				if(!seq_check_eu_status(target_phy_addr)){
//					L("moved to next EU (addr %d) which is bad ",target_phy_addr);
					/* try to find a reserve EU*/
					target_phy_addr = allocReserveEU(flags_ptr, target_phy_addr);
//					{
//						uint8_t buf[NAND_TOTAL_SIZE];
//						page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//						nandReadPageTotal(buf, 64576);
//						if(GET_HEADER_SEGMENT_ID(page_header_ptr)!= temp){
//							PRINT_MSG_AND_NUM("\n	findNextFreePage() - after allocReserveEU(). page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//						}
//					}
					SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr, IS_EU_RESERVE_TRUE);
					VALIDATE_RESERVE_ADDRESS(target_phy_addr);
//					PRINT_MSG_AND_NUM("\nfindNextFreePage() - eu is bad. allocated instaed ",target_phy_addr);
					/* found a reserve EU. start writing to it*/
					SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, target_phy_addr);
				}
			}
			/* if we are in the middle of an EU we can simply
			 * write to the expected physical address */
			else{
				target_phy_addr = calc_target_phy_addr();
			}

//			PRINT_MSG_AND_NUM("\nfindNextFreePage() - is state reclaim? ",IS_STATE_RECLAMATION());
			/* check if we are in reclamation. if not - we have an empty page
			 * in reclaimed logical address and we can return successfuly */
			if(!IS_STATE_RECLAMATION()){
//				PRINT_MSG_AND_NUM("\nfindNextFreePage() - state not reclamation. return ok_error_code=",ok_error_code);
//				PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
				return ok_error_code;
			}

			/* we only need to calculate this if we are in reclaamtion.
			 * otherwise phy_addr IS target_phy_addr */
//			PRINT("\nfindNextFreePage() - in reclamation. about ot calc phy_addr");
//			PRINT_MSG_AND_NUM("\nfindNextFreePage() - rec slot ",GET_RECLAIMED_SEGMENT_SLOT());
//			PRINT_MSG_AND_NUM(" re segment ",GET_RECLAIMED_SEGMENT());
//			PRINT_MSG_AND_NUM(" rec offset ",GET_RECLAIMED_OFFSET());
//			PRINT("\nfindNextFreePage() - logicalAddressToPhysical");
			phy_addr     = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
//			PRINT_MSG_AND_NUM("\nfindNextFreePage() offset- ",GET_RECLAIMED_OFFSET());
//			PRINT_MSG_AND_NUM("\nfindNextFreePage() segment- ",GET_RECLAIMED_SEGMENT());
//			PRINT_MSG_AND_NUM("\nfindNextFreePage() - phy_addr = ",phy_addr);

			/* state is reclamation. check if we should copy a page.*/
			/* if no need to copy - found an empty page, return successfuly*/
//			PRINT_MSG_AND_NUM("\nfindNextFreePage() - check if to copy page phy_addr=",phy_addr);
			if(!is_page_to_copy(flags_ptr, phy_addr)){
//				PRINT_MSG_AND_NUM("\nfindNextFreePage() - dont copy page. return ok_error_code=",ok_error_code);
				return ok_error_code;
			}

			/* try copying.*/
			/* if we are copying from a regular address, pass 0 as original physical address*/
			if(!isAddressReserve(phy_addr)){
				phy_addr = 0;
			}
//			PRINT("\ndo copyValidBlock()");
//			valid_copies++;
			SEQ_VERIFY(!copyValidBlock(target_phy_addr, phy_addr));
//			{
//				uint8_t buf[NAND_TOTAL_SIZE];
//				page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//				nandReadPageTotal(buf, 64576);
//				if(GET_HEADER_SEGMENT_ID(page_header_ptr)!= temp){
//					PRINT_MSG_AND_NUM("\n	findNextFreePage() - after copyValidBlock(). page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
//					PRINT_MSG_AND_NUM("\n   rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//					PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());
//					PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//				}
//			}
			init_buf(sequencing_buffer);
			/* advance segment map pointer to next address*/
			INCREMENT_RECLAIMED_OFFSET();
		}
#ifndef EXACT_COUNTERS
		if(moved_seg_flag){
//			PRINT_MSG_AND_NUM("\nfindNextFreePage() - mark segment as completely full ", GET_RECLAIMED_SEGMENT());
//			PRINT_MSG_AND_NUM(" nSegmentIterations=",nSegmentIterations);
			change_counter_level(OBS_COUNT_FULL, GET_RECLAIMED_SEGMENT());
		}
#endif
//		PRINT("\nfindNextFreePage() - finished seg iteration");
	}

//	PRINT("\nfindNextFreePage() - finished segment iterations. error");
	init_buf(sequencing_buffer);

	return ERROR_FLASH_FULL;
}

/**
 * @brief
 * auxiliart to commit().
 * set is commit flag according to page index in the checkpoint
 */
void setIsCommit(bool_t *isCommit, bool_t isCommitLastPage){
	if(isCommitLastPage)
		*isCommit = IS_COMMIT_LAST;
	else
		*isCommit = IS_COMMIT_REGULAR;
}

/**
 * @brief
 * write header for a new allocated segment in seg map. incre
 *
 * Assumptions:
 * 1. new_slot_id , is_eu_reserve, reserve_eu_addr fields of seg map are marked properly
 * @return return 0 if successful. return 1 if an error occured in writing.
 * return 1 if an error occured during commit
 */
error_t writeSegmentHeader(checkpoint_writer cp_write_p){
	uint32_t address = CALC_ADDRESS(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr),0,0);
	page_area_flags *page_header_ptr = ((page_area_flags*)(sequencing_buffer));
	INIT_FLAGS_POINTER_TO_BUFFER(flags,sequencing_buffer);

//	PRINT("\nwriteSegmentHeader() - starting");
//	PRINT_MSG_AND_NUM(" new_slot_id: ", seg_map_ptr->new_slot_id);
//	PRINT_MSG_AND_NUM(" expected address", address);

	/* set header in page area*/
	init_buf(sequencing_buffer);
	SET_HEADER_SEGMENT_ID(page_header_ptr, GET_RECLAIMED_SEGMENT());
	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	SET_HEADER_SEQUENCE_NUM(page_header_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
	/* set header in flags area*/
	SET_SEG_TYPE_FLAG(flags, SEG_TYPE_USED);

//	PRINT_MSG_AND_NUM("\nis rec addr reserve?=", GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr) == IS_EU_RESERVE_TRUE);
	/* if the first EU is reserve, update flags indicating which flag it is replcaing*/
	if(GET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr) == IS_EU_RESERVE_TRUE){
		SET_SLOT_EU_OFFSET(flags, CALCULATE_SLOT_ID_AND_EU_OFFSET(address));
		address = GET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr);
//		PRINT_MSG_AND_NUM("\nnow address=", address);
	}

//	PRINT_MSG_AND_NUM("\nwriteSegmentHeader() - about to write header to address - ", address);
	/* write segment header first page
	 * NOTICE - use direct programming, not allocAndWriteBlock(). This is important, and
	 * taken into consideration in macro IS_SEG_MAP_PAGE_OFFSET_OVERFLOWING() */
//	L("call nandProgramTotalPage() on phy_addr %d", address);
	SEQ_VERIFY(!nandProgramTotalPage(sequencing_buffer, address));

//	{
////		if(seg_map_ptr->new_slot_id == 0){
//		uint8_t buf[NAND_TOTAL_SIZE];
//		page_area_flags *page_header_ptr = ((page_area_flags*)(buf));
//		nandReadPageTotal(buf, 64576);
//		PRINT_MSG_AND_NUM("\nwriteSegmentHeader() - after programming seg header, page 64576 header seg id=", GET_HEADER_SEGMENT_ID(page_header_ptr));
////		}
//	}
	/* advance address pointer*/
	INCREMENT_RECLAIMED_OFFSET(seg_map_ptr);
//	PRINT("\nwriteSegmentHeader() - about to write checkpoint");
	SEQ_VERIFY(!cp_write_p(1));

//	PRINT("\nwriteSegmentHeader() - write checkpoint success. finished");
	return 0;
}

/**
 * @brief
 * write a header for a reserve segment in addres phy_addr
 * @param phy_addr first valid address in the segment
 * @return 0 if sucessful, 1 if no valid EU is found in the slot
 */
error_t writeReserveSegmentHeader(uint32_t phy_addr){
	page_area_flags *page_header_ptr;
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);

	init_buf(sequencing_buffer);

	page_header_ptr = (page_area_flags*)(sequencing_buffer);
	/* set header flags in spare area*/
	SET_SEG_TYPE_FLAG(seq_flags, SEG_TYPE_RESERVE);
	SET_SLOT_EU_OFFSET(seq_flags, SEQ_RESERVE_EU_IS_HEADER);
	INCREMENT_SEG_MAP_SEQUENCE_NUM(seg_map_ptr);
	/* set header flags in page area*/
	SET_HEADER_SEQUENCE_NUM(page_header_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
	SET_HEADER_SEGMENT_ID(page_header_ptr, GET_SLOT(phy_addr));

	/*try writing until we succeed, and not go to next slot*/
//	L("call nandProgramTotalPage() on phy_addr %d", phy_addr);
	while(nandProgramTotalPage(sequencing_buffer, phy_addr)){
		markEuAsBad(phy_addr);
		phy_addr+=NAND_PAGES_PER_ERASE_UNIT;
	}

	return 0;
}

error_t moveToNextSegment(spare_area_flags *flags_ptr, bool_t *cpWritten, checkpoint_writer cp_write_p){
	uint32_t old_reclaimed_seg, res ,reserve_phy_addr,new_slot = SEQ_SEGMENTS_COUNT; /* by default we assume the most delicate case-moving from allocation to reclamation*/
//	PRINT("\nmoveToNextSegment() - starting");
//	PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());
//	PRINT_MSG_AND_NUM(" new_slot_id=",seg_map_ptr->new_slot_id);
//	PRINT_MSG_AND_NUM(" nSegments=",seg_map_ptr->nSegments);

	/* get last reclaimed segment's slot */
	res = GET_RECLAIMED_SEGMENT_SLOT();

	/* if we are in reclamation, truncate old segment*/
	if(IS_STATE_RECLAMATION()){
//		PRINT("\nmoveToNextSegment() - state is reclamation, truncate old slot");
		if(truncateSlot(res, 0)){
//			PRINT("\nmoveToNextSegment() - failed truncating");
			return 1;
		}

		/* this will now be the new slot*/
		new_slot = res;
		/* change references in segment map:
		 * - segment reference is not to new slot
		 * - new slot is now the slot that was truncated */
		SET_SEGMENT_TO_SLOT(GET_RECLAIMED_SEGMENT(), seg_map_ptr->new_slot_id);
	}

	/* and the reclaimed segment is now the previously reclaimed one */
	SET_SEG_MAP_PREV_SEG(seg_map_ptr, GET_RECLAIMED_SEGMENT());

	/* now we can choose the new segment to reclaim/allocate to*/
	if(ALL_SEGMENTS_ALLOCATED()){
//		PRINT("\nmoveToNextSegment() - all segments allocated");
		/* NOTICE - this is here for two situations
		 * 1. we were in reclamation mode - new slot is what was truncated
		 * 2. we just moved from regular allocation to reclamation - new slot is SEQ_SEGMENTS_COUNT */
		SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, new_slot);
//		PRINT_MSG_AND_NUM("\nmoveToNextSegment() - seg_map_ptr->new_slot_id = ",seg_map_ptr->new_slot_id);
//		PRINT("\nmoveToNextSegment() - state is reclamation, toss coin to decide whether to perform wear leveling");
//		printf("\nmoveToNextSegment() - lfsr_state=%d", lfsr_state);
		if(gen_rand_bit(WEAR_NEEDED_PROB)){
//			PRINT("\nmoveToNextSegment() - perform wear leveling");
			/* tossed coin to decide whether to perform wear leveling.
			 * for optimization we do this only in reclamation mode.
			 * perform wear leveling and verify no error*/
			res = performWearLeveling();
//			PRINT_MSG_AND_NUM("\nmoveToNextSegment() - performWearLeveling res=", res);
			if(res)
				return 1;
		}

		old_reclaimed_seg = GET_RECLAIMED_SEGMENT();
//		PRINT_MSG_AND_NUM("\nallocAndWriteBlock() - find segment to reclaim. old reclaimed segment=", seg_map_ptr->previously_written_segment);
		/* find a segment to reclaim, with no restrictions*/
		if(findSegmentToReclaim()){
//					PRINT("\nallocAndWriteBlock() - error finding segment to reclaim");
			return ERROR_FLASH_FULL;
		}

//		PRINT_MSG_AND_NUM("\nmoveToNextSegment() - found segment to reclaim=", GET_RECLAIMED_SEGMENT());
	}
	else{
//		PRINT("\nallocAndWriteBlock() - not in reclamation");
		/* we are not in reclamation. simply continue to next physical slot*/
		seg_map_ptr->new_slot_id += 1;
		/* if we're not in reclamation increment segments count*/
		INCREMENT_SEG_MAP_NSEGMENTS(seg_map_ptr);
		SET_RECLAIMED_SEGMENT(GET_RECLAIMED_SEGMENT()+1);
		SET_SEGMENT_TO_SLOT(GET_RECLAIMED_SEGMENT(),GET_RECLAIMED_SEGMENT());
	}

//	PRINT("\nallocAndWriteBlock() - init page offset");
	/* init page offset*/
	SET_RECLAIMED_OFFSET(0);

	/* check vailidity of first EU in new slot*/
//	L("check validity of next segment first eu in addr %d", CALC_ADDRESS(seg_map_ptr->new_slot_id,0,0));
	if(!seq_check_eu_status(CALC_ADDRESS(seg_map_ptr->new_slot_id,0,0))){
//		L("first EU in segment is not valid");
		seg_map_ptr->is_eu_reserve   = IS_EU_RESERVE_TRUE;
		reserve_phy_addr = allocReserveEU(flags_ptr,CALC_ADDRESS(seg_map_ptr->new_slot_id,0,0));
//		L("allocReserveEU() returned reserve_phy_addr %d", reserve_phy_addr);
		VALIDATE_RESERVE_ADDRESS(reserve_phy_addr);
		SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, reserve_phy_addr);
		SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr, IS_EU_RESERVE_TRUE); /* 26/12/07 Aviad */
	}

	/* try writing segment header*/
//	L("about to write segment header");
//	PRINT_MSG_AND_NUM("\nmoveToNextSegment() - before writing seg header,  GET_RECLAIMED_OFFSET()=", GET_RECLAIMED_OFFSET());
	if(writeSegmentHeader(cp_write_p)){
//		PRINT("\nmoveToNextSegment() - failure in segment header");
		return 1;
	}

//	PRINT_MSG_AND_NUM("\nmoveToNextSegment() - after writing seg header,  rec offset=", GET_RECLAIMED_OFFSET());

	/* segment is now initialized. initialize it's obs counter as well*/
//	PRINT("\nmoveToNextSegmenT() - decide whther to change obs counter level");
	if(IS_STATE_RECLAMATION()){
#ifndef EXACT_COUNTERS
//		PRINT_MSG_AND_NUM("\nmoveToNextSegmenT() - changing counter level to segment ", GET_RECLAIMED_SEGMENT());
		change_counter_level(OBS_COUNT_NO_OBSOLETES,GET_RECLAIMED_SEGMENT());
#else
		change_counter_level(0,GET_RECLAIMED_SEGMENT());
#endif
//		PRINT_MSG_AND_NUM("\nmoveToNextSegment() - get_counter_level(",GET_RECLAIMED_SEGMENT());
//		PRINT_MSG_AND_NUM(")=",get_counter_level(GET_RECLAIMED_SEGMENT()));
	}

//	PRINT_MSG_AND_NUM("\nmoveToNextSegment() - get_counter_level(5)=",get_counter_level(5));
	*cpWritten = 1;

//	PRINT_MSG_AND_NUM("\nmoveToNextSegment() - seg_map_ptr->new_slot_id=",seg_map_ptr->new_slot_id);
//	PRINT("\nmoveToNextSegment() - finished");
	return 0;
}

/**
 * @brief
 * auxiliary to findNextFreePage()
 */
uint32_t calc_target_phy_addr(){
	if(IS_SEG_MAP_ADDR_RESERVE()){
//		L("seg map addr is reserve. seg_map_ptr->reserve_eu_addr=%d, add to it offset in eu %d", seg_map_ptr->reserve_eu_addr, CALC_OFFSET_IN_EU(GET_RECLAIMED_OFFSET()));
		return (seg_map_ptr->reserve_eu_addr)+CALC_OFFSET_IN_EU(GET_RECLAIMED_OFFSET());
	}

	/* seg map addr is not reserve.
	 * check, and if not in reclamtion simply translate address to physical*/
	if(!IS_STATE_RECLAMATION()){
//		L("we are not in reclamation. return %d",CALC_RECLAIMED_PHYSICAL_ADDRESS());
		return CALC_RECLAIMED_PHYSICAL_ADDRESS();
	}

	/* we are in reclamation, we should take address fron new slot*/
//	L("we are in reclamation. return %d",CALC_ADDRESS(seg_map_ptr->new_slot_id,0,GET_RECLAIMED_OFFSET()));
	return CALC_ADDRESS(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr),0,GET_RECLAIMED_OFFSET());
}

///**
// * @brief
// * initialize a logical address
// * @param log_addr pointer to a logical address
// */
//void init_logical_address(logical_addr_t log_addr){
//	log_addr->segment_num = SEQ_NO_SLOT;
//	log_addr->page_offset = SEQ_NO_PAGE_OFFSET;
//}

/**
 * @brief
 * initialize a segment map
 */
void init_seg_map(){
	uint32_t i;
	init_struct(seg_map_ptr, sizeof(segment_map));

	for(i=0; i< SEQ_N_SLOTS_COUNT; i++){
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,SEQ_NO_SLOT,i);
	}

	SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr,IS_EU_RESERVE_FALSE);
	SET_SEG_MAP_NSEGMENTS(seg_map_ptr, 0);
}

/**
 * @bried
 * initialize obsolete counters
 */
void init_obs_counters(){
	uint32_t i;

	for(i=0; i< SEQ_OBS_COUNTERS; i++){
#ifndef EXACT_COUNTERS
		change_counter_level(OBS_COUNT_NO_OBSOLETES, i);
#else
		INIT_EXACT_COUNTER(i);
#endif
	}

	SET_OBS_COUNT(0);
	SET_FREE_COUNTER(0);
}

/**
 * @brief
 * auxiliary. program with consideration to obsolete byte
 * @param buf data buffer
 * @param phy_addr physical address to write to
 * @return 0 if successful, 1 if failed program
 */
error_t nandProgramTotalPage(void *buf, uint32_t phy_addr){
	INIT_FLAGS_POINTER_TO_BUFFER(flags, buf);
//	PRINT_MSG_AND_NUM("\nnandProgramTotalPage() - obs flag=",GET_OBS_FLAG(flags));
//	PRINT_MSG_AND_NUM(" page=",phy_addr);
	/* if the page is marked as obsolete, make a complete write*/
	if(GET_OBS_FLAG(flags) == OBS_FLAG_TRUE){
//		L("write full page %d", phy_addr);
		SEQ_VERIFY(!nandProgramPageA(CAST_TO_UINT8(buf), phy_addr, 0,NAND_TOTAL_SIZE));
	}
	else{
//		L("write partial page %d", phy_addr);
		SEQ_VERIFY(!nandProgramPageA(CAST_TO_UINT8(buf), phy_addr, 0,NAND_TOTAL_SIZE-1));
	}

//	PRINT("\nnandProgramTotalPage(0 - success");
	return 0;
}

/**
 * @brief
 * auxiliary, check if a given physical address is in a slot allocated for reserve addresses
 *
 * @param phy_addr a physical address in flash
 * @return 1 if the address is in a reserve slot, 0 otherwise
 */
bool_t isAddressReserve(uint32_t phy_addr){
	uint32_t i;

	for(i=SEQ_FIRST_RESERVE_SEGMENT; i< SEQ_N_SLOTS_COUNT; i++){
		if(GET_SLOT(phy_addr) == GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, i))
			return 1;
	}

	return 0;
}

/**
 * @brief
 * find the valid EU num eu_location in a given slot
 * @param slot_id physical slot number
 * @param eu_location requested eu location in the slot
 * @return address of first valid EU in slot. if all slots are bad return SEQ_PHY_ADDRESS_EMPTY
 */
uint32_t get_valid_eu_addr_in_location(uint32_t slot_id, uint32_t eu_location){
	uint32_t phy_addr;

	assert(slot_id<SEQ_N_SLOTS_COUNT);
	phy_addr = CALC_ADDRESS(slot_id,0,0);
//	PRINT_MSG_AND_NUM("\nget_valid_eu_addr_in_location() - slot_id=",slot_id);
//	PRINT_MSG_AND_NUM(" eu_location=",eu_location);
//	PRINT_MSG_AND_NUM("\nget_valid_eu_addr_in_location() - phy_addr=",phy_addr);
	do{
		/* after finding the first valid EU in the segment, return it
		 * check if it is the one we want */
		if(seq_check_eu_status(phy_addr) ){
			eu_location--;
			if(eu_location==0)
				return phy_addr;
		}

		// not verified. advance to next EU address
		phy_addr += NAND_PAGES_PER_ERASE_UNIT;
//		PRINT_MSG_AND_NUM("\nget_valid_eu_addr_in_location() - phy_addr=",phy_addr);
	}while (phy_addr< CALC_ADDRESS(slot_id+1,0,0));

	return SEQ_PHY_ADDRESS_EMPTY;
}

/**
 * @brief
 * auxiliary to booting.
 * find the first EU that would have been allocated as a replacement for phy_addr
 * and erase it, in case we started programming on it but failed.
 *
 * @return 0 if successful, 1 otherwise
 */
error_t erasePossibleEUReplacement(uint32_t phy_addr){
	uint32_t seg_idx;
	uint32_t reserve_slot_address, reserve_slot_eu_offset;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	uint32_t eu_offset;
	uint32_t expected_cb_eu;

	eu_offset      = CALC_EU_OFFSET(phy_addr);
	expected_cb_eu = get_valid_eu_addr_in_location(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,SEQ_FIRST_RESERVE_SEGMENT),1);

	SEQ_VERIFY(!IS_PHY_ADDR_EMPTY(expected_cb_eu));

	for(seg_idx= SEQ_FIRST_RESERVE_SEGMENT; seg_idx<SEQ_N_SLOTS_COUNT;seg_idx++){
		/* iterate all EUs in segment until we find reserve EU
		 * start from EU in parallel offset, and continue cyclically */
		reserve_slot_eu_offset = eu_offset;

		reserve_slot_address   = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg_idx],reserve_slot_eu_offset,0);
		do{
			/* don't erase copy back eu, that has no meaning*/
			if(reserve_slot_address != expected_cb_eu){
				// if the eu is valid, check if it is the one we want
				if(seq_check_eu_status(reserve_slot_address)){
					nandReadPageFlags(flags, reserve_slot_address);
					// verify the reserve eu isn't used
					if(!IS_PAGE_USED(flags))
					   return nandErase(reserve_slot_address);
				}
				else{
					/* maybe EU is bad as a result of a bad program*/
					nandErase(reserve_slot_address);

					/* if it is now ok */
					if(seq_check_eu_status(reserve_slot_address)){
						nandReadPageFlags(flags, reserve_slot_address);
					/* verify the reserve eu isn't used*/
						if(!IS_PAGE_USED(flags))
						   return nandErase(reserve_slot_address);
					}
				}
			}
			/* not verified. advance to next EU address */
			reserve_slot_eu_offset +=1;
			reserve_slot_address   += NAND_PAGES_PER_ERASE_UNIT;

			/* if overflowed the slot pages, return to start (cyclic)*/
			if(reserve_slot_eu_offset == SEQ_EUS_PER_SLOT){
				reserve_slot_address   = reserve_slot_address - SEQ_PAGES_PER_SLOT;
				reserve_slot_eu_offset = 0;
			}
		}while (reserve_slot_eu_offset != eu_offset);
	}
//	PRINT("\nerasePossibleEUReplacement() - finished, found no replacment");
	return 1;
}


/**
 * @brief
 * count segments of a given type
 * @param isReserve shoudl we count reserve segmnets
 * @return number of segmnets from requested type
 */
uint32_t countSegments(bool_t isReserve){
	uint32_t seg_count = 0, i = 0, max = SEQ_SEGMENTS_COUNT;

	if(isReserve){
		i = SEQ_FIRST_RESERVE_SEGMENT;
		max = SEQ_N_SLOTS_COUNT;
	}

	for(;i<max;i++){
//		PRINT_MSG_AND_NUM("\ncountSegments() - ",);
		if(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, i) != SEQ_NO_SLOT)
			seg_count++;
	}

	return seg_count;
}

/**
 * @brief
 * find the last valid page in a given EU
 *
 * Assumptions:
 * 1. first page is valid and programmed
 * @param eu_start_addr eu we're looking at physical address
 * @param lastValid pointer where to save the last valid page
 */
error_t findLastValidPageInEU(uint32_t eu_start_addr,uint32_t *lastValid){
	uint32_t i,start, end, mid;
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
//	PRINT("\nfindLastValidPageInEU() - starting");
//	PRINT_MSG_AND_NUM(" eu_start_addr=",eu_start_addr);

    start = eu_start_addr;
	end   = eu_start_addr +NAND_MAX_EU_OFFSET;

//	 {
//    	INIT_FLAGS_STRUCT_AND_PTR(flags);
//    	nandReadPageFlags(flags, 578);
//    	PRINT_MSG_AND_NUM("\n100. is page 578 used?=",IS_PAGE_USED(flags));
//    	}
    /* get last page with data in the EU*/
    for(i=POWER_NAND_PAGES_PER_ERASE_UNIT; i>0 ; i--){
    	mid = DIV_BY_2(start + end);

//    	PRINT_MSG_AND_NUM("\nfindLastValidPageInEU() - mid=",mid);
    	if(nandReadPageTotal(sequencing_buffer, mid)){
    		end = mid-1;
    		continue;
    	}

    	if(IS_PAGE_USED(seq_flags))
    		start = mid;
    	else
    		end = mid;
//
//    	PRINT_MSG_AND_NUM("\nfindLastValidPageInEU() - start_eu=",start);
//    	PRINT_MSG_AND_NUM("\nfindLastValidPageInEU() - end_eu=",end);
    }
//    PRINT("\nfindLastValidPageInEU(0 - finished loop.");
    *lastValid = start;

    /* it might be end_eu*/
    nandReadPageTotal(sequencing_buffer, end);
    if(IS_PAGE_USED(seq_flags))
    	*lastValid = end;

    return 0;
}

/**
 * @brief
 * auxiliary to findLastValidPage().
 * check if a given eu (or it's reserve) is valid and used
 *
 * @param end_eu pointer to eu
 * @return 1 if used and valid, 0 otherwise
 */
bool_t isUsedEU(spare_area_flags *flags, uint32_t *eu_phy_addr){
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);

	init_logical_address(log_addr);

//	PRINT_MSG_AND_NUM("\nisUsedEU()- *eu_phy_addr=",*eu_phy_addr);
	/* check if we need to look at it's reserve*/
	if(!seq_check_eu_status(*eu_phy_addr)){
		initFlags(flags);
		SET_LOGICAL_OFFSET(log_addr, CALC_OFFSET_IN_SLOT(*eu_phy_addr));
		*eu_phy_addr = logicalAddressToReservePhysical(flags,*eu_phy_addr);

		if(IS_PHY_ADDR_EMPTY(*eu_phy_addr))
			return 0;
	}

//	PRINT_MSG_AND_NUM("\nisUsedEU()- after checking validity *eu_phy_addr=",*eu_phy_addr);

	/* try reading eu*/
    if(nandReadPageTotal(sequencing_buffer, *eu_phy_addr)){
		return 0;
    }

//    PRINT_MSG_AND_NUM("\nisUsedEU()- is *eu_phy_addr used?",IS_PAGE_USED(seq_flags));
    /* if end eu is valid and used, it is last_used_eu*/
    if(IS_PAGE_USED(seq_flags)){
    	return 1;
    }

    return 0;
}

/**
 * @brief
 * find the last valid page in slot #slot_id, using binary search. store it in lastValid.
 * if none is found SEQ_PHY_ADDRESS_EMPTY is stoed in lastValid
 * @param lastValid pointer to address variable to save address to
 * @param slot_id physical slot id
 * @return 0 if successful. 1 in case of an error
 */
error_t  findLastValidPageInSlot(uint32_t *lastValid, uint32_t slot_id){
	uint32_t start_eu = CALC_ADDRESS(slot_id,0,0), end_eu = start_eu+SEQ_PAGES_PER_SLOT-NAND_PAGES_PER_ERASE_UNIT;
	uint32_t orig_mid_eu, mid_eu, last_used_eu = start_eu;
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);

//	PRINT("\nfindLastValidPageInSlot() - starting");
//	PRINT_MSG_AND_NUM("\nfindLastValidPageInSlot() - slot_id=",slot_id);
	while (end_eu-start_eu>NAND_PAGES_PER_ERASE_UNIT) {
//		if(mid_eu>CALC_ADDRESS(slot_id,0,0)){
//			PRINT_MSG_AND_NUM("\nfindLastValidPageInSlot() - b4 next iteration mid_eu=",mid_eu);
//			PRINT_MSG_AND_NUM(" end_eu=",end_eu);
//			PRINT_MSG_AND_NUM(" start_eu=",start_eu);
//			PRINT_MSG_AND_NUM(" last_used_eu=",last_used_eu);
//		}
		/* look at middle eu*/
		mid_eu      = CALC_EU_START(DIV_BY_2(start_eu + end_eu));
		orig_mid_eu = mid_eu;

		if(!seq_check_eu_status(mid_eu)){
			initFlags(flags);
			SET_LOGICAL_OFFSET(log_addr, CALC_OFFSET_IN_SLOT(mid_eu));
			mid_eu = logicalAddressToReservePhysical(flags,orig_mid_eu);
//			PRINT_MSG_AND_NUM("\nmid_eu not valid. replcament is ",mid_eu );

			/* if none was found then we didn't start writing in this offset for sure*/
			if(IS_PHY_ADDR_EMPTY(mid_eu)){
//				PRINT("\naddress contains no data");
				end_eu = orig_mid_eu;
			}else{
				/* we found a used reserve EU for orig_mid_eu*/
				/* NOTICE - orig_mid_eu, since we need it to calcaulte future mid_eu's*/
				start_eu     = orig_mid_eu;
				/* NOTICE - mid_eu, since it is a reserve one.*/
				last_used_eu = mid_eu;
			}

//			PRINT_MSG_AND_NUM("\nfindLastValidPageInSlot() - b4 next iteration mid_eu=",mid_eu);
//			PRINT_MSG_AND_NUM(" end_eu=",end_eu);
//			PRINT_MSG_AND_NUM(" start_eu=",start_eu);
//			PRINT_MSG_AND_NUM(" last_used_eu=",last_used_eu);
			continue;
		}

    	/* else, EU is valid. check it. read current mid_eu and check if contains data
         * if a read error occurs continue reading until we find a good one.
   	     *  if page not ECCed, then this is the result of a bad program*/
   	    if(nandReadPageTotal(sequencing_buffer, mid_eu)){
   	    	assert(!nandErase(mid_eu));
   	    	end_eu = mid_eu;

//   	    	PRINT_MSG_AND_NUM("\nfindLastValidPageInSlot() - b4 next iteration mid_eu=",mid_eu);
//			PRINT_MSG_AND_NUM(" end_eu=",end_eu);
//			PRINT_MSG_AND_NUM(" start_eu=",start_eu);
//			PRINT_MSG_AND_NUM(" last_used_eu=",last_used_eu);
   	    	continue;
   	    }

//   	    PRINT_MSG_AND_NUM("\nfindLastValidPageInSlot() - check if page is used ",IS_PAGE_USED(seq_flags));
//   	    PRINT_MSG_AND_NUM(" page ",mid_eu);
   	    if(!IS_PAGE_USED(seq_flags)){
   	    	end_eu = mid_eu;
   	    }
   	    else{
   	    	start_eu = mid_eu;
   	    	last_used_eu = start_eu;
   	    }

//   	    PRINT_MSG_AND_NUM("\nfindLastValidPageInSlot() - b4 next iteration mid_eu=",mid_eu);
//		PRINT_MSG_AND_NUM(" end_eu=",end_eu);
//		PRINT_MSG_AND_NUM(" start_eu=",start_eu);
//		PRINT_MSG_AND_NUM(" last_used_eu=",last_used_eu);
	}

//    PRINT("\nfindLastValidPageInSlot() - finished loop.");
//    PRINT_MSG_AND_NUM(" end_eu=",end_eu);
//    PRINT_MSG_AND_NUM(" start_eu=",start_eu);
//    PRINT_MSG_AND_NUM(" last_used_eu=",last_used_eu);
//
	 /* verify it is not the end eu.
	  * if not, verify it is start eu (or actually it's reserve )*/
	if(isUsedEU(flags, &end_eu)){
		last_used_eu = end_eu;
	}
    else if(isUsedEU(flags, &start_eu)){
		last_used_eu = start_eu;
	}

    SEQ_VERIFY(!findLastValidPageInEU(last_used_eu,lastValid));

//    PRINT_MSG_AND_NUM("\nfindLastValidPageInSlot() - finished. lastValid=",*lastValid);
//    assert(0);
	return 0;
}

/**
 * @brief
 * find last valid page in an EU
 *
 * Assumptions:
 * 1. a bad read is the resuly of a bad program. page before it is the last valid (last proper program)
 * @param eu_phy_addr physical address of EU to check
 * @return SEQ_PHY_ADDRESS_EMPTY if only written page is the first and it is bad. otherwise the physical address
 */
uint32_t getLastValidEuPage(uint32_t eu_phy_addr){
	uint32_t res, i, mid, start = eu_phy_addr,end = start + NAND_PAGES_PER_ERASE_UNIT-1;
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);

	for(i=POWER_NAND_PAGES_PER_ERASE_UNIT; i>0 ; i--){
    	mid = CALC_EU_START(DIV_BY_2(start + end));

    	res = nandReadPageTotal(sequencing_buffer, mid);

    	/* if page not ECCed then it is bad
    	 * return error */
    	if(res){
    		if(mid ==eu_phy_addr){
    			return SEQ_FIRST_PHY_ADDRESS_EMPTY;
    		}

    		return mid-1;
    	}

    	if(IS_PAGE_USED(seq_flags))
    		start = mid;
    	else
    		end = mid;
    }

    return end;
}

/**
 * @brief
 * handle case we have two copies of the same reserve segment.
 * the one which is less complete will be deleted. iterate backwards until you find a page with data
 * in an offset that the second slot has no data
 *
 * @param seg_id segment who has two slots
 * @param slot_id1 first slot copy id, which is currently marked as containing seg_id in seg map
 * @param slot_id2 second slot copy id
 * @return 0 if successful
 */
error_t handleDoubleReserveSlot(uint32_t seg_id, uint32_t slot_id1, uint32_t slot_id2){
	uint32_t phy_addr1, phy_addr2, res1, res2;
	bool_t is_used1, is_used2;
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	phy_addr1 = CALC_ADDRESS(slot_id1+1,0,0)-NAND_PAGES_PER_ERASE_UNIT;
	phy_addr2 = CALC_ADDRESS(slot_id2+1,0,0)-NAND_PAGES_PER_ERASE_UNIT;

	/* iterate backwards*/
	while(1){
	      /* advance to next valid EU's*/
	      while(!seq_check_eu_status(phy_addr1))
	      	phy_addr1-=NAND_PAGES_PER_ERASE_UNIT;

	      while(!seq_check_eu_status(phy_addr2))
	      	phy_addr2-=NAND_PAGES_PER_ERASE_UNIT;

	      /* check used status of each EU first page*/
	      nandReadPageTotal(sequencing_buffer, phy_addr1);
	      is_used1 = IS_PAGE_USED(seq_flags);

	      nandReadPageTotal(sequencing_buffer, phy_addr2);
	      is_used2 = IS_PAGE_USED(seq_flags);

	      /* if used status is identical and not used - continue to previous EU*/
	      if(!is_used1 && !is_used2){
	      	phy_addr1-=NAND_PAGES_PER_ERASE_UNIT;
	      	phy_addr2-=NAND_PAGES_PER_ERASE_UNIT;
	      	continue;
	      }

	      if(is_used1 == is_used2){
	      	res1 = getLastValidEuPage(phy_addr1);
	      	res2 = getLastValidEuPage(phy_addr2);

	      	/* if last valid page is identical then we can simply erase one of them. for convinience erase the second
	      	 * also if the second slot last valid offset is actually non-existent erase it*/
	      	if(res1>=res2 || IS_PHY_ADDR_EMPTY(res2))
	      		is_used1 = 1;
	      	else
	      		is_used1 = 0;

	      }
	      /* we know for sure which one is with the furthest offset*/
	      break;
	}

	/* check which slot to erase (and if we should change seg-to-slot map)*/
	if(is_used1){
		SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, slot_id2);
		if(truncateSlot(slot_id2,1))
			return 1;
	}
	else{
		SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, slot_id1);
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, seg_id, slot_id2);
		if(truncateSlot(slot_id1,1))
			return 1;
	}

	return 0;
}

/**
 * @brief
 * complete writing of all reserve segment headers
 */
error_t handleReservesAreEmpty(){
	uint32_t i, first_valid_eu;

	/* if we have less than expected reserve segments - we crashed during initial booting
	 * in stage (1) marking reserve segments
	 * then we didn't complete writing all reserve segments - DO IT!*/
	i = SEQ_FIRST_RESERVE_SEGMENT;
	while(i<SEQ_N_SLOTS_COUNT){
//			PRINT_MSG_AND_NUM("\nhandleReservesAreEmpty() - RESERVE_SEGMENTS_COUNT()=",RESERVE_SEGMENTS_COUNT());
//			PRINT_MSG_AND_NUM("\nhandleReservesAreEmpty() - SEQ_K_RESERVE_SLOTS=",SEQ_K_RESERVE_SLOTS);
		/* find first not allocated reserve segment*/
		if(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,i) != SEQ_NO_SLOT){
			i++;
			continue;
		}

//			PRINT_MSG_AND_NUM("\nhandleReservesAreEmpty() - slot for next reserve segment to allocate is ",i);
		first_valid_eu = get_valid_eu_addr_in_location(i,1);
//			PRINT_MSG_AND_NUM("\nhandleReservesAreEmpty() - first_valid_eu=",first_valid_eu);
		/* write header*/
		SEQ_VERIFY(!writeReserveSegmentHeader(first_valid_eu));
		/* mark in seg map*/
//			PRINT_MSG_AND_NUM("\nhandleReservesAreEmpty() - GET_SLOT(first_valid_eu)=",GET_SLOT(first_valid_eu));
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,GET_SLOT(first_valid_eu),i);
//			PRINT_MSG_AND_NUM("\nhandleReservesAreEmpty() - GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,GET_SLOT(first_valid_eu))=",GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,GET_SLOT(first_valid_eu)));
	}

	return 0;
}
/**
 * @brief
 * find reserve segments in flash.
 * - for every segment, iterate until you find the first valid EU and check if it is a valid reserve segment header.
 * - if there aren't SEQ_K_RESERVE_SLOTS reserve segments, allocate the rest
 * - handle case we crashed during wear levelling of reserve slot
 *
 * as an optimization we read regular segment headers as well, until we stumble upon a bad first slot eu
 * this means we cannot read it without knowing whichare the reserve segments.
 * so we'll do it later in sequencingBotting in a dediacted function.
 *
 * @param badFirstEU indicator whther we had any bad first eu in slot
 * @param doublyAllocatedSegment pointer to save id of segment which is allocated on two slots
 * @param secondSlot the second slot on which doublyAllocatedSegment is allocated
 * @param nBytes file system data size
 * @return 0 if successful
 */
error_t handleReserveSegments(bool_t *badFirstEU, uint32_t *doublyAllocatedSegment, uint32_t *secondSlot){
	uint32_t slot_id, first_valid_eu;
	page_area_flags *page_header_ptr = (page_area_flags*)(sequencing_buffer);
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);

//	PRINT("\nhandleReserveSegments() - start");
	for(slot_id=0 ; slot_id<SEQ_N_SLOTS_COUNT ; slot_id++){
		first_valid_eu = get_valid_eu_addr_in_location(slot_id,1);

		/* if none was found continue to next slot*/
		if(IS_PHY_ADDR_EMPTY(first_valid_eu)){
//			PRINT("\nhandleReserveSegments() - first_valid_eu is empty. continue");
			continue;
		}

//		PRINT_MSG_AND_NUM("\nhandleReserveSegments() - first_valid_eu=",first_valid_eu);
		if(!IS_FIRST_IN_SLOT(first_valid_eu)){
			*badFirstEU = 1;
		}

		if(nandReadPageTotal(sequencing_buffer,first_valid_eu)){
//			PRINT_MSG_AND_NUM("\nhandleReserveSegments() - first_valid_eu read error. continue. first_valid_eu=", first_valid_eu);
			SEQ_VERIFY(!nandErase(first_valid_eu));
			continue;
		}

//		PRINT_MSG_AND_NUM("\nhandleReserveSegments() - seg-type=",GET_SEG_TYPE_FLAG(seq_flags));
//		PRINT_MSG_AND_NUM(" slot_id=",slot_id);
//		PRINT_MSG_AND_NUM(" first_valid_eu=",first_valid_eu);
//		PRINT_MSG_AND_NUM(" new_slot_id=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
		/* if the page is not used, erase it, in case a program failed halfway thru*/
		if(GET_SEG_TYPE_FLAG(seq_flags) == SEG_TYPE_NOT_USED){
//			/* if we have no empty slot marked, mark this one*/
//			if(IS_SLOT_EMPTY(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr))){
//				SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, slot_id);
//			}
//			PRINT_MSG_AND_NUM("\nhandleReserveSegments() - new_slot_id is now=",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
			/* erase page in case we failed in mid-program*/
			nandErase(first_valid_eu);
			continue;
		}
//
//		PRINT_MSG_AND_NUM("\nhandleReserveSegments() - we're in a used segmnet at slot=",slot_id );
//		PRINT_MSG_AND_NUM("\nhandleReserveSegments() - *badFirstEU=",*badFirstEU );
//		PRINT_MSG_AND_NUM("\nhandleReserveSegments() - GET_SEG_TYPE_FLAG(seq_flags)=",GET_SEG_TYPE_FLAG(seq_flags) );
		/* if we still haven't encountered a bad first eu, keep reading data segment headers*/
		if(GET_SEG_TYPE_FLAG(seq_flags) != SEG_TYPE_RESERVE){
			/* if this is not a reserve segment header, we can continue*/
			continue;
		}

//		PRINT_MSG_AND_NUM("\nhandleReserveSegments() - we're in a reserve segmnet at slot ",slot_id );
		/* this IS a reserve segment*/
		/* check if there's another slot allocated for this segmnet.
		 * meaning we're in the middle of wear leveling*/
		if(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, GET_HEADER_SEGMENT_ID(page_header_ptr)) != SEQ_NO_SLOT){
			*doublyAllocatedSegment = GET_HEADER_SEGMENT_ID(page_header_ptr);
			*secondSlot = slot_id;
			continue;
		}
//		PRINT_MSG_AND_NUM("\nhandleReserveSegments() - setting slot to segment  ",GET_HEADER_SEGMENT_ID(page_header_ptr));
		/* we're not in wear leveling. simply continue*/
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, GET_HEADER_SEGMENT_ID(page_header_ptr), slot_id);

		if(GET_HEADER_SEQUENCE_NUM(page_header_ptr) > GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr)){
			SET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr, GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
		}
	}

	/* if we haven't allocated reserve segments*/
	if(RESERVE_SEGMENTS_COUNT()< SEQ_K_RESERVE_SLOTS){
//		 PRINT("\nhandleReserveSegments() -  finish writing all reserve headers. call handleReservesAreEmpty()");
		SEQ_VERIFY(!handleReservesAreEmpty());
	}

//	PRINT("\nhandleReserveSegments() - finished. return 0");
	return 0;
}

/**
 * @brief
 * auxiliary to markSegmentInSegMap.
 * check if a header is complete - includes checkpoint and ecced.
 * if header is not verified then we erase it's EU
 *
 * Assumptions:
 * 1. header_phy_addr is valid
 *
 * @param header_phy_addr physical address of EU containing header
 * @param nBytes size of file system data
 */
bool_t verifyCompleteSegmentHeader(uint32_t header_phy_addr, uint32_t nBytes){
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags,sequencing_buffer);
	init_buf(sequencing_buffer);

//	PRINT("\nverifyCompleteSegmentHeader() - starting");
//	PRINT_MSG_AND_NUM("\nverifyCompleteSegmentHeader() - nBytes=",nBytes);
//	PRINT_MSG_AND_NUM(" cp size=",CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+nBytes));
//	PRINT_MSG_AND_NUM(" read header from=",header_phy_addr +CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+nBytes));
	if(nandReadPageTotal(sequencing_buffer,header_phy_addr +CALCULATE_IN_PAGES(SEQ_CHECKPOINT_SIZE+nBytes))){
//		PRINT("\nmarkSegmentInSegMap() - expected cp page not found, erase first EU of slot");
		nandErase(header_phy_addr);
		return 0;
	}
//	PRINT_MSG_AND_NUM("\nverifyCompleteSegmentHeader() - is last cp page marked as cp? ",GET_CHECKPOINT_FLAG(seq_flags) != CP_NOT_PART_OF_CP);
	/* if the page is not a checkpoint page, then the header was not completely written*/
	if(GET_CHECKPOINT_FLAG(seq_flags) == CP_NOT_PART_OF_CP){
		nandErase(header_phy_addr);
		return 0;
	}

	return 1;
}

/**
 * @brief
 * auxiliary, mark a data segment in segment map according to data in it's header
 *
 * Assumptions:
 * 1. page header is valid
 *
 * @param page_header_ptr pointer to buffer containing
 * @param header_phy_addr physical address of the segment header
 * @param slot_id id of the segment slot
 * @param nBytes file system data size in the checkpoint
 * @return 0 if successful
 */
error_t markSegmentInSegMap(page_area_flags *page_header_ptr, uint32_t header_phy_addr, uint32_t slot_id, uint32_t nBytes){
	uint32_t res,seg_id, sequence_num, lastValid1, lastValid2, slot_to_truncate;
	bool_t isSlotReserve = 0;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);

//	PRINT("\nmarkSegmentInSegMap() - starting");
//	PRINT_MSG_AND_NUM(" slot_id=", slot_id);
	/* page is valid and used for data. extract data*/
	seg_id       = GET_HEADER_SEGMENT_ID(page_header_ptr);
	sequence_num = GET_HEADER_SEQUENCE_NUM(page_header_ptr);
//	PRINT_MSG_AND_NUM(" seg_id=",seg_id);
//	PRINT_MSG_AND_NUM(" sequence_num=",sequence_num);
	isSlotReserve = (seg_id >= SEQ_FIRST_RESERVE_SEGMENT);

	if(!verifyCompleteSegmentHeader(header_phy_addr, nBytes)){
//		PRINT("\nmarkSegmentInSegMap() - segment header not complete");
		return 0;
	}

//	PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - is there no slot for this segment? ",GET_SLOT_BY_SEGMENT(seg_id) == SEQ_NO_SLOT);
	/* check if we already have a slot allocated for this segmnet*/
	if(GET_SLOT_BY_SEGMENT(seg_id) == SEQ_NO_SLOT){
//		PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - no slot marked yet for segment ", seg_id);
		/* try reading from where the last page of the checkpoint hsould be.
		 * if it is a non-valid page, then segment header was not finished*/
//		PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - about to read expected last page of cp ", header_phy_addr +CALCULATE_IN_PAGES(nBytes));

		INCREMENT_SEG_MAP_NSEGMENTS(seg_map_ptr);
		SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, slot_id, seg_id);
//		PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - sequence_num=", sequence_num);
//		PRINT_MSG_AND_NUM(" seg map seq num", GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr));
//		PRINT_MSG_AND_NUM(" slot_id=",slot_id);
		if(sequence_num>GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr) ||
		    GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr)==SEQ_NO_SEQUENCE_NUM){
//			PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - sequence_num>GET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr) for slot ",slot_id);

			SET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr, sequence_num);
			SET_SEG_MAP_PREV_SEG(seg_map_ptr, GET_RECLAIMED_SEGMENT());
			SET_RECLAIMED_SEGMENT(seg_id);
			SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, slot_id);
		}

//		PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - new rec seg=", GET_RECLAIMED_SEGMENT());
//		PRINT_MSG_AND_NUM(" whose slot is=",GET_RECLAIMED_SEGMENT_SLOT());
		return 0;
	}

	/* NOTICE - if we got here then we already have
	 * a slot allocated for this segmnet
	 * check if we located it during handleReservesegments()*/
	if(slot_id==GET_SLOT_BY_SEGMENT(seg_id))
		return 0;

	SET_LOGICAL_OFFSET(log_addr,0);
	SET_LOGICAL_SEGMENT(log_addr,seg_id);
//	PRINT("\nmarkSegmentInSegMap() - logicalAddressToPhysical");
	res = logicalAddressToPhysical(log_addr);
	assert(!IS_PHY_ADDR_EMPTY(res));
//	PRINT_MSG_AND_NUM("\nread seg header from ", res);

	/* the read is definitely good, since we alreday preformed it */
	nandReadPageTotal(sequencing_buffer, res);

//	PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - are we in wear leveling?", GET_HEADER_SEQUENCE_NUM(page_header_ptr) == sequence_num);
	/* if the slot on segmap has the same sequence num as the first one, then we crashed during wear levelling */
	if(GET_HEADER_SEQUENCE_NUM(page_header_ptr) == sequence_num){
//		PRINT("\nmarkSegmentInSegMap() - we are in wear leveling");
		assert(!findLastValidPageInSlot(&lastValid1, GET_SLOT_BY_SEGMENT(seg_id)));
		assert(!findLastValidPageInSlot(&lastValid2, slot_id));

		/* check which slot to erase (and if we should change seg-to-slot map)*/
		if(CALC_OFFSET_IN_SLOT(lastValid1)>CALC_OFFSET_IN_SLOT(lastValid2)){
			slot_to_truncate = slot_id;
		}
		else{
			slot_to_truncate = GET_SLOT_BY_SEGMENT(seg_id);
			SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, seg_id, slot_id);
		}

//		PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - about to truncate slot ", slot_to_truncate);
		/* verify truncation */

		SEQ_VERIFY(!truncateSlot(slot_to_truncate,isSlotReserve));
//
		return 0;
	}


	/* we are in reclamation.
	 * replace prev segment, and reclaimed segment */
//	PRINT("\nmarkSegmentInSegMap() - we are in reclamation");
	SET_SEG_MAP_PREV_SEG(seg_map_ptr,GET_RECLAIMED_SEGMENT());
	SET_RECLAIMED_SEGMENT(seg_id);

	/* compare generations */
	if(GET_HEADER_SEQUENCE_NUM(page_header_ptr) > sequence_num){
//		PRINT("\nmarkSegmentInSegMap() - this is old generation");
		/* currently marked slot is actually the new generation.
		 * mark second slot as the old generation*/
		SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, GET_SLOT_BY_SEGMENT(seg_id));
		SET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr, GET_HEADER_SEQUENCE_NUM(page_header_ptr));
		SET_SEGMENT_TO_SLOT(seg_id, slot_id);
	}
	else{
//		PRINT("\nmarkSegmentInSegMap() - this is new generation");
		/* current marked slot is old generation.
		 * mark second slot as the new generation*/
		SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, slot_id);
		SET_SEG_MAP_SEQUENCE_NUM(seg_map_ptr, sequence_num);
	}

	/* find offset in new generation*/
	assert(!findLastValidPageInSlot(&lastValid1, GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr)));

//	PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - last Valid in new generation=",lastValid1);
//		assert(0);
	/* if new generation slot is full then we can erase old generation */
	if(CALC_OFFSET_IN_SLOT(lastValid1) == SEQ_MAX_LOGICAL_OFFSET){
//		PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - new generation full. erase old generation slot ",GET_SLOT_BY_SEGMENT(seg_id));

		/* - truncate old generation
		 * - set new generation as the segment's slot (definitely not reserve)*/
		SEQ_VERIFY(!truncateSlot(GET_SLOT_BY_SEGMENT(seg_id),0));
//		PRINT_MSG_AND_NUM("\nmarkSegmentInSegMap() - truncated slot ",GET_SLOT_BY_SEGMENT(seg_id));
		SET_SEGMENT_TO_SLOT(seg_id, slot_id);
	}

//	PRINT("\nmarkSegmentInSegMap() - finished");
//	PRINT_MSG_AND_NUM(" new slot id= ",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" nsegments= ",GET_SEG_MAP_NSEGMENTS(seg_map_ptr));
//	PRINT_MSG_AND_NUM(" rec slot= ",GET_RECLAIMED_SEGMENT_SLOT());
//	PRINT_MSG_AND_NUM(" rec segmnet= ",GET_RECLAIMED_SEGMENT());
//	assert(0);
	return 0;
}
/**
 * @brief
 * read slot headers and collect data about allocated data segments.
 * after reading every header mark the segment in seg map.
 * if we have two slots for the same segments, check if one of them is a wear leveling copy.
 * if so - erase it. otherwise - one of them is a new generation of reclamation.
 * check which one, and mark it in segmap
 *
 * Assumptions -
 * 1. reserve segments have already been fully allocated
 * @param nBytes file system data size
 * @return 0 if successful
 */
error_t handleRegularSegments(uint32_t nBytes){
	uint32_t res, slot_id, first_valid_eu, phy_addr;
	page_area_flags *page_header_ptr = (page_area_flags*)(sequencing_buffer);
	uint32_t doublyAllocatedSegment, secondSlot;
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);

	SET_LOGICAL_OFFSET(log_addr, 0);
	SET_SEG_MAP_NSEGMENTS(seg_map_ptr,0);

//	PRINT("\nhandleRegularSegments() - starting");
	/* by default we have no reserve segment in wear leveling*/
	doublyAllocatedSegment = SEQ_NO_SEGMENT_NUM;
	secondSlot = SEQ_NO_SLOT;

	for(slot_id=0 ; slot_id<SEQ_N_SLOTS_COUNT ; slot_id++){
//		PRINT_MSG_AND_NUM("\nhandleRegularSegments() - slot_id=",slot_id);
		phy_addr = CALC_ADDRESS(slot_id,0,0);
//		PRINT_MSG_AND_NUM(" phy_addr=",phy_addr);

		first_valid_eu = get_valid_eu_addr_in_location(slot_id, 1);
		assert(!IS_PHY_ADDR_EMPTY(first_valid_eu));
		SEQ_VERIFY(!nandReadPageTotal(sequencing_buffer, phy_addr));

		/* check if slot_id is reserve. if so it was already handled - continue */
//		PRINT_MSG_AND_NUM("\n phy_addr=",phy_addr);
		if(GET_SEG_TYPE_FLAG(seq_flags) == SEG_TYPE_RESERVE){
//			PRINT_MSG_AND_NUM("\nhandleRegularSegments() - reserve slot_id=",slot_id);
			continue;
		}

		/* slot is not reserve.
		 * check if the first EU is valid in a slot*/
		if(!seq_check_eu_status(phy_addr)){
//			PRINT("\nslot is not reserve, but with bad first EU");
			initFlags(flags);
			phy_addr = logicalAddressToReservePhysical(flags, phy_addr);
//			PRINT_MSG_AND_NUM("\nhandleRegularSegments() - bad first eu. instead read from ",phy_addr);
		}

//		PRINT("\nslot first EU is ok");
		/* if there is no replacment for the EU then no segment was allocated on it*/
		if(IS_PHY_ADDR_EMPTY(phy_addr)){
			/* if we have no empty slot marked, mark this one*/
//				if(IS_SLOT_EMPTY(GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr)))
//					SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, slot_id);
//			PRINT_MSG_AND_NUM("\nhandleRegularSegments() - empty slot_id=",slot_id);
			continue;
		}

		/* try reading what should contain segment header*/
		res = nandReadPageTotal(sequencing_buffer, phy_addr);

		if(res){
			nandErase(phy_addr);
			markEuAsBad(phy_addr);
//			PRINT_MSG_AND_NUM("\nhandleRegularSegments() - bad eu with seg header slot_id=",slot_id);
			continue;
		}

//		PRINT_MSG_AND_NUM("\nhandleRegularSegments() - check if the first page is used", IS_PAGE_USED(seq_flags));
		/* if the segment is not used continue*/
		if(!IS_PAGE_USED(seq_flags)){
			/* erase page in case we failed in mid-program*/
			nandErase(phy_addr);
//			PRINT_MSG_AND_NUM("\nhandleRegularSegments() - empty slot_id=",slot_id);
			continue;
		}

		/* try marking segment in seg map*/
		SEQ_VERIFY(!markSegmentInSegMap(page_header_ptr, phy_addr, slot_id, nBytes));
	}

//	PRINT("\nhandleRegularSegments() - finished");
//	PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
//	PRINT_MSG_AND_NUM(" rec segment=",GET_RECLAIMED_SEGMENT());
//	if(!IS_SEG_EMPTY(GET_RECLAIMED_SEGMENT())){
//		PRINT_MSG_AND_NUM(" rec slot=",GET_RECLAIMED_SEGMENT_SLOT());
//		PRINT_MSG_AND_NUM(" reclamation? ",IS_STATE_RECLAMATION());
//		PRINT_MSG_AND_NUM(" new slot id ",GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
//	}
	return 0;
}

/**
 * @brief
 * handle copy back finish, in case we crashed during a copy back of a reserve EU
 * a. if we don't have a reserve EU for org_phy_addr we crashed before re-writing eat. repeat whole process
 * (after deleting possible replacment)
 * b. we have one. handle cases of different offsets.
 *
 * @param org_phy_addr original physical addressm that the reserev EU covers for
 * @param cb_eu_phy_addr found copy back EU
 * @return 0 if successful, 1 if a read/program/erase error occurs
 */
error_t handleCopyBackSpecial(uint32_t org_phy_addr, uint32_t cb_eu_phy_addr){
	uint32_t offset_in_cb, offset_in_org, reserve_phy_addr;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);

//	PRINT_MSG_AND_NUM("\nhandleCopyBackSpecial() - org_phy_addr=",org_phy_addr);
//	PRINT_MSG_AND_NUM(" cb_eu_phy_addr=",cb_eu_phy_addr);
	SET_LOGICAL_OFFSET(log_addr, 0);

	/* we crashed during a copy back to a reserve EU replacing org_phy_addr.
	 * try finding such a reserve EU*/
	reserve_phy_addr = logicalAddressToReservePhysical(flags, org_phy_addr);
//	PRINT_MSG_AND_NUM("\nhandleCopyBackSpecial() - reserve_phy_addr=",reserve_phy_addr);
//	assert(0);
	/* we calc this anyway */
	offset_in_cb  = CALC_OFFSET_IN_EU(getLastValidEuPage(cb_eu_phy_addr));
//	PRINT_MSG_AND_NUM("\nhandleCopyBackSpecial() - offset_in_cb=",offset_in_cb);

	if(IS_PHY_ADDR_EMPTY(reserve_phy_addr)){
		/* if we have none, erase a possible replcament we started writing*/
		SEQ_VERIFY(!erasePossibleEUReplacement(org_phy_addr));
		reserve_phy_addr = allocReserveEU(flags, org_phy_addr);
		SEQ_VERIFY(!IS_PHY_ADDR_EMPTY(reserve_phy_addr));
//		PRINT_MSG_AND_NUM("\nhandleCopyBackSpecial() - no reserve address. allocated ",reserve_phy_addr);
	}
	else{
//		PRINT_MSG_AND_NUM("\nhandleCopyBackSpecial() - we have a reserve address ",reserve_phy_addr);
//		assert(0);
		/* we started writing back to the reserve EU
		 * find offsets in every one*/
		offset_in_org = CALC_OFFSET_IN_EU(getLastValidEuPage(reserve_phy_addr));
//		PRINT_MSG_AND_NUM(" offset_in_org=",offset_in_org);
		/* if copy back EU is programmed badly, or wasn't completed*/
		if(IS_ADDRESS_ERROR(offset_in_cb) || offset_in_org > offset_in_cb){
			if(nandErase(cb_eu_phy_addr))
				markEuAsBad(cb_eu_phy_addr);

			cb_eu_phy_addr = GET_COPYBACK_EU();

			/* maybe the previous one is now bad...*/
			nandErase(cb_eu_phy_addr);
			/* do copy back again*/
//			return copyBackSimple(cb_eu_phy_addr, reserve_phy_addr, offset_in_org, 0, org_phy_addr);
			return copyBackSimple(cb_eu_phy_addr, reserve_phy_addr+offset_in_org, 0, org_phy_addr);
		}
	}

//	return copyBackSimple(cb_eu_phy_addr, reserve_phy_addr, offset_in_cb, 1, org_phy_addr);
	return copyBackSimple(cb_eu_phy_addr, reserve_phy_addr+ offset_in_cb, 1, org_phy_addr);
}

/**
 *@brief
 * handle case of booting after crash during copy back.
 * search for the second valid reserve EU, which contains the copy back EU.
 * if it is empty then we have no copyback. otherwise, erase original EU and complete copy back
 *
 * Assumptions:
 * 1. reserve segments loacted
 * 2. every slot has at least 2 valid EU's
 * @param copyBackHandled indicator whther we indeed found a copy back to handle
 * @return 1 if a write/erase error occured. 0 if successful
 */
error_t handleCopyBack(bool_t *copyBackHandled){
	uint32_t cb_eu_phy_addr, org_phy_addr, offset_in_cb, offset_in_org;
	INIT_FLAGS_POINTER_TO_BUFFER(seq_flags, sequencing_buffer);

//	PRINT("\nhandleCopyBack() - starting");
	cb_eu_phy_addr = GET_COPYBACK_EU();
//	PRINT_MSG_AND_NUM("\nhandleCopyBack() - cb_eu_phy_addr=",cb_eu_phy_addr);
	assert(!IS_PHY_ADDR_EMPTY(cb_eu_phy_addr));

	SEQ_VERIFY(!nandReadPageTotal(sequencing_buffer, cb_eu_phy_addr));

	/* if no copyback - we're done*/
	if(GET_COPY_BACK_FLAG(seq_flags) == COPY_BACK_FLAG_FALSE){
//		PRINT("\nhandleCopyBack() - no cb eu");
		return 0;
	}

	*copyBackHandled = 1;

	/* there is a copy back eu - finish copy back */
//	PRINT_MSG_AND_HEX("\nhandleCopyBack() - slot id eu offset==",GET_SLOT_EU_OFFSET(seq_flags));
//	PRINT_MSG_AND_HEX("\nSLOT_MASK=",SLOT_MASK);

//	PRINT_MSG_AND_HEX("\nSLOT=",(((GET_SLOT_EU_OFFSET(seq_flags)) & SLOT_MASK) >> NAND_ADDRESS_EU_BITS_COUNT));
//	PRINT_MSG_AND_HEX("\nEU OFFSET=",((GET_SLOT_EU_OFFSET(seq_flags)) & EU_OFFSET_MASK));

	org_phy_addr = EXTRACT_ADDR_FROM_SLOT_ID_AND_EU_OFFSET(GET_SLOT_EU_OFFSET(seq_flags));

//	PRINT_MSG_AND_NUM("\nhandleCopyBack() - org_phy_addr=",org_phy_addr);
//	PRINT_MSG_AND_NUM("\nhandleCopyBack() - is org_phy_addr valid?=",nandCheckEuStatus(org_phy_addr));
	if(!seq_check_eu_status(org_phy_addr)){
		return handleCopyBackSpecial(org_phy_addr, cb_eu_phy_addr);
	}

	offset_in_org = CALC_OFFSET_IN_EU(getLastValidEuPage(org_phy_addr));
	offset_in_cb  = CALC_OFFSET_IN_EU(getLastValidEuPage(cb_eu_phy_addr));
//	PRINT("\nhandleCopyBack() - got offsets in copyback");
//	PRINT_MSG_AND_NUM(" offset_in_org=",offset_in_org);
//	PRINT_MSG_AND_NUM(" offset_in_cb=",offset_in_cb);

	/* if :
	 * a. copy back EU is programmed badly, or
	 * b. was not programmed entirely
	 * then we know first stage of copyback was not completed*/
	if(IS_ADDRESS_ERROR(offset_in_cb) || offset_in_org > offset_in_cb){
//		PRINT("\nhandleCopyBack() - 1st stage not completed. erase cb_eu_phy_addr");
		if(nandErase(cb_eu_phy_addr))
			markEuAsBad(cb_eu_phy_addr);

		cb_eu_phy_addr = GET_COPYBACK_EU();

		/* maybe the previous one is now bad...*/
		SEQ_VERIFY(!nandErase(cb_eu_phy_addr));
		/* do copy back again*/
//		PRINT("\nhandleCopyBack() - complete copyback");
		return copyBackSimple(cb_eu_phy_addr, org_phy_addr+offset_in_org, 0, SEQ_PHY_ADDRESS_EMPTY);
	}

	SEQ_VERIFY(!nandErase(org_phy_addr));
//	PRINT("\nhandleCopyBack() - first stage was completed. complete copyback");

	/* original EU is not erased, we crashed during second stage of copyback.
	 * the copy back EU is fine,simply complete copyback*/
	return copyBackSimple(cb_eu_phy_addr, org_phy_addr+ offset_in_cb, 1, SEQ_PHY_ADDRESS_EMPTY);
}

/**
 * @brief
 * handle case that the flash is empty.
 * - erase first EU
 * - allocate first segment
 * @param cp_write_p checkpoint writer to write the checkpoint
 * @return 0 if successful
 */
error_t handleFlashIsEmpty(checkpoint_writer cp_write_p){
	uint32_t last_valid;
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, 0);
	SET_SEGMENT_TO_SLOT(0,0);

	last_valid = 0;
	SET_RECLAIMED_OFFSET(0);
	SET_RECLAIMED_SEGMENT(0);

	if(!seq_check_eu_status(last_valid)){
		last_valid = allocReserveEU(flags, last_valid);
		assert(!IS_PHY_ADDR_EMPTY(last_valid));
		SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, last_valid);
		SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr, IS_EU_RESERVE_TRUE);
	}
	/* erase EU before writing*/
	SEQ_VERIFY(!nandErase(last_valid));
//	PRINT("\nhandleFlashIsEmpty() - first valid eu erase success");
	/* write segment header*/
	SEQ_VERIFY(!writeSegmentHeader(cp_write_p));
//	PRINT("\nhandleFlashIsEmpty() - first segment written success");

	INCREMENT_SEG_MAP_NSEGMENTS(seg_map_ptr);

	return 0;
}

/**
 * @brief
 * auxiliary, find the first empty slot, so we erase it's first empty EU (or a possible
 * reserve) in case we started writing an error on but failed
 */
uint32_t findFirstEmptySlot(){
	uint32_t i;

	for(i=0; i< SEQ_SEGMENTS_COUNT;i++){
		if(IS_SLOT_EMPTY(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,i)))
			return i;
	}

	return SEQ_NO_SEGMENT_NUM;
}

/**
 * @brief
 * find the first empty slot in flash
 * @return free slot id
 */
uint32_t findEmptySlot(){
	uint32_t slot_idx, seg_idx;

	for(slot_idx=0;slot_idx< SEQ_N_SLOTS_COUNT;slot_idx++){
		for(seg_idx=0;seg_idx<SEQ_N_SLOTS_COUNT;seg_idx++){
			if(GET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, seg_idx) == slot_idx)
				break;
		}

		/* if we found no segment for this slot*/
		if(seg_idx == SEQ_N_SLOTS_COUNT)
			return slot_idx;
	}

	return SEQ_PHY_ADDRESS_EMPTY;
}

/**
 * @brief
 * erase possible first eu of a slot we might have started writing it's header
 * @return 0 if successful, 1 if failed
 */
error_t erasePossibleNextFirstEU(){
	uint32_t phy_addr, slot_id;

	if(!ALL_SEGMENTS_ALLOCATED()){
		slot_id  = findFirstEmptySlot();
//		PRINT_MSG_AND_NUM("\nerasePossibleNextFirstEU() - not all slots allocated. free slot_id=",slot_id);
	}
	/* we don't care about possible next EU if we're in reclamation -
	 * we crashed right after writing last page in new generation, and we definitely didn't start
	 * writing a new header somewhere*/
	else if(!IS_STATE_RECLAMATION()){
		slot_id = findEmptySlot();
//		PRINT_MSG_AND_NUM("\nerasePossibleNextFirstEU() - all slots allocated. free slot_id=",slot_id);
		SEQ_VERIFY(!IS_SLOT_EMPTY(slot_id));
	}
	else{
//		PRINT("\nerasePossibleNextFirstEU() - state not reclamation, but all segments full. we can return safely");
		return 0;
	}
//
//	PRINT_MSG_AND_NUM("\nerasePossibleNextFirstEU() - slot_id=",slot_id);
//	PRINT_MSG_AND_NUM("\nerasePossibleNextFirstEU() - phy_addr =",phy_addr);
	phy_addr = CALC_ADDRESS(slot_id,0,0);
	if(seq_check_eu_status(phy_addr)){
//		PRINT("\nerasePossibleNextFirstEU() - nandCheckEuStatus() ok");
		SEQ_VERIFY(!nandErase((uint32_t)phy_addr));
	}
	else{
//		PRINT("\nerasePossibleNextFirstEU() - erasing erasePossibleEUReplacement()");
		/* it is also possible this is bad eu*/
		SEQ_VERIFY(!erasePossibleEUReplacement(phy_addr));
	}

	return 0;
}

void returnToPrevSeg(){
	/* decrement nsegments*/
	if(!IS_STATE_RECLAMATION()){
		SET_SEG_MAP_NSEGMENTS(seg_map_ptr,GET_SEG_MAP_NSEGMENTS(seg_map_ptr)-1);
	}

	SET_RECLAIMED_SEGMENT(GET_SEG_MAP_PREV_SEG(seg_map_ptr));
	SET_RECLAIMED_SEGMENT(SEQ_MAX_LOGICAL_OFFSET);
}

//void initSegMapReserveFields(){
//	SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, SEQ_PHY_ADDRESS_EMPTY);
//	SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr, IS_EU_RESERVE_FALSE);
//}

uint32_t calcOriginalSlotOffset(spare_area_flags *flags, uint32_t reserve_addr){
	/* if last valid is in reserve EU, we need to extract offset in original address*/
	/* calc offset of addressfrom slot_eu_flag*/
	nandReadPageFlags(flags,reserve_addr);
	return (CALC_OFFSET_IN_SLOT(EXTRACT_ADDR_FROM_SLOT_ID_AND_EU_OFFSET(GET_SLOT_EU_OFFSET(flags))))+
		   /* add offset in EU*/
	       CALC_OFFSET_IN_EU(reserve_addr);
}

/**
 * @brief
 * auxiliary to booting.
 * check possible cases of bad programs after the last valid page found -
 * 1. last valid is in eu middle (maybe reserve).
 * 2. last valid is in eu end - maybe next eu is bad, and we need to find the possible replacmenet
 *                              whose program failed.
 * 3. last valid is end of slot -
 *
 * @param last_valid last valid page in flash
 * @param cp_pages_size checkpoint size in pages (including header)
 * @return 0 if successful, 1 if an erase error occures, if no reserve address was found when expected
 */
error_t handleInvalidPrograms(uint32_t last_valid, uint32_t cp_pages_size){
	uint32_t slot_id, phy_addr;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_addr);

	init_logical_address(log_addr);

	/* we want to look at the status of the address after last valid.*/
	INCREMENT_RECLAIMED_OFFSET(seg_map_ptr);

	/* if last valid page is in the middle of an EU (i.e. not last page)
	 * and not outside the segment*/
	if(!IS_SEG_MAP_PAGE_OFFSET_OVERFLOWING() &&
		!IS_EU_START(GET_RECLAIMED_OFFSET())){
//		 && CALC_OFFSET_IN_EU(GET_RECLAIMED_OFFSET()) < NAND_PAGES_PER_ERASE_UNIT){
//	    PRINT("\nhandleInvalidPrograms() - case 1. seg map offset doesnt overflow and in middle of seg map");
//	    PRINT_MSG_AND_NUM(" rec seg=",GET_RECLAIMED_SEGMENT());
//	    PRINT_MSG_AND_NUM(" rec offset=",GET_RECLAIMED_OFFSET());
		/* copy back to clear the page after last valid */
		SEQ_VERIFY(!copyBackEU(GET_RECLAIMED_ADDRESS_PTR()));
//		assert(0);

//		PRINT_MSG_AND_NUM("\ncase 1.is rec offset below cp size? ",GET_RECLAIMED_OFFSET() < cp_pages_size);
		/* if offset is less than Total header size (cp+header page)
		 * then we haven't even completed writing the header
		 * erase it, and return to the previous segment*/
		if(GET_RECLAIMED_OFFSET() < cp_pages_size){
//			PRINT("\nhandleInvalidPrograms() - returning to prev seg");
			SEQ_VERIFY(!nandErase(last_valid));
			returnToPrevSeg();

			SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr, GET_SLOT(last_valid));

			/* check if last EU of prev segment is valid*/
//			if(nandCheckEuStatus(logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR()))){
//				initSegMapReserveFields();
//			}
//			else{
//				SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr, IS_EU_RESERVE_TRUE);
//				phy_addr = logicalAddressToReservePhysical(GET_RECLAIMED_ADDRESS_PTR(),flags, last_valid);
//				SEQ_VERIFY(IS_PHYSICAL_ADDRESS_EMPTY(phy_addr));
//				SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, phy_addr);
//			}
		}
		/* reclaimed offset does not change.
		 * we should test if it is in a reserve EU. findNextFreePage() assumes
		 * later on that we know this.*/
		else{
//			PRINT_MSG_AND_NUM("\ncase 1.is last valid reserve? ",isAddressReserve(last_valid));
			/* we can check if last_valid is reserve because it is
			 * in the same EU as reclaimed address (in this case)*/
			if(isAddressReserve(last_valid)){
				/* mark reserve EU*/
//				PRINT("\nsetting reserve addr in seg map ");
				SET_SEG_MAP_RESERVE_EU_ADDR(seg_map_ptr, CALC_EU_START(last_valid));
				SET_SEG_MAP_IS_EU_RESERVE(seg_map_ptr, IS_EU_RESERVE_TRUE);
			}
		}
	}
	/* last valid page is at end of an EU. */
	else if(!IS_SEG_MAP_PAGE_OFFSET_OVERFLOWING()){
//		PRINT("\nhandleInvalidPrograms() - case 2. seg map offset doesnt overflow");
		phy_addr = logicalAddressToPhysical(GET_RECLAIMED_ADDRESS_PTR());
		/* NOTICE - if the next EU is invalid, then we might have failed in
	 	* the middle of allocating a reserve one.*/
		if(!IS_PHY_ADDR_EMPTY(phy_addr)){
			/* erase the address*/
			if(nandErase(phy_addr)){
				markEuAsBad(phy_addr);
			}
		}
		else{
			SEQ_VERIFY(!erasePossibleEUReplacement(phy_addr));
			/* delete all reserve EU's that might have been allocated as a replacement*/
		}

//		DECREMENT_RECLAIMED_OFFSET();
	}
	/* we overflowed the reclaimed segment*/
	else{
//		PRINT("\nhandleInvalidPrograms() - case 3. seg map offset overflows");
		if(IS_STATE_RECLAMATION()){
//			PRINT("\nhandleInvalidPrograms() - overflows, reclamation");
			/* truncate old generation*/
			slot_id = GET_RECLAIMED_SEGMENT_SLOT();
			SEQ_VERIFY(truncateSlot(slot_id, 0));

			SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, GET_RECLAIMED_SEGMENT(), GET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr));
			SET_SEG_MAP_NEW_SLOT_ID(seg_map_ptr,slot_id);
		}
		else{
//			PRINT("\nhandleInvalidPrograms() - overflows, allocation");
			SEQ_VERIFY(!erasePossibleNextFirstEU());
		}

//		DECREMENT_RECLAIMED_OFFSET();
	}

//	PRINT_MSG_AND_NUM("\nhandleInvalidPrograms() - finisghed. reclaimed offset=",GET_RECLAIMED_OFFSET());

	return 0;
}
