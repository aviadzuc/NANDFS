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

#ifndef NAND_H_
#define NAND_H_

#include <system.h>
#include <utils/yaffs_ecc.h>

/* unique chip features*/
#ifndef LARGE_PAGE
#define POWER_NAND_PAGES_PER_ERASE_UNIT 5   /* 32 */
#define POWER_NAND_PAGE_SIZE		    9  /* 512 */
#define POWER_NAND_SPARE_SIZE		    4   /* 16 */
#define NAND_EU_MASK	                0xffffffe0 /* 11111111 11111111 11111111 11100000 */
#define NAND_ADDRESS_EU_BITS_COUNT      11 // actually: log(page_address) - NAND_ADDRESS_PAGE_BITS_COUNT
#define SIM_FILE_NAME   "flash_sim_512b_page"

#else
#define POWER_NAND_PAGES_PER_ERASE_UNIT      6   /* 64 */
#define POWER_NAND_PAGE_SIZE		    11  /* 2048 */
#define POWER_NAND_SPARE_SIZE		    6   /* 64 */
#define NAND_EU_MASK	                0xffffffc0 /* 11111111 11111111 11111111 11000000 */
#define SIM_FILE_NAME   "flash_sim_2048b_page"
#define NAND_ADDRESS_EU_BITS_COUNT      13 // for 1gb chip simulation
#endif

#define NAND_PAGE_SIZE				  (1 << POWER_NAND_PAGE_SIZE)  
#define NAND_PAGES_PER_ERASE_UNIT     (1 << POWER_NAND_PAGES_PER_ERASE_UNIT) 
#define NAND_SPARE_SIZE               (1 << POWER_NAND_SPARE_SIZE) 
#define NAND_TOTAL_SIZE				  (NAND_PAGE_SIZE+NAND_SPARE_SIZE)
#define NAND_MAX_EU_OFFSET			  (NAND_PAGES_PER_ERASE_UNIT-1)
#define NAND_ADDRESS_PAGE_BITS_COUNT  POWER_NAND_PAGES_PER_ERASE_UNIT // actually: log(NAND_PAGES_PER_ERASE_UNIT)
#define POWER_NAND_PAGE_COUNT		  (POWER_NAND_PAGES_PER_ERASE_UNIT+NAND_ADDRESS_EU_BITS_COUNT)
#define NAND_PAGE_COUNT				  (1 << POWER_NAND_PAGE_COUNT)
#define NAND_MAX_PARTIAL_WRITES		  2
#define NAND_MAX_PARTIAL_WRITES_SPARE 3
#define NAND_BAD_EU_FLAG_BYTE_NUM     6 
#define NAND_VERIFY_BYTE_LOCATION     (NAND_PAGE_SIZE+NAND_BAD_EU_FLAG_BYTE_NUM-1) // -1 since the 518th byte, is 517 in the byte array
#define NAND_DEVICE_ID				  0x00000f0f

#ifndef DO_ECC 
#define ECC_BYTS_PER_256_BYTES 0
#else
#define POWER_256 8
#define ECC_BYTES    ((NAND_PAGE_SIZE >> POWER_256) * ECC_BYTS_PER_256_BYTES)
#endif

/* spare area flags and pointers */
typedef struct __spare{
	uint8_t bytes[NAND_SPARE_SIZE];
} spare_area_flags;

#define PREV_BLOCK_PTR_LOCATION 0
#define COPY_BACK_LOCATION		(PREV_BLOCK_PTR_LOCATION+3)
#define CHECKPOINT_LOCATION		(COPY_BACK_LOCATION+1)
#define VOTS_LOCATION			CHECKPOINT_LOCATION
#define COPY_LOCATION			CHECKPOINT_LOCATION
#define SEG_TYPE_LOCATION		CHECKPOINT_LOCATION
#define BAD_EU_BYTE_LOCATION	(CHECKPOINT_LOCATION+1)
#define SLOT_OFFSET_LOCATION	(BAD_EU_BYTE_LOCATION+1)
#define ECC_LOCATION			(SLOT_OFFSET_LOCATION+3)
#define OBS_LOCATION			(NAND_SPARE_SIZE-1)

