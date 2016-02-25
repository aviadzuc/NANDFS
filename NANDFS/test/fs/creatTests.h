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

#ifndef CREATTESTS_H_
#define CREATTESTS_H_

#include <system.h>

void runAllCreatTests(void);

/**
 * @brief 
 * init creat test
 * 
 */
void init_creatTest(void);

/**
 * @brief 
 * tear down creat test
 * 
 */
void tearDown_creatTest(void);

/**
 * @brief
 * create a file in root node, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest1(void);

/**
 * @brief
 * create a file in root node that already exists, and verify failure
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest2(void);

/**
 * @brief
 * create a file not in root node, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest3(void);

/**
 * @brief
 * create a file not in root node that already exists, and verify afilure
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest4(void);

/**
 * @brief
 * create a file in root node, when we have no free direentries to use and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest6(void);

/**
 * @brief
 * create a file not in root directory "directory1/directory2/file.dat/". should fail
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest7(void);

/**
 * @brief
 * create a file in a directory open for reading. should fail
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest8(void);

/**
 * @brief
 * create a file "/file.dat/". should fail
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest9(void);

/**
 * @brief
 * create 2 files root node, and verify creation
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest10(void);

/**
 * @brief
 * create file with long name. verify creation, and that direntry is marked as not free
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest11(void);

/**
 * @brief
 * create a file whose inode is in indirect offset in inode0
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest20(void);

/**
 * @brief
 * create a file whose inode is in double offset in inode0
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest21(void);

/**
 * @brief
 * create a file whose inode is in triple offset in inode0
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t creatTest22(void);


#endif /*CREATTESTS_H_*/
