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

#ifndef FINDCHECKPOINTANDTRUNCATETESTS_H_
#define FINDCHECKPOINTANDTRUNCATETESTS_H_

#include <system.h>

/**
 * run all findCheckpointAndTruncate tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllfindCheckpointAndTruncateTests(void);

/**
 * @brief 
 * init findCheckpointAndTruncate test
 */
error_t init_findCheckpointAndTruncateTest(void);	

/**
 * @brief 
 * tear down findCheckpointAndTruncate test
 */
error_t tearDown_findCheckpointAndTruncateTest(void);

/**
 * @brief
 * write a checkpoint to header of segment and stop. then try to find the checkpoint
 * test checkpoint was read properly, that pending VOTs is set as expected.
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest1(void);

/**
 * @brief
 * write a checkpoint to header of segment, write another page and stop. 
 * now we treat the situation as if we rebooted.
 * test if pending VOTs is set to 0 as expected,
 * and that the checkpoint is as expected
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest2(void);

/**
 * @brief
 * write a checkpoint to header of segment, write (pages per EU) * 2 pages. 
 * now we treat the situation as if we rebooted. all EU's and pages after checkpoint should be empty
 * test if pending VOTs is set to 0 as expected,
 * and that the checkpoint is as expected
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest3(void);

/**
 * @brief
 * test segment truncation.
 * write a checkpoint to header of segment, write (segment size) pages after it, so we move to the next segment, and there
 * only page 0 (header) is written. 
 * now we treat the situation as if we rebooted. all EU's and pages after checkpoint should be empty
 * test if pending VOTs is set to 0 as expected,
 * and that the checkpoint is as expected
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest4(void);

/**
 * @brief
 * write a checkpoint to header of segment and stop. then try to find the checkpoint
 * test checkpoint was read properly, that pending VOTs is set as expected.
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest5(void);

/**
 * @brief
 * test segment truncation.
 * - write a checkpoint to header of segment
 * - write k pages.
 * - write another checkpoint
 * - write til the end of segment
 * - write header of next segment
 * now we treat the situation as if we rebooted. all EU's and pages after checkpoint should be empty
 * test if pending VOTs is set to 0 as expected,
 * and that the checkpoint is as expected
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest6(void);	

/**
 * @brief
 * write a checkpoint to header of segment in reclamation and stop. then try to find the checkpoint.
 * should return to reclamation state.
 * test checkpoint was read properly, that pending VOTs is set as expected.
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest7(void);

/**
 * @brief
 * write only first page of header of segment in reclamation and stop. then try to find the checkpoint
 * test checkpoint was read properly, that pending VOTs is set as expected.
 * 
 * @return 1 if successful, 0 if any of the tests fails.
 */
error_t findCheckpointAndTruncateTest8(void);

#endif /*FINDCHECKPOINTANDTRUNCATETESTS_H_*/
