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

#ifndef VARIOUSFUNCSTESTS_H_
#define VARIOUSFUNCSTESTS_H_

#include <system.h>

void runAllVariousFuncsTests(void);

/**
 * @brief 
 * init lseek test
 * 
 */
void init_lseekTest(void);

/**
 * @brief 
 * tear down lseek test
 * 
 */
void tearDown_lseekTest(void);

/**
 * @brief
 * seek file to various legal offset. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t lseekTest1(void);

/**
 * @brief
 * seek file to offset that exceeds file size. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t lseekTest2(void);

/**
 * @brief
 * seek file with illegal whence, and illegal offset. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t lseekTest3(void);

/**
 * @brief
 * seek file that is written by a transaction that extended the file offset, and hasn't
 * commited inode yet to flash. seek is done to newly written section. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t lseekTest4(void);

/**
 * @brief 
 * init telldir test
 * 
 */
void init_telldirTest(void);

/**
 * @brief 
 * tear down telldir test
 * 
 */
void tearDown_telldirTest(void);

/**
 * @brief
 * tell dir directory. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t telldirTest1(void);

/**
 * @brief
 * telldir empty dirstream. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t telldirTest2(void);

/**
 * @brief
 * telldir directory in some offset. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t telldirTest3(void);

/**
 * @brief
 * telldir directory opened by another user. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t telldirTest4(void);

/**
 * @brief
 * telldir directory after reaching EOF with readdir.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t telldirTest5(void);

/**
 * @brief 
 * init dirfd test
 * 
 */
void init_dirfdTest(void);

/**
 * @brief 
 * tear down dirfd test
 * 
 */
void tearDown_dirfdTest(void);

/**
 * @brief
 * dirfd dirstream. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t dirfdTest1(void);

/**
 * @brief
 * dirfd empty dirstream. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t dirfdTest2(void);

/**
 * @brief
 * dirfd dirstream opened by another user. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t dirfdTest3(void);

/**
 * @brief 
 * init seekdir test
 * 
 */
void init_seekdirTest(void);

/**
 * @brief 
 * tear down seekdir test
 * 
 */
void tearDown_seekdirTest(void);

/**
 * @brief
 * seekdir directoy. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t seekdirTest1(void);

/**
 * @brief
 * seekdir dirstream opened by another user. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t seekdirTest2(void);

/**
 * @brief 
 * init rewinddir test
 * 
 */
void init_rewinddirTest(void);

/**
 * @brief 
 * tear down rewinddir test
 * 
 */
void tearDown_rewinddirTest(void);

/**
 * @brief
 * rewinddir directoy. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t rewinddirTest1(void);

/**
 * @brief
 * rewinddir dirstream opened by another user. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rewinddirTest2(void);

/**
 * @brief 
 * init closedir test
 * 
 */
void init_closedirTest(void);

/**
 * @brief 
 * tear down closedir test
 * 
 */
void tearDown_closedirTest(void);

/**
 * @brief
 * close directory. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t closedirTest1(void);

/**
 * @brief
 * close non-existsent directory stream. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t closedirTest2(void);

/**
 * @brief
 * close directory opened by 2 users. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t closedirTest3(void);

/**
 * @brief 
 * init stat test
 * 
 */
void init_statTest(void);

/**
 * @brief 
 * tear down stat test
 * 
 */
void tearDown_statTest(void);

/**
 * @brief
 * stat a file. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t statTest1(void);

/**
 * @brief
 * stat a directory. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t statTest2(void);

/**
 * @brief
 * stat non-existant file. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t statTest3(void);

/**
 * @brief
 * stat a file in the middle of uncommited transacction and verify size is as transaction file size.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t statTest4(void);

/**
 * @brief 
 * init dup test
 * 
 */
void init_dupTest(void);

/**
 * @brief 
 * tear down dup test
 * 
 */
void tearDown_dupTest(void);

/**
 * @brief
 * duplicate a file descriptor. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t dupTest1(void);

/**
 * @brief
 * duplicate illegal file desciptors. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t dupTest2(void);

/**
 * @brief
 * duplicate file descriptor when there are no free descriptors.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t dupTest3(void);

/**
 * @brief 
 * init dup2 test
 * 
 */
void init_dup2Test(void);

/**
 * @brief 
 * tear down dup2 test
 * 
 */
void tearDown_dup2Test(void);

/**
 * @brief
 * duplicate a file descriptor to an empty file desceriptor. 
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test1(void);

/**
 * @brief
 * duplicate a file descriptor to a file open for reading 
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test2(void);

/**
 * @brief
 * duplicate a file descriptor to a file open for writing 
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test3(void);

/**
 * @brief
 * duplicate a file descriptor to an illegal file descriptor 
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test4(void);

/**
 * @brief
 * duplicate a file descriptor to a new file descriptor of a file open for writing.
 * when closing the new fd will result in failure because the current user cannot close it.
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test5(void);

/**
 * @brief
 * duplicate a file descriptor to an empty file desceriptor 3 times. 
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t dup2Test6(void);

#endif /*VARIOUSFUNCSTESTS_H_*/
