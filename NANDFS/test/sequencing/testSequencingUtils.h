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

#ifndef TESTSEQUENCINGUTILS_H_
#define TESTSEQUENCINGUTILS_H_

#include <system.h>
#include <src/sequencing/sequencing.h>

error_t checkpointWriter(bool_t isPartOfHeader);

/**
 * @brief
 * initialize read buffer
 * @param flags the read buffer to initialize
 */
void init_flags(spare_area_flags *flags);

void init_flash(void);

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
				 uint32_t reserve_eu_addr);

/**
 * @brief
 * auxiliary, mark reserve segments headers
 */
void mark_reserve_segment_headers(void);

/**
 * @brief
 * imitate reboot by initializing obs and seg map
 */
void mock_reboot(void);

/**
 * @brief
 * auxiliary function - find the first not occupied reserve unit, and mark it
 * as a replacement for the EU of orig_phy_addr
 * @param orig_phy_addr some physical address in one of the data slots
 * @return the physical address of the newly allocated reserve eu 
 * if no reserve EU exists return SEQ_PHY_ADDRESS_EMPTY
 */
uint32_t write_to_reserve_eu(uint32_t orig_phy_addr);

/**
 * @brief
 * auxiliary function - find the first not occupied reserve unit, write data to all it's pagesand mark it
 * as a replacement for the EU of orig_phy_addr
 * @param orig_phy_addr some physical address in one of the data slots
 * @param buf data buffer
 * @return the physical address of the newly allocated reserve eu. if a program error occured SEQ_PHY_ADDRESS_EMPTY is returned
 */
uint32_t write_data_to_reserve_eu(uint32_t orig_phy_addr,uint8_t *buf);

/**
 * auxiliary. get data of expected reserve segment, for some given address
 * and compare resuilt of looking for the reserve and the expected
 */
error_t compare_expected_and_reserve(uint32_t seg_id, uint32_t eu_offset, uint32_t page_offset, uint32_t expected_reserve);

/**
 * auxiliary. get data of expected reserve segment, for some given address
 * and compare resuilt of looking for the reserve and the expected
 */
error_t compare_expected_and_reserve(uint32_t seg_id, uint32_t eu_offset, uint32_t page_offset, uint32_t expected_reserve);

/**
 * @brief
 * auxiliary function - get level of segment seg_id from obsolete counters map obs_map
 * @param seg_id segment id
 * @param obs_map pointer to a segment map
 * @return counter level
 */
uint8_t get_counter_level_by_map(uint32_t seg_id, obs_pages_per_seg_counters *obs_map);

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
error_t read_cp_to_buffers(uint8_t *fs_data, uint32_t fs_data_size, segment_map *cp_seg_map_ptr, obs_pages_per_seg_counters *cp_obs_map_ptr);

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
					   uint32_t nSegments);

/**
 * @brief
 * @param level obs counter level to set 
 * set obsolete counters to a given value
 */
void set_obs_counters(uint8_t level);

/**
 * @brief
 * set obsolete counters to a given value
 * @param level expected obs counter level
 * @return 1 if verified, 0 otherwise
 */
error_t verify_obs_counters(uint8_t level);

/**
 * @brief
 * verify there are no copy back eus
 * @return 1 if none, 0 otherwise
 */
bool_t verify_no_cb_eu(void);

/**
 * @brief
 * auxiliary. check if any copy back eu's remain in flash
 * @return 1 if there is a reserve EU marked copy back, 0 if none
 */
error_t isThereCopyBackEU(void);

/* re-introduce reserve segments*/
void simpleRemarkReserveSegments(void);

/**
 * @brief
 * auxiliary. verify a page is empty of any data
 * @param log_address logical address of a page
 * @return 1 if empty, 0 if not
 */
error_t verifyPageIsEmpty(logical_addr_t log_address);

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
error_t writeMockPage(logical_addr_t log_addr, logical_addr_t prev_log_addr, uint8_t vots);

/**
 * @brief
 * - write mock VOTs
 * - write data
 * - write mock VOTs 
 * @param log_addr logical address to use in functions
 * @return 1 if all allocations were successful. 0 otherwise
 */
error_t writeMockVOTsPage(logical_addr_t log_addr, logical_addr_t prev_log_addr);

/**
 * @brief
 * write mock data page filled with byte
 */
error_t writeMockDataPage(logical_addr_t log_addr, logical_addr_t prev_log_addr);

/**
 * @brief
 * mark data segment Sequentially
 */
void markDataSegementsSequentially(void);

/**
 * @brief
 * verify all pages in the EU at address eu_start_phy_addr are filled with byte
 * @return 1 if successful, 0 if not
 */
error_t verifyEUWithByte(uint32_t eu_start_phy_addr, uint8_t byte);

/**
 *@brief
 * find the segment this slot belongs to
 * @param slot_id physical slot id
 * @return segment number.
 */
uint32_t find_seg_for_slot(uint32_t slot_id);

/**
 * @brief
 * check if a slot is completely erased
 * @param slot_id physical slot id
 * @return 1 if yes, 0 otherwise
 */
error_t verifySlotIsErased(uint32_t slot_id);

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
error_t replaceReserveSlotWithDataSlot(uint32_t reserve_slot_id, uint32_t data_slot_id);

/**
 * @brief
 * perform wear leveling only half way through for random_slot until page max_offset
 * @param random_slot the slot physical address we wear level
 * @param max_offset maximum offset until which we wear level in the slot
 * @return 0 if successful. return 1 if a read/write/erase error has occured
 */
error_t performMockWearLeveling(uint32_t random_slot, uint32_t max_offset);

/**
 * @brief
 * auxiliary function. find the first valid EU in a reserve slot - it contains it's header
 * @param reserve_slot_id physical address of a reserve slot
 * @return 1 successful, 0 otherwise
 */
uint32_t readReserveSegmentHeader(uint32_t reserve_slot_id);

/**
 * @brief
 * verify segment headet data is as expeceted
 * @param seg_id segment id
 * @param seg_type expected segmnet type
 * @return 0 if failed, 1 if successful
 */
error_t verify_seg_header(uint32_t seg_id, uint32_t seg_type);

/**
 * @brief
 * write simple segment headers from
 * @return 0 if successful, 1 in case of segment write error 
 */
error_t writeSimpleSegmentHeaders(uint32_t first_seg_id);

#endif /*TESTSEQUENCINGUTILS_H_*/
