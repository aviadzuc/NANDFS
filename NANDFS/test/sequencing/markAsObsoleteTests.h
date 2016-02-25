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

#ifndef MARKASOBSOLETETESTS_H_
#define MARKASOBSOLETETESTS_H_

#include <system.h>

/**
 * run all markAsObsolete tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllMarkAsObsoleteTests(void);

/**
 * @brief
 * init markAsObsolete test
 * @return 1 if succesful, 0 otherwise
 */
error_t init_markAsObsoleteTest(void);

/**
 * @brief
 * tear down markAsObsolete test
 * @return 1 if succesful, 0 otherwise
 */
error_t tearDown_markAsObsoleteTest(void);

/**
 * @brief
 * mark a page as obsolete and check if it done
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest0(void);

/**
 * @brief
 * mark a page as obsolete and check if it done
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest1(void);

/**
 * @brief
 * mark EU as bad, allocate a reserve EU replacing it
 * and mark a page in original EU as obsolete, verify that the page in the reserve
 * EU was marked obsolete
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest2(void);

/**
 * @brief
 * mark 2 pages as obsolete and check if it done
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest3(void);

/**
 * @brief
 * mark 2 pages as obsolete and check if it done, verify pages near them are not obsolete
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest4(void);

/**
 * @brief
 * mark a page as obsolete and check if counter was incremented
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest5(void);

/**
 * @brief
 * mark a page as obsolete and check if counter was incremented
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest6(void);

/**
 * @brief
 * in regulat allocation mode mark pages as obsolete, and verify obsolete pages count increase 
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest7(void);

/**
 * @brief
 * mark a page as obsolete and check counter incremented
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest8();

/**
 * @brief
 * mark mutiple page as obsolete and check counter was incremented properly
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest9();
#endif /*MARKASOBSOLETETESTS_H_*/
