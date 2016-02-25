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

#ifndef READBLOCKTESTS_H_
#define READBLOCKTESTS_H_

#include <system.h>

/**
 * run all readBlock tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllReadBlockTests(void);

/**
 * @brief 
 * init readBlock test
 */
error_t init_readBlockTest(void);					

/**
 * @brief 
 * tear down readBlock test
 */
error_t tearDown_readBlockTest(void);

/**
 * @brief
 * write a block, read it and verify compared read is as expected
 * @return 1 if test was successful
 */
error_t readBlockTest1(void);

/**
 * @brief
 * write a block to a logical address whose physical address is corrupt,
 * read it and verify compared read is as expected
 * @return 1 if test was successful
 */
error_t readBlockTest2(void);

/**
 * @brief
 * write 2 blocks with different data
 * read and verify compared read is as expected
 * @return 1 if test was successful
 */
error_t readBlockTest3(void);

#endif /*READBLOCKTESTS_H_*/
