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

#ifndef LOGICALADDRESSTORESERVEPHYSICALTESTS_H_
#define LOGICALADDRESSTORESERVEPHYSICALTESTS_H_

#include <system.h>

/**
 * run all logicalAddressToPhysical tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllLogicalAddressToReservePhysicalTests(void);

/**
 * @brief
 * init logicalAddressToReservePhysical test
 * @return 1 if succesful, 0 otherwise
 */
error_t init_logicalAddressToReservePhysicalTest(void);
	
/**
 * @brief
 * tear down logicalAddressToReservePhysical test
 * @return 1 if succesful, 0 otherwise
 */
error_t tearDown_logicalAddressToReservePhysicalTest(void);


/**
 * @brief
 * mark an eu as a reserve of some eu, and try reading it
 * write to phy_addr a reserve eu replacing the eu of orig_phy_addr 
 * and try to calculate it again using logicalAddressToReservePhysical
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToReservePhysicalTest1(void);

/**
 * @brief
 * mark 2 eus as a reserve of 2 other eu's, and try reading the second one
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToReservePhysicalTest2(void);

/**
 * @brief
 * mark 3 eus as a reserve of 3 other eu's, and try reading the second one,
 * then the third one
 * @return 0 if succesful, 1 otherwise
 */
error_t logicalAddressToReservePhysicalTest3(void);

/**
 * @brief
 * write SEQ_EUS_PER_SLOT reserve eu's. this will write
 * SEQ_EUS_PER_SLOT-1 reserve eu's to the first segment, and 1 to the next
 * verify one of the first EUs, and one in the second reserve segment
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToReservePhysicalTest4(void);

/**
 * @brief
 * 3 consecutice calls
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToReservePhysicalTest5(void);

/**
 * @brief
 * SEQ_EUS_PER_SLOT consecutive calls
 * @return 0 if succesful, 1 otherwise
 */
error_t logicalAddressToReservePhysicalTest6(void);

#endif /*LOGICALADDRESSTORESERVEPHYSICALTESTS_H_*/
