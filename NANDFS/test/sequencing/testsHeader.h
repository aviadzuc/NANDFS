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

#ifndef SEQTESTSHEADER_H_
#define SEQTESTSHEADER_H_

#include <system.h>
#include <test/sequencing/testSequencingUtils.h>
#include <src/sequencing/sequencingUtils.h>
#include <src/sequencing/sequencing.h>
#include <src/sequencing/lfsr.h>
#include <src/fs/fs.h>
#include <lpc2000/uart.h>
#include <lpc2000/clocks.h>
#include <lpc2000/busywait.h>
#include <utils/print.h>
#include <utils/memlib.h>
#include <test/macroTest.h>
#include <utils/string_lib.h>
#include <src/sequencing/lfsr.h>
#include <test/testUtils.h>
 
extern segment_map *seg_map_ptr; 
extern obs_pages_per_seg_counters *obs_counters_map_ptr;
extern uint32_t lfsr_state;

#ifdef Debug
extern uint8_t fs_data[MOCK_FS_DATA_SIZE];
#endif

#ifdef SIM	
extern unsigned char main_area_writes[NAND_PAGE_COUNT];	
extern unsigned char spare_writes[NAND_PAGE_COUNT];
#endif

#endif /*SEQTESTSHEADER_H_*/
