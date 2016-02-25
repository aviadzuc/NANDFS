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

/** @file testUtils.c
 *  various common utility functions for tests
 */

#include <test/macroTest.h>
#include <test/testUtils.h>
#include <src/fs/fs.h>

#define MOCK_BAD_EU_BYTE 0xee

/**
 * verify eu status. read phy_addr of an eu, and perform eu validiy check
 * return 1 if eu is ok, 0 otherwise
 */
error_t nandCheckMockBadEuStatus(uint32_t phy_addr){
	uint8_t spare_buf[NAND_SPARE_SIZE];

//	printf("\nnandCheckEuStatus() - phy_addr=%x", phy_addr);
//	printf("\nread first apge spare (%u)", phy_addr & NAND_EU_MASK);
	nandReadPageSpare(spare_buf, phy_addr & NAND_EU_MASK, 0, NAND_SPARE_SIZE);
//	printf("\nverify it");
	/* verify the eu isn't bad
	 * check bad eu flag in first two pages of the EU (as required by the datasheet)*/
	return (spare_buf[NAND_BAD_EU_FLAG_BYTE_NUM-1] == MOCK_BAD_EU_BYTE);
}

error_t markEuAsMockBad(uint32_t page_addr){
	uint8_t bad_byte = MOCK_BAD_EU_BYTE;
//	PRINT_MSG_AND_NUM("\nmarkEuAsMockBad() - marking eu as bad in address ",page_addr);
	// mark eu as bad  (if not already marked)
	if(!nandCheckEuStatus(page_addr)){
//		PRINT("\nmarkEuAsBad() - EU is already bad");
		return 1;
	}

//	PRINT_MSG_AND_NUM("\nmarkEuAsMockBad() - mark page as bad ",page_addr & NAND_EU_MASK);
	if(nandProgramPageC(&bad_byte, page_addr & NAND_EU_MASK, NAND_BAD_EU_FLAG_BYTE_NUM-1, 1)){
		return 0;
	}
//	PRINT("\nmarkEuAsMockBad() - EU is marked as bad");
	return 1;
}

#ifdef ARM
#define nandBruteErase(PAGE_ADDR) nandErase(PAGE_ADDR)
#endif

void init_flash(){
	uint32_t i, j;

//	PRINT("\ninit_flash() - starting");
//	PRINT_MSG_AND_NUM("\nSEQ_PAGES_PER_SLOT=", SEQ_PAGES_PER_SLOT);
//	PRINT_MSG_AND_NUM("\nSEQ_N_SLOTS_COUNT=", SEQ_N_SLOTS_COUNT);

	for(i=0; i<SEQ_N_SLOTS_COUNT; i++){
//		PRINT_MSG_AND_NUM("\ni=", i);
		/* erase eu's one by one from the segment */
		j = i * SEQ_PAGES_PER_SLOT;
//		PRINT_MSG_AND_NUM("\nstart erasing from ", j);
		for(; j< (i+1)* SEQ_PAGES_PER_SLOT; j+=NAND_PAGES_PER_ERASE_UNIT){
//			// if we encounter the first valid EU
//			if(nandCheckEuStatus(j)){
				/* erase it */
//				PRINT_MSG_AND_NUM("\nerase ", j);
				nandErase(j);

				/* if the EU is marked bad as part of a test (indicated by MOCK_BAD_EU_BYTE)
				 * erase it anyway*/
				if(nandCheckMockBadEuStatus(j)){
					nandBruteErase(j);
				}
//				PRINT_MSG_AND_NUM("\ninit_flash() erased");
//			}
		}
	}
}

void printBlock(uint8_t *buf){
	int32_t i;

	for(i=0; i < FS_BLOCK_SIZE; i++){
		if(buf[i] == 0xff)
			continue;

		PRINT_MSG_AND_NUM("\n", i);
		PRINT_MSG_AND_HEX(". ", buf[i]);
	}
}

void printSeqBlock(uint8_t *buf){
	int32_t i;

	for(i=0; i < NAND_TOTAL_SIZE; i++){
		if(buf[i] == 0xff)
			continue;

		PRINT_MSG_AND_NUM("\n", i);
		PRINT_MSG_AND_HEX(". ", buf[i]);
	}
}

/**
 * return 1 if buffers are exact, 0 otherwise
 */
bool_t compare_bufs(void *buf1, void *buf2, int32_t len){
	int32_t i;

	for(i=0; i<len;i++){
		if(!COMPARE(CAST_TO_UINT8(buf1)[i], CAST_TO_UINT8(buf2)[i])){
			L("verification failure. byte #%d: buf1[%d]=%x buf2[%d]=%x", i, i, CAST_TO_UINT8(buf1)[i], i, CAST_TO_UINT8(buf2)[i]);
			return 0;
		}
	}

	return 1;
}
