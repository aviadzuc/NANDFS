/** @file sequencingUtils.h
 * Header File for sequencing layer auxiliary functions, definitions and prototypes
 */
#ifndef SEQUENCINGUTILS_H_
#define SEQUENCINGUTILS_H_

#include <system.h>
#include <src/sequencing/sequencing.h>

#define init_buf(BUF)    				init_struct(BUF, NAND_TOTAL_SIZE)
#define initFlags(FLAGS) 				init_struct(FLAGS, NAND_SPARE_SIZE)
#define init_logical_address(LOG_ADDR)  init_struct(LOG_ADDR, SEQ_LOG_ADDR_SIZE)

#define copyPrevAddressToFlags(FLAGS, PREV_LOG_ADDR) copyTruncatedLogicalAddress((logical_addr_t)(&((FLAGS)->bytes[PREV_BLOCK_PTR_LOCATION])), PREV_LOG_ADDR)
#define extract_prev_address(FLAGS, PREV_LOG_ADDR) 	 copyTruncatedLogicalAddress(PREV_LOG_ADDR,(logical_addr_t)(&((FLAGS)->bytes[PREV_BLOCK_PTR_LOCATION])))	

void setLogicalOffset(logical_addr_t log_addr, uint32_t offset);

void setLogicalSegment(logical_addr_t log_addr, uint32_t seg_num);
/**  
 * @brief
 * Reserve EU search:
 * EU is bad. search for the page in a reserve EU
 * iterate all reserve segments until we find the reserve EU
 * (reserve segments are always the last) 
 * @param flags pointer to flags struct to be used in the function
 * @param phy_addr the physical address we search for replacement
 * @return reserve physical address if successful, SEQ_PHY_ADDRESS_EMPTY otherwise
 */
uint32_t logicalAddressToReservePhysical(spare_area_flags *flags, uint32_t phy_addr);

/**  
 * @brief
 * translate logical address to physical.
 * - check if address is in the reclaimed slot
 * - check if it is in a reserve eu
 * - if in reserve eu call logicalAddressToReservePhysical
 * @return reserve physical address if successful, SEQ_PHY_ADDRESS_EMPTY otherwise
 */
uint32_t logicalAddressToPhysical(logical_addr_t logical_addr);

/**
 * @brief
 * check if a page is already marked as obsolete
 * @return 1 if yes, 0 if no
 */
error_t is_page_marked_obsolete(uint32_t phy_addr);

uint32_t get_counter_level(uint32_t seg_id);

/**
 * @brief
 * update the level of the obsolete counter of segment seg_id
 * @param level current level of segment
 * @param seg_id segment id
 */
void change_counter_level(uint32_t level, uint32_t seg_id);

/**
 * @brief
 * increment probabailsticaly obsolete counter of segment seg_id
 * @param seg_id segment id
 */ 
void increment_counter(uint32_t seg_id);

/**
 * @brief
 * initialize a buffer in a given size
 * @param buf buffer to initialize
 * @param size buffer size
 */
void init_struct(void *buf, uint32_t size);

/**
 * @brief
 * auxiliary, copy logical address details
 * @param to target logical address
 * @param from original logical address
 */
void copyLogicalAddress(logical_addr_t to, logical_addr_t from);

/**
 * @brief
 * auxiliary, copy logical address details, without last byte (marking hole)
 * @param to target logical address
 * @param from original logical address
 */
void copyTruncatedLogicalAddress(logical_addr_t to, logical_addr_t from);

/**
 * @brief
 * auxiliary function for allocAndWriteBlock - write a block of data to the next physical address,
 * and increment logical offset. if we are not given a physical address, then we can simply use 
 * the next (incremeneted) physical address
 * 
 * @param log_addr logical address to which the block is written to
 * @param data buffer
 * @param phy_addr physical address to write data. if 0, use the one from seg_map
 * @return @return if successful logical address the data was written to. if program fails returns NULL
 */ 
void writeBlock(logical_addr_t log_addr, void* data, uint32_t phy_addr);

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
void set_cp_page_flag(spare_area_flags *flags, uint32_t cp_page_idx, uint32_t cp_page_idx_max);

/**
 * @brief
 * auxiliary, delete current segment until its beginning
 * @return 0 if successful. if an erase fails return 1
 */
error_t truncateCurrentSegment(void);

