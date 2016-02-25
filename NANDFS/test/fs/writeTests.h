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

#ifndef WRITETESTS_H_
#define WRITETESTS_H_

#include <system.h>

void runAllwriteTests(void);

/**
 * @brief 
 * init write test
 * 
 */
void init_writeTest(void);

/**
 * @brief 
 * tear down write test
 * 
 */
void tearDown_writeTest(void);

/**
 * @brief
 * write to a new file from offset 0 at length smaller than inode file data size. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest1(void);

/**
 * @brief
 * write to a new file from offset 0 so that data is stored in direct entry. 
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest2(void);

/**
 * @brief
 * write to a new file from offset 0 so that data is stored in indirect block 
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest3(void);

/**
 * @brief
 * write to a new file from offset 0 so that data is stored in double block offset 
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest4(void);

/**
 * @brief
 * write to a new file from offset 0 so that data is stored in triple block offset 
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest5(void);

/**
 * @brief
 * write to a new file not from offset 0 so that data is stored in inode file data
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest6(void);

/**
 * @brief
 * write to a new file not from offset 0 so that data is stored in inode file data
 * and first direct entry. should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest7(void);

/**
 * @brief
 * write to a new file from indirect offset until first direct block of double offset, so that file size is increased.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest8(void);

/**
 * @brief
 * write to a new file from indirect offset until first direct entry in triple offset, so that file size is increased.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest9(void);

/**
 * @brief
 * write to a new file from last entry of indirect offset , so that file size doesn't change.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest10(void);

/**
 * @brief
 * write 2 blocks to a new file from double offset , so that file size doesn't change.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest11(void);


/**
 * @brief
 * write to a new file from triple offset , so that file size doesn't change.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest12(void);

/**
 * @brief
 * write to offset in double area. do creat.
 * continue writing to first file.
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest14(void);

/**
 * @brief
 * write to file, so the indirect block doesn't commit to flash.
 * lseek to file end (the transactional one, not the flash one!)
 * write again, make sure writing was done to transactional file end 
 * should succeed.
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest16(void);

/**
 * @brief
 * write to file, and re-write parts already written. 
 * verify vot count
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest17(void);
 
/**
 * @brief
 * peform write that exhausts all spare pages, so that the transaction commits even though
 * we didn't perform close. in regular allocation mode
 * also test expected vots count, and free pages count
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest20(void);

/**
 * @brief
 * peform write that exhaustst all spare pages, so we don't have anymore left.
 * keep writing until an error is returned, and verify that the transaction was aborted
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest21(void);

/**
 * @brief
 * peform write that exhausts all spare pages, so that the transaction aborts
 * other concurrent transactions in order to complete  
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest22(void);

/**
 * @brief
 * peform write that exhausts all spare pages, so that the transaction aborts
 * other concurrent transactions in order to complete, and also performs 
 * a temporary commit  
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest23(void);

/**
 * @brief
 * peform write that exhausts all spare pages, so that the transaction commits even though
 * we didn't perform close, in reclamation mode.
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest24(void);

/**
 * @brief
 * peform write that exhaustst all spare pages, so we don't have anymore left and
 * a temporary commit is performed. do some more writing so we abort
 * and commited bytes count (<write count) is returned.
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest25(void);

/**
 * @brief
 * write to a file that contains data that has already been written, and is open for reading.
 * write beyond the read offset, and verify that when we continue to read the read data is the
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t writeTest26();
#endif /*WRITETESTS_H_*/
