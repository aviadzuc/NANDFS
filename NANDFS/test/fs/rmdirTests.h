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

#ifndef RMDIRTESTS_H_
#define RMDIRTESTS_H_

#include <system.h>

void runAllRmdirTests(void);

/**
 * @brief 
 * init rmdir test
 */
void init_rmdirTest(void);

/**
 * @brief 
 * tear down rmdir test
 */
void tearDown_rmdirTest(void);

/**
 * @brief
 * rmdir an existing directory in root directory 
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest1(void);

/**
 * @brief
 * rmdir a non-existant directory in root directory. should fail 
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest2(void);

/**
 * @brief
 * rmdir an existing directory not in root directory. should succeed 
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest3(void);

/**
 * @brief
 * rmdir a non-existant directory not in root directory. should fail 
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest4(void);

/**
 * @brief
 * rmdir a directory open for reading. should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest5(void);

/**
 * @brief
 * rmdir a directory whose parent directory is open for reading
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest6(void);

/**
 * @brief
 * rmdir a directory where the suffix of pathname contains "." or "..". try to rmdir root directory
 * should fail.
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest7(void);

/**
 * @brief
 * rmdir a directory, and re-creat it. should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest8(void);

/**
 * @brief
 * rmdir directory that contains a direntry in first inode direct entry.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest9(void);

/**
 * @brief
 * rmdir directory that contains a direntry in some inode direct entry.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest10(void);

/**
 * @brief
 * rmdir directory that contains a direntry in indirect entry.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest11(void);

/**
 * @brief
 * rmdir directory that contains a direntry in double entry.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest12(void);

/**
 * @brief
 * rmdir directory that contains a direntry in triple entry.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest13(void);

/**
 * @brief
 * rmdir directories recursively until 256 depth
 * should succeed.
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest14(void);

/**
 * @brief
 * rmdir file whose direntry which resides in triple entry in parent directory
 * @return 1 if successful, 0 otherwise
 */
error_t rmdirTest15(void);


#endif /*RMDIRTESTS_H_*/
