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

#ifndef LOGICALADDRESSTOPHYSICALTESTS_H_
#define LOGICALADDRESSTOPHYSICALTESTS_H_

#include <system.h>

/**
 * run all logicalAddressToPhysical tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllLogicalAddressToPhysicalTests(void);

/**
 * @brief
 * tear down logicalAddressToPhysical test
 * @return 1 if succesful, 0 otherwise
 */
error_t init_logicalAddressToPhysicalTest(void);

/**
 * @brief
 * tear down logicalAddressToPhysical test
 * @return 1 if successful, 0 otherwise
 */
error_t tearDown_logicalAddressToPhysicalTest(void);

/**
 * call once 
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToPhysicalTest2(void);

/**
 * call twice
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToPhysicalTest3(void);

/**
 * call once, for an address in a bad page
 * @return 0 if succesful, 1 otherwise
 */
error_t logicalAddressToPhysicalTest4(void);

/**
 * @brief
 * test that reading from a page in a reclaimed address is found properly
 * @return 0 if succesful, 1 otherwise
 */
error_t logicalAddressToPhysicalTest6(void);

/**
 * @brief
 * call once, for an address in a supposedly in reclaimed page, but the state is not reclamation
 * @return 0 if succesful, 1 otherwise
 */
error_t logicalAddressToPhysicalTest7(void);

#endif /*LOGICALADDRESSTOPHYSICALTESTS_H_*/