/**
 * @brief
 * auxiliary. find a reserve erase unit to replace EU of address org_phy_addr
 * @param org_phy_addr address whose EU we replace
 * @return address of reserve EU. if none is found return SEQ_PHY_ADDRESS_EMPTY
 */
uint32_t findFreeReserveEU(uint32_t org_phy_addr);

/**
 * @brief
 * auxiliary, does the actual copy back
 * 
 * Assumptions
 * 1. cb_eu_phy_addr is empty and valid
 * 2. org_eu_phy_addr is valid
 * @param cb_eu_phy_addr physical address of copy back eu
 * @param org_eu_phy_addr physical address of eu we want to copy back + maximum offset until which we will copy (<max_offset, not itself)   
 * @param isOrgErased indicator if the original eu is erased. if so no need to copy to copy back eu
 * @param org_eu original of org_eu_phy_addr (if it is reserve). otherwise = SEQ_PHY_ADDRESS_EMPTY 
 * @return 0 if successful, 1 if a program error occured
 */
error_t copyBackSimple(uint32_t cb_eu_phy_addr, uint32_t org_eu_phy_addr, bool_t isOrgErased, uint32_t org_eu);

/**
 * @brief
 * auxiliary. copy back one page. mark it as copy back in flags if necessary.
 * if we copy back a reserve EU (org_from != SEQ_PHY_ADDRESS_EMPTY), then we 
 * must maintain slot_eu_flag of the original physical address and not change it,
 * even after copy back
 * 
 * @param to address to copy to
 * @param from address to copy from
 * @param org_from indicate whther from is a reserve EU. if not org_from is SEQ_PHY_ADDRESS_EMPTY
 * @withCopyBackFlags should we mark copy back flag
 */
error_t copyBackPage(uint32_t to, uint32_t from, bool_t withCopyBackFlags, uint32_t org_from);

/**
 * @brief
 * - copy the EU log_addr is part of to a reserve EU
 * - erase the original
 * - copy back only until the offset in log_addr
 * @param log_addr logical address whose matching physical address EU should be copyback-ed
 * @return 0 if the copyback was successful, 1 if an error occured when copying
 */
error_t copyBackEU(logical_addr_t log_addr);

/**
 * @brief
 * auxiliary. iterate while we still haven't reached segment start
 * look for the end of a checkpoint
 * NOTICE - we don't look for CP_LOCATION_FIRST, since finding it doesn't mean we have
 * CP_LOCATION_LAST after it somewhere 
 * @param flags spare area flags buffer
 * @param log_addr logical address used to iterate the segment
 * @param nBytes file system layer data structures (for checkpoint) size
 * @return 0 if successful, 1 if none found
 */
error_t findCheckpointInSegment(spare_area_flags *flags, logical_addr_t log_addr, bool_t cp_more_than_one_page);

/**
 * @brief
 * auxiliary, iterate backwards in current segment until you find the first page marked
 * as checkpoint start.
 * Assumptions:
 * 1. we already found the last part of a checkpoint in the segment, therefore there MUST be a first part
 */
error_t findCheckpointStart(spare_area_flags *flags, logical_addr_t log_addr);

/**
 * @brief
 * find start of checkpoint and read it to memory structures
 * @param data file system data pointer
 * @param nBytes file system data size
 * @param flags spare area buffer
 * @param temporary logical address index
 * @return 0 if successful
 */
error_t readCheckpointToMemory(void *data, uint32_t nBytes, spare_area_flags *flags, logical_addr_t log_addr);

/**
 * @brief
 * auxiliary, copy prev_log_addr to the matching field in flags 
 * @param flags pointer to page buffer spare area
 * @param prev_log_addr logical address to copy to flags
 */
//void copyPrevAddressToFlags(spare_area_flags *flags, logical_addr_t prev_log_addr);

/**
 * @brief
 * auxiliary to performWearLeveling(). copy back one page. mark it as copy back in flags if necessary
 * @param to address to copy to
 * @param from address to copy from
 * @param isSlotReserve is he slot we are copying a reserve slot
 * @return 0 if successful. if a program error occured return 1
 */
error_t copyPage(uint32_t to, uint32_t from, bool_t isToReserve, uint32_t originalToAddress, bool_t isSlotReserve);

///**
// * @brief 
// * initialize all flags.
// * @param flags opinter to spare area buffer
// */
//void initFlags(spare_area_flags *flags);

