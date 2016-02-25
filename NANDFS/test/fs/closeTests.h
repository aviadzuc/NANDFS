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

#ifndef CLOSETESTS_H_
#define CLOSETESTS_H_

#include <system.h>

void runAllcloseTests(void);

/**
 * @brief 
 * init close test
 * 
 */
void init_closeTest(void);

/**
 * @brief 
 * tear down close test
 * 
 */
void tearDown_closeTest(void);

/**
 * @brief
 * open file for writing and close it. 
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest1(void);

/**
 * @brief
 * close illegal fd and verify failure
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest2(void);

/**
 * @brief
 * open file using creat, and close it.
 * verify no physical writes 
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest3(void);

/**
 * @brief
 * open file for writing, write and close.
 * verify transaction was commited properly 
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest4(void);

/**
 * @brief
 * open file for writing, write. try to open it again for writing (should fail)
 * close file, and try again to open for writing. verify success
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest5(void);

/**
 * @brief
 * open 1 file for reading, open 2 files, write to them, and close them both.
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest6(void);

/**
 * @brief
 * write to file until it exhausts all free pages. then try to close it.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest7(void);

/**
 * @brief
 * write to file until it exhausts all free pages, and doesn't have enough free vots.
 * then try to close it. should fail
 * @return 1 if successful, 0 otherwise
 */
error_t closeTest8(void);

#endif /*CLOSETESTS_H_*/
