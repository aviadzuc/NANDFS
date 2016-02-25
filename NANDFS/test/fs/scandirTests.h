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

#ifndef SCANDIRTESTS_H_
#define SCANDIRTESTS_H_

#include <system.h>

void runAllScandirTests(void);

/**
 * @brief 
 * init scandir test
 * 
 */
void init_scandirTest(void);

/**
 * @brief 
 * tear down scandir test
 * 
 */
void tearDown_scandirTest(void);

/**
 * @brief
 * scan a directory that contains directory entries, filter according to length, and sort by name.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t scandirTest1(void);

/**
 * @brief
 * scan illegal directory name.
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t scandirTest3(void);

/**
 * @brief
 * scan directory and filter all entries.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t scandirTest4(void);

/**
 * @brief
 * scan a directory that contains directory entries, where allocating all entries will result in an error. 
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t scandirTest5(void);

#endif /*SCANDIRTESTS_H_*/
