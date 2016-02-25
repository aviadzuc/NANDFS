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

#ifndef UNLINKTESTS_H_
#define UNLINKTESTS_H_

#include <system.h>

void runAllunlinkTests(void);

/**
 * @brief 
 * init unlink test
 */
void init_unlinkTest(void);

/**
 * @brief 
 * tear down unlink test
 */
void tearDown_unlinkTest(void);

/**
 * @brief
 * unlink an existing  file in root directory 
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest1(void);

/**
 * @brief
 * unlink a non-existant file in root directory. should fail 
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest2(void);

/**
 * @brief
 * unlink an existing file not in root directory. shoudl succeed 
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest3(void);

/**
 * @brief
 * unlink a non-existant file not in root directory. should fail 
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest4(void);

/**
 * @brief
 * unlink a file open for reading/writing. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest5(void);

/**
 * @brief
 * unlink a file whose parent directory is open for reading/writing 
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest6(void);

/**
 * @brief
 * unlink a file where one of it's parent directories is open for reading/writing. shoudl succeed
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest7(void);

/**
 * @brief
 * unlink a file, and re-creat it. shoudl succeed
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest8(void);

/**
 * @brief
 * unlink file with inode file data, and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest9(void);

/**
 * @brief
 * unlink file with indirect entries, and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest10(void);

/**
 * @brief
 * unlink file with double entries, and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest11(void);

/**
 * @brief
 * unlink file with triple entries, and verify success
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest12(void);

/**
 * @brief
 * unlink file whose direntry is the only directory entry in it's block.
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest13(void);

/**
 * @brief
 * unlink file which resides in indirect entry in inode0
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest14(void);

/**
 * @brief
 * unlink file whose direntry which resides in indirect entry in parent directory
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest15(void);

/**
 * @brief
 * unlink file which resides in double entry in inode0
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest16(void);

/**
 * @brief
 * unlink file whose direntry which resides in double entry in parent directory
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest17(void);

/**
 * @brief
 * unlink file which resides in triple entry in inode0
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest18(void);

/**
 * @brief
 * unlink file whose direntry which resides in triple entry in parent directory
 * @return 1 if successful, 0 otherwise
 */
error_t unlinkTest19(void);

#endif /*UNLINKTESTS_H_*/
