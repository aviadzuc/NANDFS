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

#ifndef READVOTSANDPREVTESTS_H_
#define READVOTSANDPREVTESTS_H_

#include <system.h>

/**
 * run all readVOTsAndPrev tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllReadVOTsAndPrevTests(void);

/**
 * @brief 
 * init readVOTsAndPrev test
 */
error_t init_readVOTsAndPrevTest(void);				

/**
 * @brief 
 * tear down readVOTsAndPrev test
 */
error_t tearDown_readVOTsAndPrevTest(void);

/**
 * @brief
 * write a page marked as VOTs, read it using readVOTsAndPrevTest1(void) and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest1(void);

/**
 * @brief
 * - write a page marked as VOTs
 * - write another regular page
 * - write a page marked as VOTs with the previous VOTs write marked in it's header
 * read pages using readVOTsAndPrevTest1(void) and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest2(void);

/**
 * @brief
 * repeat k times the process of 
 * - write VOTs
 * - write data
 * and try k sequential reads using readVOTsAndPrevTest1(void) and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest3(void);

/**
 * @brief
 * repeat k times the process of 
 * - write VOTs of list
 * - write data
 * - write VOTs of different list
 * - write data
 * and try k sequential reads using readVOTsAndPrevTest1(void) and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest4(void);

/**
 * @brief
 * repeat k times the process of 
 * - write VOTs to a segment
 * - write data to another segmnet
 * - write VOTs again for the other segmnet
 * and try to read using readVOTsAndPrevTest1(void) and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest5(void);

/**
 * @brief
 * repeat k times the process of 
 * - write VOTs
 * - write data
 * - write unrelated data
 * and try k sequential reads using readVOTsAndPrevTest1(void) and verify read data.
 * @return 1 if test was successful. return 0 if the test fails.
 * 0 is returned if allocation of page fails.
 */
error_t readVOTsAndPrevTest6(void);

#endif /*READVOTSANDPREVTESTS_H_*/
