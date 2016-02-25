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

#ifndef TESTSFS_H_
#define TESTSFS_H_

#include <system.h>

error_t init_AllOpenTests();
error_t tearDown_AllOpenTests();

/**
 * @brief
 * open a file that already exists for reading.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t openTest1();

/**
 * @brief
 * open a file that doesn't exists for reading.
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t openTest2();

/**
 * @brief
 * open a file that exists for writing.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t openTest3();

/**
 * @brief
 * open a file that doesn't exists for writing
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t openTest4();
/**
 * @brief
 * open a file that exists twice for reading
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t openTest5();
/**
 * @brief
 * open a file for writing, then try to open it to reading
 * should fail (write exclusive)
 * @return 1 if successful, 0 otherwise
 */
error_t openTest6();
/**
 * @brief
 * open a file that exusts for reading, the try to  open for writing. 
 * should fail (write exclusive)
 * @return 1 if successful, 0 otherwise
 */
error_t openTest7();

/**
 * @brief
 * open FS_MAX_VNODES different files for reading , than another file and making sure
 * the last open fails with the matching error 
 * @return 1 if successful, 0 otherwise
 */
error_t openTest8();

/**
 * @brief
 * open with different users FS_MAX_OPEN_FILES open file entries (for less than FS_MAX_VNODES files),
 * than another file and making sure  the last open fails with 
 * the matching error 
 * @return 1 if successful, 0 otherwise
 */
error_t openTest9();

/************* test to check after we have read, write etc.***********/
/**
 * @brief
 * open a file that exists with O_TRUNCAT and verify it was indeed truncated 
 * @return 1 if successful, 0 otherwise
 */
error_t openTest20();

/**
 * @brief
 * open a file with O_CREAT and verify it was created
 * @return 1 if successful, 0 otherwise
 */
error_t openTest21();

/**
 * @brief
 * open a file for writing with O_APPEND and making sure that 
 * the file entry offset is set to the end of the file
 * should fail
 * @return 1 if successful, 0 otherwise
 */ 
error_t openTest22();

/**
 * @brief
 * try opening a pathname that leads to a directory.
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t openTest23();

/**
 * @brief
 * try opening a pathname which has a non existing directory in the middle
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t openTest24();

/**
 * @brief
 * try opening a pathname which has a non existing directory in the end
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t openTest25();

/**
 * @brief
 * try opening a legal pathname with "." and ".." 
 * @return 1 if successful, 0 otherwise
 */
error_t openTest26();

#endif /*TESTSFS_H_*/
