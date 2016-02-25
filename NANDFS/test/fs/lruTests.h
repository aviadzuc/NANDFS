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

#ifndef LRUTESTS_H_
#define LRUTESTS_H_

#include <system.h>

void runAllLruTests();

/**
 * @brief
 * init lru test
 *
 */
void
init_lruTest();

/**
 * @brief
 * tear down lru test
 *
 */
void
tearDown_lruTest();

/**
 * @brief
 * read inode of a file that's NOT part of a transaction. verify
 * - insertion to lru
 * - page is marked as read page
 * - no parent offset and id
 * - no tid
 * @return 1 if successful, 0 otherwise
 */
error_t lruTest1();

error_t lruTest2();

error_t lruTest3();

error_t lruTest4();

error_t lruTest5();

error_t lruTest6();

error_t lruTest7();

error_t lruTest8();

error_t lruTest9();

#endif /*LRUTESTS_H_*/
