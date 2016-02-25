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

#ifndef SEQUENCINGBOOTINGTESTS_H_
#define SEQUENCINGBOOTINGTESTS_H_

#include <system.h>

/**
 * run all sequencingBooting tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllSequencingBootingTests(void);

/**
 * @brief 
 * init sequencingBooting test
 */
error_t init_sequencingBootingTest(void);
		
/**
 * @brief 
 * tear down sequencingBooting test
 */
error_t tearDown_sequencingBootingTest(void);

/**
 * @brief
 * the flash is empty.
 * booting should detect it, and initialize the file system. i.e:
 * 1) mark reserve segments
 * 2) mark first segment and it's checkpoint
 * 3) prepare for writing
 *
 * @return 1 if successful, 0 if a read/write error occured
 */
error_t sequencingBootingTest1(void);

/**
 * @brief
 * we crashed during stage (1) of initialization.
 * booting should detect it, delete header of last written reserve segment
 * and continue initialization
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest2(void);

/**
 * @brief
 * we crashed during stage (2) of initialization.
 * booting should detect it, delete header of first segment and
 * continue initialization
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest3(void);

/**
 * @brief
 * we crashed during regular allocation (tests usage of allocAndWriteBlock(void) also). 
 * last EU allocated is regular. booting should:
 * - detect it
 * - do a copy back
 * - find latest checkpoint
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest4(void);

/**
 * @brief
 * we crashed during regular allocation, when writing new header for segment #1. last EU allocated is regular.
 * booting should:
 * - detect it
 * - do a copy back
 * - findCheckpointAndTruncate(void)
 * - dedect system is empty
 * - initialize
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest5(void);

/**
 * @brief
 * we crashed during regular allocation, when writing new header for segment. last EU allocated is reserve.
 * booting should:
 * - detect it
 * - do a copy back
 * - findCheckpointAndTruncate
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest7(void);

/**
 * @brief
 * the flash is empty. first EU is bad, and so is the first EU of the first reserve segment 
 * booting should detect it, and initialize the file system. i.e:
 * 1) mark reserve segments
 * 2) mark first segment and it's checkpoint
 * 3) prepare for writing
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest8(void);

/**
 * @brief * 
 * we crashed during stage (1) of initialization.
 * first EU is bad, and so is the first EU of the first reserve segment 
 * booting should detect it, delete header of last written reserve segment
 * and continue initialization
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest9(void);

/**
 * @brief
 * we crashed during stage (2) of initialization.
 * first EU is bad, and so is the first EU of the first reserve segment.
 * booting should detect it, delete header of first segment and
 * continue initialization
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest10(void);

/**
 * @brief
 * we crashed during regular allocation. last EU allocated is reserve.
 * booting should:
 * - detect it
 * - do a copy back
 * - find latest checkpoint
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest11(void);

/**
 * @brief
 * we crashed during regular allocation, when writing new header for segment #1. it's first EU is reserve.
 * booting should:
 * - detect it
 * - do a copy back
 * - findCheckpointAndTruncate(void)
 * - detect system is empty
 * - initialize
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest12(void);

/**
 * @brief
 * we crashed after writing last page in regular allocation. we have a checkpoint halfway thru the segment. 
 * booting should:
 * - detect it
 * - findCheckpointAndTruncate(void)
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest13(void);

/**
 * @brief
 * we crashed during reclamation, when writing new header for a newly reclaimed segment. first EU is regular.
 * booting should:
 * - detect it
 * - copyback
 * - findCheckpointAndTruncate(void)
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest14(void);

/**
 * @brief
 * we crashed during reclamation, when writing new header for the new segment. it's first EU is reserve.
 * booting should:
 * - detect it
 * - copyback
 * - findCheckpointAndTruncate(void)
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest15(void);

/**
 * @brief
 * we crashed during reclamation. last written EU is regular. 
 * the new generation checkpoint is complete , so we should return to reclamation state.
 * booting should:
 * - detect it
 * - copyback
 * - findCheckpointAndTruncate(void)
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest16(void);

/**
 * @brief
 * we crashed during copyback, during regular allocation. last written EU is regular, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate(void)
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest17(void);

/**
 * @brief
 * we crashed during copyback, during regular allocation. last written EU is reserve, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate(void)
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest18(void);

/**
 * @brief
 * we crashed during copyback, during reclamation. last written EU is regular, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate(void)
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest19(void);

/**
 * @brief
 * we crashed during copyback, during reclamation. last written EU is reserve, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate(void)
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest20(void);


/**
 * @brief
 * we crashed during copyback, during regular allocation of very first EU. last written EU is regular, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate(void)
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest21(void);

/**
 * @brief
 * we crashed during copyback, during regular allocation of very first EU. last written EU is reserve, and was half written.
 * booting should:
 * - detect it
 * - complete copyback
 * - findCheckpointAndTruncate(void)
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest22(void);

/**
 * @brief
 * we crashed during wear leveling. in the middle of copying the original slot, we reboot. 
 * last EU we copied to is regular.
 * booting should: 
 * - detect it
 * - delete incomplete slot
 * - find segment to allocate
 * - prepare for next allocation
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest23(void);

/**
 * @brief
 * we crashed during wear leveling. in the middle of copying the original slot, we reboot. 
 * last EU we copied to is reserve.
 * booting should: 
 * - detect it
 * - delete incomplete slot
 * - find segment to allocate
 * - prepare for next allocation
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest24(void);

/**
 * @brief
 * we crashed during erasure of old generation of reclaimed segment. 
 * we crashed just before erasing a regular EU
 * booting should: 
 * - detect it
 * - delete incomplete slot
 * - find segment to allocate
 * - prepare for next allocation
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest25(void);

/**
 * @brief
 * we crashed during erasure of old generation of reclaimed segment. 
 * we crashed just before erasing a reserve EU
 * booting should: 
 * - detect it
 * - delete incomplete slot
 * - find segment to allocate
 * - prepare for next allocation
 *
 * @return 1 if successful, 0 otherwise
 */
error_t sequencingBootingTest26(void);

#endif /*SEQUENCINGBOOTINGTESTS_H_*/
