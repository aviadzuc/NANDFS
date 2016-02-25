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

#ifndef VERIFYECCTESTS_H_
#define VERIFYECCTESTS_H_

#include <system.h>

/**
 * run all verifyECC tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllVerifyECCTests(void);

/**
 * @brief 
 * init verifyECC test
 */
error_t init_verifyECCTest(void);			

/**
 * @brief 
 * tear down verifyECC test
 */
error_t tearDown_verifyECCTest(void);

/**
 * @brief 
 * test verifyECC - calculate ecc for data buffer, verify recalculation succeeds,
 * @return 1 if successful, 0 otherwise
 */
error_t verifyECCTest1(void);

/**
 * @brief 
 * test verifyECC - calculate ecc for data buffer, make it erroneous.
 * verify recalculation fails
 * @return 1 if successful, 0 otherwise
 */
error_t verifyECCTest2(void);

/**
 * @brief 
 * test verifyECC - calculate ecc for data buffer, make it erroneous.
 * verify recalculation succeeds
 * @return 1 if successful, 0 otherwise
 */
error_t verifyECCTest3(void);

#endif /*VERIFYECCTESTS_H_*/
