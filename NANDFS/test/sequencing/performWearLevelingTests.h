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

#ifndef PERFORMWEARLEVELINGTESTS_H_
#define PERFORMWEARLEVELINGTESTS_H_

#include <system.h>

/**
 * run all performWearLeveling tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllPerformWearLevelingTests(void);

/**
 * @brief 
 * init performWearLeveling test
 */
error_t init_performWearLevelingTest(void);		

/**
 * @brief 
 * tear down performWearLeveling test
 */
error_t tearDown_performWearLevelingTest(void);

/**
 * @brief
 * perform wear leveling on a slot with 0 bad EU's to a slot with 0 bad EU's
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest1(void);

/**
 * @brief
 * perform wear leveling on a slot with 1 bad EU to a slot with 0 bad EU's
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest2(void);

/**
 * @brief
 * perform wear leveling on a slot with 0 bad EU to a slot with 1 bad EU
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest3(void);

/**
 * @brief
 * perform wear leveling on a slot with 1 bad EU to a slot with 1 bad EU
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest4(void);

/**
 * @brief
 * perform wear leveling on a slot with 2 consecutive bad EU's to a slot with 2 consecutive bad EU
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest5(void);

/**
 * @brief
 * perform wear leveling on a reserve slot with no bad EU's to a slot with no bad EU's
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest6(void);

/**
 * @brief
 * perform wear leveling on a reserve slot with 1 bad EU's to a slot with 1 bad EU's
 * @return 1 if the test was successful, 0 otherwise
 */
error_t performWearLevelingTest7(void);

#endif /*PERFORMWEARLEVELINGTESTS_H_*/
