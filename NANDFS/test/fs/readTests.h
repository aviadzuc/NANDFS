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

#ifndef READTESTS_H_
#define READTESTS_H_

#include <system.h>

void runAllreadTests(void);

/**
 * @brief 
 * init read test
 */
void init_readTest(void);

/**
 * @brief 
 * tear down read test
 */
void tearDown_readTest(void);

/**
 * @brief
 * try reading empty file. should return 0 
 * @return 1 if successful, 0 otherwise
 */
error_t readTest1(void);

/**
 * @brief
 * read data from inode file data.
 * should succeed 
 * @return 1 if successful, 0 otherwise
 */
error_t readTest2(void);

/**
 * @brief
 * read data from direct entry.
 * should succeed 
 * @return 1 if successful, 0 otherwise
 */
error_t readTest3(void);

/**
 * @brief
 * read data from indirect entry.
 * should succeed  
 * @return 1 if successful, 0 otherwise
 */
error_t readTest4(void);

/**
 * @brief
 * read data from double entry.
 * should succeed  
 * @return 1 if successful, 0 otherwise
 */
error_t readTest5(void);

/**
 * @brief
 * read data from triple entry.
 * should succeed 
 * @return 1 if successful, 0 otherwise
 */
error_t readTest6(void);

/**
 * @brief
 * read data from illegal file descriptor, and illegal bytes count.
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t readTest7(void);

/**
 * @brief
 * read data from inode file data to triple entry, and verify success 
 * @return 1 if successful, 0 otherwise
 */
error_t readTest8(void);

/**
 * @brief
 * read from two files concurrently, that belong to two different users. should succeed.
 * also test reading a file using a file descriptor that was opened by an other user. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t readTest9(void);

/**
 * @brief
 * try reading a file by two users concurrently from different file descriptors 
 * @return 1 if successful, 0 otherwise
 */
error_t readTest10(void);

/**
 * @brief
 * try reading a directory. should fail 
 * @return 1 if successful, 0 otherwise
 */
error_t readTest11(void);

/**
 * @brief
 * read beyond file size, so that we read less bytes than expected 
 * @return 1 if successful, 0 otherwise
 */
error_t readTest12(void);

/**
 * @brief
 * open file with O_RDWR, and read/write simultaneously from transaction indirect block. should succeed  
 * @return 1 if successful, 0 otherwise
 */
error_t readTest13(void);

/**
 * @brief
 * open file with O_RDWR, and read/write simultaneously. read not from transaction indirect block. 
 * should succeed.  
 * @return 1 if successful, 0 otherwise
 */
error_t readTest14(void);

/**
 * @brief
 * open file by the same user, once for O_RDONLY once for O_WRONLY.
 * read and write simultaneously, shoudl succeed.
 * try reading with another user. should fail.
 * try open for reading with another user. should fail.   
 * @return 1 if successful, 0 otherwise
 */
error_t readTest15(void);

/**
 * @brief
 * read data where no complete block are read 
 * @return 1 if successful, 0 otherwise
 */
error_t readTest16(void);

/**
 * @brief
 * write and read to beginning of file in non-aligned offsets 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t readTest17();
#endif /*READTESTS_H_*/
