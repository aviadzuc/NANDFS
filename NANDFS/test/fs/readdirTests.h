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

#ifndef READDIRTESTS_H_
#define READDIRTESTS_H_

#include <system.h>

void runAllReaddirTests(void);

/**
 * @brief 
 * init readdir test
 * 
 */
void init_readdirTest(void);

/**
 * @brief 
 * tear down readdir test
 * 
 */
void tearDown_readdirTest(void);

/**
 * @brief
 * open directory and read readdir. should return "." entry.
 * keep reading until finished 
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest1(void);

/**
 * @brief
 * open directory and read readdir. than do another readdir, where the next direntry is in 
 * a direct entry. should succeed 
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest2(void);

/**
 * @brief
 * open directory and read readdir. than do another readdir, where the next direntry is in 
 * an indirect entry. should succeed 
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest3(void);

/**
 * @brief
 * open directory and read readdir. than do another readdir, where the next direntry is in 
 * a double entry. should succeed 
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest4(void);

/**
 * @brief
 * open directory and read readdir. than do another readdir, where the next direntry is in 
 * an triple entry. should succeed  
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest5(void);

/**
 * @brief
 * try to read from an illegal dirstream. should fail 
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest7(void);

/**
 * @brief
 * try to readdir a directory opened by another user. should fail. 
 * @return 1 if successful, 0 otherwise
 */
error_t readdirTest8(void);


#endif /*READDIRTESTS_H_*/
