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

#ifndef AUXILIARYFUNCSTESTS_H_
#define AUXILIARYFUNCSTESTS_H_

#include <system.h>

void runAllAuxiliaryFuncsTests(void);

/**
 * @brief
 * init namei test
 *
 */
void init_nameiTest(void);

/**
 * @brief
 * tear down namei test
 *
 */
void tearDown_nameiTest(void);

/**
 * @brief
 * test that various illegal path names are not recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest0(void);

/**
 * @brief
 * test that an existing file in root dir is recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest1(void);

/**
 * @brief
 * test that an non-existing file in root dir is not recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest2(void);

/**
 * @brief
 * test that an existing file not in root dir. should be recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest3(void);

/**
 * @brief
 * test that a non-existing file, where only the last part of the path name is illegal
 * is not recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest4(void);

/**
 * @brief
 * try opening a legal pathname with "." and ".."
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest5(void);

/**
 * @brief
 * test that an existing file not in root dir, and do namei() when cwd is not root and path is relative.
 * should be recognized
 * @return 1 if successful, 0 otherwise
 */
error_t nameiTest6();

/**
 * @brief
 * init readFileBlock test
 *
 */
void init_readFileBlockTest(void);

/**
 * @brief
 * tear down readFileBlock test
 *
 */
void tearDown_readFileBlockTest(void);

/**
 * @brief
 * read file data in inode offset, verify read and that no inode data was read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest1(void);

/**
 * @brief
 * read file data in direct index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest2(void);

/**
 * @brief
 * read file data in empty direct index, verify read empty
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest21(void);

/**
 * @brief
 * read file data in indirect index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest3(void);

/**
 * @brief
 * read file data in empty indirect index, verify read empty
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest31(void);

/**
 * @brief
 * read file data in indirect index, where the direct index is empty.
 * verify read empty
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest32(void);
/**
 * @brief
 * read file data in double index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest4(void);

/**
 * @brief
 * read file data in empty double index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest41(void);

/**
 * @brief
 * read file data in double index, where the direct index is empty.
 * verify read empty
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest42(void);

/**
 * @brief
 * read file data in triple index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest5(void);

/**
 * @brief
 * read file data in empty triple index, verify read
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest51(void);

/**
 * @brief
 * read file data in triple index, where the direct index is empty.
 * verify read empty
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest52(void);

/**
 * @brief
 * read file data from offset larger than file size. verify failure
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest6(void);

/**
 * @brief
 * readFileBlock for a file involved in another transaction, should fail
 * @return 1 if successful, 0 otherwise
 */
error_t readFileBlockTest7(void);

/**
 * @brief
 * init findEmptySparseBlock test
 *
 */
void init_findEmptySparseBlockTest(void);

/**
 * @brief
 * tear down findEmptySparseBlock test
 *
 */
void tearDown_findEmptySparseBlockTest(void);

/**
 * @brief
 * findEmptySparseBlock in root directory. should locate first block
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest1(void);

/**
 * @brief
 * findEmptySparseBlock in root directory, after filling all direct entries, and first indirect entry.
 * should locate second indirect block
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest2(void);

/**
 * @brief
 * findEmptySparseBlock in root directory, after filling all direct entries, marking indirect entries not sparse,
 * and first indirect entry as full.
 * should locate first direct block in second indirect block
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest3(void);

/**
 * @brief
 * findEmptySparseBlock in root directory, after filling all direct entries, marking double, indirect entries as not sparse,
 * and first double entry as full.
 * should locate first direct block in second double block, first indirect block
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest4(void);

/**
 * @brief
 * findEmptySparseBlock in root directory, when all blocks are marked as full, after FS_MAX_FILESIZE size.
 * fill all direct entries, mark double, indirect entries as not sparse, and all triple indexed blocks accordingly *
 * should return error  - can't find block
 *
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest5(void);

/**
 * @brief
 * findEmptySparseBlock in root directory, where there is a negative address followed by valid addresses
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest6(void);

/**
 * @brief
 *
 * @return 1 if successful, 0 otherwise
 */
error_t findEmptySparseBlockTest7(void);

/**
 * @brief
 * init findIndirectEntriesBlock test
 *
 */
void init_findIndirectEntriesBlockTest(void);

/**
 * @brief
 * tear down findIndirectEntriesBlock test
 *
 */
void tearDown_findIndirectEntriesBlockTest(void);

/**
 * @brief
 * get indirect block of offset in file data size.
 * indirect block should be inode
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest1(void);

/**
 * @brief
 * get indirect block of offset in direct entry
 * indirect bock should be inode
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest2(void);

/**
 * @brief
 * get indirect block of offset in indirect entry
 * indirect bock should be the file's indirect block
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest3(void);

/**
 * @brief
 * get indirect block of offset in first double block entry
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest4(void);

/**
 * @brief
 * get indirect block of offset in one of the triple block entries
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest5(void);

/**
 * @brief
 * get indirect block of offset larger than file size
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest6(void);

/**
 * @brief
 * get cached indirect block, and verify it is not in cache anymore
 *
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest7();

/**
 * @brief
 * get indirect block of offset in indirect entry
 * indirect block should be the file's indirect block.
 * verify cached inode is aware of taht
 * @return 1 if successful, 0 otherwise
 */
error_t findIndirectEntriesBlockTest8();

/**
 * @brief
 * init commitInode test
 *
 */
void init_commitInodeTest(void);

/**
 * @brief
 * tear down commitInode test
 *
 */
void tearDown_commitInodeTest(void);

/**
 * @brief
 * commit inode, when indirect is the inode
 * @return 1 if successful, 0 otherwise
 */
error_t commitInodeTest1(void);

/**
 * @brief
 * commit inode, when indirect is the indirect block
 * @return 1 if successful, 0 otherwise
 */
error_t commitInodeTest2(void);

/**
 * @brief
 * commit inode, when indirect is an entry in double block
 * @return 1 if successful, 0 otherwise
 */
error_t commitInodeTest3(void);

/**
 * @brief
 * commit inode, when indirect is mapped somewhere in triple block
 * @return 1 if successful, 0 otherwise
 */
error_t commitInodeTest4(void);

#endif /*AUXILIARYFUNCSTESTS_H_*/
