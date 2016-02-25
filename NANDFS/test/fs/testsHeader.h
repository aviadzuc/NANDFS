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

#ifndef FSTESTSHEADER_H_
#define FSTESTSHEADER_H_

#include <system.h>
#include <src/sequencing/sequencingUtils.h>
#include <src/sequencing/sequencing.h>
#include <src/sequencing/lfsr.h>
#include <src/fs/fsUtils.h>
#include <src/fs/fs.h>
#include <src/fs/transactions.h>
#include <lpc2000/uart.h>
#include <lpc2000/clocks.h>
#include <lpc2000/busywait.h>
#include <utils/print.h>
#include <utils/memlib.h>
#include <utils/string_lib.h>
#include <test/macroTest.h>
#include <test/testUtils.h>
#include <test/fs/testFsUtils.h>

/* pointers to global data structures */
extern transaction_t transactions[FS_MAX_N_TRANSACTIONS];
extern vnode_t vnodes[FS_MAX_VNODES];
extern open_file_t open_files[FS_MAX_OPEN_FILES];
extern fs_t fs_ptr;
extern uint8_t fs_buffer[FS_BLOCK_SIZE];
extern obs_pages_per_seg_counters *obs_counters_map_ptr;
extern segment_map *seg_map_ptr;
extern fs_dirstream dirstreams[FS_MAX_OPEN_DIRESTREAMS];
extern cache_lru_q lru_ptr;

#endif /*FSTESTSHEADER_H_*/

