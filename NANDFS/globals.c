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

/** @file globals.c
 *  global variables used in the file system
 */
#include <system.h>
#include <peripherals/nand.h>
#include <src/sequencing/sequencing.h>
#include <src/sequencing/lfsr.h>
#include <src/fs/transactions.h>
#include <src/fs/fs.h>
#include <src/fs/cache.h>

/* @brief
 * lfsr state for obsolete counter and segment number random bit generator*/
uint32_t lfsr_state = LFSR_STATE_INITIAL;

/** @brief
 * the actual segment map
 */
segment_map seg_map;

/**@brief
 *  segment obsolete counters
 */
obs_pages_per_seg_counters obs_counters_map;

/** @brief
 * a buffer for copying pages from one segment to another
 */
uint8_t sequencing_buffer[NAND_TOTAL_SIZE];

/** @brief
 * the last byte of the spare area with the obsolete flag raised
 */
uint8_t obsolete_byte = 0x7f;

#ifdef Debug
/* mock file system data*/
	uint8_t fs_data[MOCK_FS_DATA_SIZE];
#endif
#ifdef SIM
	unsigned char main_area_writes[NAND_PAGE_COUNT];
	unsigned char spare_writes[NAND_PAGE_COUNT];
#endif

#ifdef Profiling
/* mock file system data*/
	uint8_t fs_data[MOCK_FS_DATA_SIZE];
#endif
segment_map *seg_map_ptr = &seg_map;
obs_pages_per_seg_counters *obs_counters_map_ptr = &obs_counters_map;

/**
 * @brief
 * vnodes array
 */
vnode_t vnodes[FS_MAX_VNODES];

/**
 * @brief
 * open file entries array.
 * a file descriptor is an ffset in the array
 */
open_file_t open_files[FS_MAX_OPEN_FILES];

/**
 * @brief
 * file system basic data
 */
filesystem_t fs;

fs_t fs_ptr = &fs;

process_t process;
process_t *currentProcess = &process;

uint8_t fs_buffer[FS_BLOCK_SIZE];

NANDFS_DIR dirstreams[FS_MAX_OPEN_DIRESTREAMS];

/**
 * @brief
 * transactions array
 */
transaction_t transactions[FS_MAX_N_TRANSACTIONS];

cache_lru_q lru_ptr;
