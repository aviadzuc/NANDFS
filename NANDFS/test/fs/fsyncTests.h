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

#ifndef FSYNCTESTS_H_
#define FSYNCTESTS_H_

#include <system.h>

void runAllfsyncTests(void);

/**
 * @brief 
 * init fsync test
 * 
 */
void init_fsyncTest(void);

/**
 * @brief 
 * tear down fsync test
 * 
 */
void tearDown_fsyncTest(void);

/**
 * @brief
 * fsync a file with uncommited data in inode 
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest0();

/**
 * @brief
 * fsync a file with uncommited data and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest1(void);

/**
 * @brief
 * fsync illegal fd and verify failure
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest2(void);

/**
 * @brief
 * fsync file that has no uncommited data.
 * should succeed without performing any writes to flash
 *
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest3(void);

/**
 * @brief
 * write to file until it exhausts all free pages. then try to fsync it.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest4(void);

/**
 * @brief
 * write to file until it exhausts all free pages, and doesn't have enough free vots.
 * then try to fsync it. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t fsyncTest5(void);


#endif /*FSYNCTESTS_H_*/
