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

#ifndef MKDIRTESTS_H_
#define MKDIRTESTS_H_

#include <system.h>

void runAllMkdirTests(void);

/**
 * @brief 
 * init mkdir test
 * 
 */
void init_mkdirTest(void);

/**
 * @brief 
 * tear down mkdir test
 * 
 */
void tearDown_mkdirTest(void);

/**
 * @brief
 * create a directory in root node, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest1(void);

/**
 * @brief
 * create a directory in root node that already exists, and verify failure
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest2(void);

/**
 * @brief
 * create a directory not in root node, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest3(void);

/**
 * @brief
 * create a directory not in root node that already exists, and verify afilure
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest4(void);

/**
 * @brief
 * create a directory in root node, when we have no free direentries to use and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest6(void);

/**
 * @brief
 * create a directory with length 0 ("/", "directory1/directory2/"). should fail
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest7(void);

/**
 * @brief
 * create a directory in a directory open for reading. should fail
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest8(void);

/**
 * @brief
 * try to create a directory in a non-existent directory
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest9(void);

/**
 * @brief
 * create 2 directories in root node, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest10(void);

/**
 * @brief
 * create directory, and create file in it. verify creation.
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest11(void);

/**
 * @brief
 * create directory, create file in it, unlink file. verify unlink
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest12(void);

/**
 * @brief
 * create a directory whose inode is in triple offset in inode0
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest13(void);

/**
 * @brief
 * create directory within directory until 256 depth. verify creation.
 * @return 1 if successful, 0 otherwise
 */
error_t mkdirTest14(void);

#endif /*MKDIRTESTS_H_*/