#define GET_PREV_BLOCK(FLAGS_PTR)     (GET_BYTE(FLAGS_PTR,PREV_BLOCK_PTR_LOCATION) | (GET_BYTE(FLAGS_PTR,PREV_BLOCK_PTR_LOCATION+1) << 8) | \
									  (GET_BYTE(FLAGS_PTR,PREV_BLOCK_PTR_LOCATION+2) << 16))
#define SET_PREV_BLOCK(FLAGS_PTR,VAL) {SET_BYTE(FLAGS_PTR, PREV_BLOCK_PTR_LOCATION,  GET_BYTE0(VAL));\
									  SET_BYTE(FLAGS_PTR, PREV_BLOCK_PTR_LOCATION+1, GET_BYTE1(VAL));\
  									  SET_BYTE(FLAGS_PTR, PREV_BLOCK_PTR_LOCATION+2, GET_BYTE2(VAL));}

#define GET_COPY_BACK_FLAG(FLAGS_PTR)      (GET_BIT(FLAGS_PTR, 7, COPY_BACK_LOCATION))
#define SET_COPY_BACK_FLAG(FLAGS_PTR,VAL)  SET_BIT(FLAGS_PTR, 7, COPY_BACK_LOCATION, VAL)

#define GET_CHECKPOINT_FLAG(FLAGS_PTR)      (GET_BIT(FLAGS_PTR, 6, CHECKPOINT_LOCATION) | (GET_BIT(FLAGS_PTR, 7, CHECKPOINT_LOCATION) << 1))
#define SET_CHECKPOINT_FLAG(FLAGS_PTR,VAL)  {SET_BIT(FLAGS_PTR, 6, CHECKPOINT_LOCATION, (VAL) & 1);\
											SET_BIT(FLAGS_PTR, 7, CHECKPOINT_LOCATION, (VAL) & 2);}

#define GET_VOTS_FLAG(FLAGS_PTR)      (GET_BIT(FLAGS_PTR, 4, VOTS_LOCATION) | (GET_BIT(FLAGS_PTR, 5, VOTS_LOCATION) << 1))
#define SET_VOTS_FLAG(FLAGS_PTR,VAL)  {SET_BIT(FLAGS_PTR, 4, VOTS_LOCATION, (VAL) & 1);\
									  SET_BIT(FLAGS_PTR, 5, VOTS_LOCATION, VAL & 2);}

#define GET_COPY_FLAG(FLAGS_PTR)      (GET_BIT(FLAGS_PTR, 3, COPY_LOCATION))
#define SET_COPY_FLAG(FLAGS_PTR,VAL)  SET_BIT(FLAGS_PTR, 3, COPY_LOCATION, (VAL));

#define GET_SEG_TYPE_FLAG(FLAGS_PTR)      (GET_BIT(FLAGS_PTR, 0, SEG_TYPE_LOCATION) | (GET_BIT(FLAGS_PTR, 1, SEG_TYPE_LOCATION) << 1) | (GET_BIT(FLAGS_PTR, 2, SEG_TYPE_LOCATION) << 2))
#define SET_SEG_TYPE_FLAG(FLAGS_PTR,VAL)  {SET_BIT(FLAGS_PTR, 0, SEG_TYPE_LOCATION, (VAL) & 1);\
										  SET_BIT(FLAGS_PTR, 1, SEG_TYPE_LOCATION, (VAL) & 2);\
										  SET_BIT(FLAGS_PTR, 2, SEG_TYPE_LOCATION, (VAL) & 4);}										  

#define GET_BAD_EU(FLAGS_PTR)			  GET_BYTE(FLAGS_PTR, BAD_EU_BYTE_LOCATION)
#define SET_BAD_EU(FLAGS_PTR, VAL)		  SET_BYTE(FLAGS_PTR, BAD_EU_BYTE_LOCATION, VAL)

