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

#ifndef ALLOCANDWRITEBLOCKTESTS_H_
#define ALLOCANDWRITEBLOCKTESTS_H_

#include <system.h>

/**
 * run all allocAndWriteBlock tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllallocAndWriteBlockTests(void);

/**
 * @brief 
 * init allocAndWriteBlock test
 */
error_t init_allocAndWriteBlockTest(void);				

/**
 * @brief 
 * tear down allocAndWriteBlock test
 */
error_t tearDown_allocAndWriteBlockTest(void);

/**
 * @brief
 * write to a segment when the flash is not full (not in allocation mode).
 * write is done in the middle of an EU (no funny stuff).
 * should simply write the page and advance pointer
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest1(void);

/**
 * @brief
 * write to a segment when the flash is not full (not in reclamation mode).
 * write is to the end of EU, forcing the test of the next EU whetehr it is valid.
 * should simply write the page and advance pointer
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest2(void);

/**
 * @brief
 * write to a segment when the flash is not full (not in reclamation mode).
 * write is to the end of EU, forcing the test of the next EU whetehr it is valid.
 * next eu is not valid, so a reserve EU should be allocated * 
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest3(void);

/**
 * @brief
 * write to a segment when we are in reclamation state.
 * no copying is needed. 
 * should simply write the page and advance pointer
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest4(void);

/**
 * @brief
 * write to a segment when we are in reclamation state.
 * write is done to last page of EU, so we now move to a new one.
 * the next one is bad so a reserve EU should be allocated
 * no copying is needed. 
 * should simply write the page and advance pointer.
 * verify segment map us amrked with the reserve EU address
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest5(void);


/**
 * @brief
 * write to a segment when the flash is not full (not in reclamation mode).
 * write is to the end of a segment, forcing us to find and allocate a new segment.
 * should write the page, write header, checkpoint and advance pointer to a new segment
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest7(void);

/**
 * @brief
 * write to the end of a segment, forcing us to find and allocate a new segment, in the point we move from
 * regular allocation to reclamation (meaning, we now have to find a segment to reclaim).
 * should write the page, not truncate any segment, find a segment to reclaim, write header, 
 * checkpoint and advance pointer to the new segment.
 * (no copying required)
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest8(void);

/**
 * @brief
 * write is to the end of a segment, forcing us to find and allocate a new segment (now in reclamation mode).
 * should write the page, find a segment to reclaim, write header, checkpoint and advance pointer to the new segment.
 * now we will mark only the k'th page of every segment as obsolete. this is to ensure it will not be copied
 * and that whatever segment we choose to reclaim, the reclaimed address in the segment map will be in offset k
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest9(void);

/**
 * @brief
 * write to a segment when the flash is not full (not in reclamation mode).
 * write is to the end of EU, forcing the test of the next EU whetehr it is valid.
 * next eu is not valid, so a reserve EU should be allocated.
 * However ALL reserve EU's are taken, so an error should be returned 
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest10(void);

/**
 * @brief
 * write to a segment, in reclamation mode.
 * write is to the end of a segment, forcing us to find and allocate a new segment.
 * we force choosing a segment to reclaim with obs counter level high, but actually no obsolete pages.
 * should write the page, mark another segment for writing.
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest11(void);

/**
 * @brief
 * write to a segment when the flash is full (not in reclamation mode).
 * write is to the end of a segment, forcing us to find and allocate a new segment.
 * should write the page and return error
 * @return 1 if test is successful,0 if not.
 */
error_t allocAndWriteBlockTest12(void);

/**
 * @brief
 * in regulat allocation mode allocate the last page, mark pages as obsolete, and verify obsolete pages count increase, and free pages counter doesn't change 
 * @return 1 if succesful, 0 otherwise
 */
error_t allocAndWriteBlockTest13(void);

#endif /*ALLOCANDWRITEBLOCKTESTS_H_*/