/**
 * @brief 
 * extract previous logical address field from flags and copy it to prev
 * @param flags pointer to page flags
 * @param prev logical address pointer
 */
//void extract_prev_address(spare_area_flags *flags, logical_addr_t prev);

/**
 * @brief 
 * auxiliary, check if all segments are amrked as real full
 */
bool_t is_flash_full(void);

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
uint32_t allocReserveEU(spare_area_flags *flags, uint32_t phy_addr);

/**
 * @brief
 * auxiliary. check if the current page in the seg map is considered valid for copying during reclamation
 * a page is copied only if it is not obsolete, and not part of a checkpoint
 * @param phy_addr the page to check whther to copy
 * @return 1 if the current paeg should be copied, 0 if not
 */
error_t is_page_to_copy(spare_area_flags *copy_flags, uint32_t phy_addr);

/**
 * @brief
 * auxiliary function for allocAndWriteBlock - copy a page from old generation 
 * to new generation. We assume both of them exist, and pointed by the segment map
 * @param phy_addr address to write data to
 * @param org_phy_addr the original physical address which ohy_addr replaces. if phy_addr is not reserve, org_phy_addr is 0
 * @return 1 if an error occured during write/read. return 0 if successful
 */
error_t copyValidBlock(uint32_t phy_addr, uint32_t org_phy_addr);

/**
 * @brief
 * copy a random slot to the current new slot in the segment map.
 * this is to avoid slots seldomly being reclaimed.
 * 
 * Assumptions - 
 * 1. we are in reclaamtion state
 * @return 0 if successful, 1 if a write error occured
 */
error_t performWearLeveling(void);

/**
 * @brief
 * erase slot from last EU to start
 * @param slot_id the slot id
 * @return 1 if an error occured during the write, 0 if successful
 * 
 */
error_t truncateSlot(uint32_t slot_id, bool_t isSlotReserve);

/**
 * @brief
 * iterate all levels of obsolete counters starting from obs_counter_level backwards
 * and find a segment with at least that level of obsolete pages, other than last_segment_reclaimed.
 *
 * @return 0 if successful. 0 if no segment was found (all full)
 */
error_t findSegmentToReclaim();

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
error_t findNextFreePage(bool_t *cpWritten, checkpoint_writer cp_write_p, uint32_t ok_error_code);

/**
 * @brief
 * auxiliart to commit().
 * set is commit flag according to page index in the checkpoint
 */
void setIsCommit(bool_t *isCommit, bool_t isCommitLastPage);

/**
 * @brief 
 * write header for a new allocated segment in seg map.
 * @return return 0 if successful. return 1 if an error occured in writing. 
 * return 1 if an error occured during commit
 */
error_t writeSegmentHeader(checkpoint_writer cp_write_p);
	
/**
 * @brief
 * write a header for a reserve segment in addres phy_addr
 * @param phy_addr first valid address in the segment
 * @return 0 if sucessful 
 */
error_t writeReserveSegmentHeader(uint32_t phy_addr);
	
/**
 * @brief
 * auxiliary function to allocAndWriteBlock(). find the next segment to reclaim/allocate.
 * 
 * @flags spare area buffer
 * @cpWritten boolean indicator whther a checkpoint is written during allocation.
 * @cp_write_p checkpoint writer function
 */	
error_t moveToNextSegment(spare_area_flags *flags_ptr, bool_t *cpWritten, checkpoint_writer cp_write_p);

uint32_t calc_target_phy_addr(void);

//void init_logical_address(logical_addr_t log_address);
//
void init_seg_map(void);

void init_obs_counters(void);

uint32_t get_first_valid_eu_addr(uint32_t slot_id);

/**
 * @brief
 * auxiliary. copy back one page. mark it as copy back in flags if necessary
 * @param to address to copy to
 * @param from address to copy from
 * @withCopyBackFlags should we mark copy back flag
 * @return 0 if successful. if a program error occured return 1
 */
error_t nandProgramTotalPage(void *buf, uint32_t phy_addr);

/**
 * @brief
 * auxiliary, check if a given physical address is in a slot allocated for reserve addresses
 * 
 * @param phy_addr a physical address in flash
 * @return 1 if the address is in a reserve slot, 0 otherwise
 */
bool_t isAddressReserve(uint32_t phy_addr);

/**
 * @brief
 * find the valid EU num eu_location in a given slot
 * @param slot_id physical slot number
 * @param eu_location requested eu location in the slot
 * @return address of first valid EU in slot. if all slots are bad return SEQ_PHY_ADDRESS_EMPTY
 */