#define GET_SLOT_EU_OFFSET(FLAGS_PTR)     (GET_BYTE(FLAGS_PTR,SLOT_OFFSET_LOCATION) | (GET_BYTE(FLAGS_PTR,SLOT_OFFSET_LOCATION+1) << 8) | \
									      (GET_BYTE(FLAGS_PTR,SLOT_OFFSET_LOCATION+2) << 16))
#define SET_SLOT_EU_OFFSET(FLAGS_PTR,VAL) {SET_BYTE(FLAGS_PTR, SLOT_OFFSET_LOCATION,  GET_BYTE0(VAL));\
									 	  SET_BYTE(FLAGS_PTR, SLOT_OFFSET_LOCATION+1, GET_BYTE1(VAL));\
  									  	  SET_BYTE(FLAGS_PTR, SLOT_OFFSET_LOCATION+2, GET_BYTE2(VAL));}
 
#define GET_OBS_FLAG(FLAGS_PTR)      (GET_BIT(FLAGS_PTR, 7, OBS_LOCATION))
#define SET_OBS_FLAG(FLAGS_PTR,VAL)  SET_BIT(FLAGS_PTR, 7, OBS_LOCATION, (VAL))

typedef struct _page_area_flags{
	uint64_t sequence_num;
	uint16_t segment_id;
} page_area_flags;

#define CAST_TO_PAGE_AREA_FLAGS(BUF)	
#define GET_HEADER_SEQUENCE_NUM(BUF)	(((page_area_flags*)(BUF))->sequence_num)
#define GET_HEADER_SEGMENT_ID(BUF)		(((page_area_flags*)(BUF))->segment_id)
#define SET_HEADER_SEQUENCE_NUM(BUF, SEQ_NUM)	((page_area_flags*)(BUF))->sequence_num = SEQ_NUM
#define SET_HEADER_SEGMENT_ID(BUF, SEG_ID)		((page_area_flags*)(BUF))->segment_id = SEG_ID

#define INIT_FLAGS_STRUCT_AND_PTR(FLAGS_NAME)          spare_area_flags flags_struct, *FLAGS_NAME = &flags_struct
#define CAST_TO_UINT8(PTR)   ((uint8_t*)(PTR))
#define INIT_FLAGS_POINTER_TO_BUFFER(FLAGS, BUFFER)    spare_area_flags *FLAGS = ((spare_area_flags *)(&(CAST_TO_UINT8(BUFFER)[NAND_PAGE_SIZE])))   													   
#define INIT_PAGE_AREA_PTR_TO_BUFFER(PTR_NAME, BUF)    page_area_flags *PTR_NAME = (page_area_flags*)(BUF)

/* macros */
#define nandReadPageFlags(BUF, PHY_ADDR)               nandReadPageSpare(CAST_TO_UINT8(BUF), PHY_ADDR, 0, NAND_SPARE_SIZE)
#define nandReadPageTotal(BUF, PHY_ADDR)               nandReadPage(CAST_TO_UINT8(BUF), PHY_ADDR, 0, NAND_TOTAL_SIZE)

error_t nandInit(void);

/**
 * @brief
 * verify that the expected ECC of the page is as expected
 * @param buf buffer with page data
 * @return 1 if ecc failed, 0 if succeeded
 */
error_t verifyECC(uint8_t *buf);

/**
 * @brief
 * set ecc field of page flags
 * @param buf buffer with page data
 */
error_t setECC(uint8_t *buf);

uint32_t nandReadID(void);

error_t nandReadStatus(void);

error_t nandReadPage(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len);

error_t nandReadPageSpare(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len);

error_t nandProgramPageA(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len);

error_t nandProgramPageB(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len);

error_t nandProgramPageC(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len);

error_t markEuAsBad(uint32_t page_addr);

#ifdef Debug
/**
 * @brief
 * erase an EU regardless of it's status (bad or not)
 */
error_t nandBruteErase(uint32_t page_in_erase_unit);
#endif

error_t nandErase(uint32_t page_in_erase_unit);

void nandTerminate(void);	

error_t nandCheckEuStatus(uint32_t phy_addr);

#endif /*NAND_H_*/
