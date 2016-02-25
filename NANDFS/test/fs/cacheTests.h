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

#ifndef CACHETESTS_H_
#define CACHETESTS_H_

#include <system.h>

void runAllCacheTests(void);

/**
 * @brief 
 * init cache test
 * 
 */
void init_cacheTest(void);

/**
 * @brief 
 * tear down cache test
 * 
 */
void tearDown_cacheTest(void);

/**
 * @brief
 * manipulate root idrectory to contain blocks until indirect offset.
 * read first root directory file block, verify one cache buffer is allocated to it.
 * read a block in indirect offset and make sure the cache changes to this offset, 
 * and there is still only one cache for this file.
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest0();

/**
 * @brief
 * test a situation with no dedicated read buffers - try to read, and verify no caching was performed.  
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest1();

/**
 * @brief
 * create a file, write to it and verify there are caches of the file and it's parent directory (if there are two read caches at least)
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest2();

/**
 * @brief
 * create a file, close it, and verify all indirect caches are empty
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest3();

/**
 * @brief
 * write to a file in inode data offset. close, read file.
 * verify a cache contains the file inode 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest4();

/**
 * @brief
 * write to a file until direct entries offset. close, read file.
 * verify a cache contains the file inode 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest5();

/**
 * @brief
 * write to a file until indirect entry data offset. close, read file.
 * verify a cache contains the file inode 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest6();

/**
 * @brief
 * write to a file utnil double entry data offset. close, read file.
 * verify a cache contains the file inode 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest7();

/**
 * @brief
 * write to a file until triple entry data offset. close, read file.
 * verify a cache contains the file inode 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest8();

/**
 * @brief
 * scan a directory, verify cache contains it's indirect block
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest9();

/**
 * @brief
 * read two files concurrently, and verify we have two caches relating to each file
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest10();

/**
 * @brief
 * read 3 files, and verify each one of them has a buffer
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest11();

/**
 * @brief
 * create filem write, close.
 * unlink file, create again, write different data, close.
 * try to read file and verify read data is from newly written data
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest12();

error_t cacheTest21();

/**
 * @brief
 * write to 2 files, when we have room for 2 transactions, and verify that we use both of them 
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t cacheTest22();

#endif /*CACHETESTS_H_*/