uint32_t get_valid_eu_addr_in_location(uint32_t slot_id, uint32_t eu_location);

/**
 * @brief 
 * auxiliary to booting.
 * find the first EU that would have been allocated as a replacement for phy_addr
 * and erase it, in case we started programming on it but failed.
 * 
 * @return 0 if successful, 1 otherwise 
 */
error_t erasePossibleEUReplacement(uint32_t phy_addr);


/**
 * @brief
 * count segments of a given type
 * @param isReserve shoudl we count reserve segmnets
 * @return number of segmnets from requested type
 */
uint32_t countSegments(bool_t isReserve);

/**
 * @brief
 * find the last valid page in a given EU
 * 
 * Assumptions:
 * 1. first page is valid and programmed
 * @param eu_start_addr eu we're looking at physical address
 * @param lastValid pointer where to save the last valid page
 */
error_t findLastValidPageInEU(uint32_t eu_start_addr,uint32_t *lastValid);

/**
 * @brief
 * auxiliary to findLastValidPage().
 * check if a given eu (or it's reserve) is valid and used
 * 
 * @param end_eu pointer to eu
 * @return 1 if used and valid, 0 otherwise
 */
bool_t isUsedEU(spare_area_flags *flags, uint32_t *eu_phy_addr);

/**
 * @brief
 * find the last valid page in slot #slot_id, using binary search. store it in lastValid.
 * if none is found SEQ_PHY_ADDRESS_EMPTY is stoed in lastValid
 * @param lastValid pointer to address variable to save address to
 * @param slot_id physical slot id  
 * @return 0 if successful. 1 in case of an error
 */
error_t  findLastValidPageInSlot(uint32_t *lastValid, uint32_t slot_id);

/**
 * @brief
 * find last valid page in an EU
 * 
 * Assumptions:
 * 1. a bad read is the resuly of a bad program. page before it is the last valid (last proper program)
 * @param eu_phy_addr physical address of EU to check
 * @return SEQ_PHY_ADDRESS_EMPTY if only written page is the first and it is bad. otherwise the physical address
 */
uint32_t getLastValidEuPage(uint32_t eu_phy_addr);

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
error_t handleDoubleReserveSlot(uint32_t seg_id, uint32_t slot_id1, uint32_t slot_id2);

/**
 * @brief
 * complete writing of all reserve segment headers
 */ 
error_t handleReservesAreEmpty(void);

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
 * @return 0 if successful
 */
error_t handleReserveSegments(bool_t *badFirstEU, uint32_t *doublyAllocatedSegment, uint32_t *secondSlot);

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
bool_t verifyCompleteSegmentHeader(uint32_t header_phy_addr, uint32_t nBytes);

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
error_t markSegmentInSegMap(page_area_flags *page_header_ptr, uint32_t header_phy_addr, uint32_t slot_id, uint32_t nBytes);

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
error_t handleRegularSegments(uint32_t nBytes);

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
error_t handleCopyBackSpecial(uint32_t org_phy_addr, uint32_t cb_eu_phy_addr);

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
error_t handleCopyBack(bool_t *copyBackHandled);

/**
 * @brief
 * handle case that the flash is empty. 
 * - erase first EU
 * - allocate first segment
 * @param cp_write_p checkpoint writer to write the checkpoint
 * @return 0 if successful
 */
error_t handleFlashIsEmpty(checkpoint_writer cp_write_p);

/**
 * @brief
 * auxiliary, find the first empty slot, so we erase it's first empty EU (or a possible
 * reserve) in case we started writing an error on but failed
 */
uint32_t findFirstEmptySlot(void);

/**
 * @brief
 * find the first empty slot in flash
 * @return free slot id
 */
uint32_t findEmptySlot(void);
 
/**
 * @brief
 * erase possible first eu of a slot we might have started writing it's header
 * @return 0 if successful, 1 if failed
 */
error_t erasePossibleNextFirstEU(void);

void returnToPrevSeg(void);

//void initSegMapReserveFields();

uint32_t calcOriginalSlotOffset(spare_area_flags *flags, uint32_t reserve_addr);

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
error_t handleInvalidPrograms(uint32_t last_valid, uint32_t cp_pages_size);

#endif /*SEQUENCINGUTILS_H_*/
