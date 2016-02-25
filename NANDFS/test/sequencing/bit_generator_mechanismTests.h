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

#ifndef BIT_GENERATOR_MECHANISMTESTS_H_
#define BIT_GENERATOR_MECHANISMTESTS_H_

#include <system.h>

/**
 * run all bit_generator_mechanism tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllbit_generator_mechanismTests(void);

error_t init_bit_generator_mechanismTest(void);

error_t tearDown_bit_generator_mechanismTest(void);

/**
 * @brief 
 * test that our random bit generator indeed repeats otself only after 2^32-1 times
 * @return 1 if successful, 0 otherwise
 */
uint32_t bit_generator_mechanismTest1(void);

#endif /*BIT_GENERATOR_MECHANISMTESTS_H_*/
