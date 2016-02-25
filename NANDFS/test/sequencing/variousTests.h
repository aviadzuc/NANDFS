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

#ifndef VARIOUSTESTS_H_
#define VARIOUSTESTS_H_

#include <system.h>

/**
 * run all various tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllVariousTests(void);

/**
 * @brief 
 * init nandReadPageSpare test
 */
error_t init_nandReadPageSpareTest(void);	

/**
 * @brief 
 * tear down nandReadPageSpare test
 */
error_t tearDown_nandReadPageSpareTest(void);

error_t nandReadPageSpareTest1(void);

/**
 * @brief 
 * init bit_fields test
 */
error_t init_bit_fieldsTest(void);		

/**
 * @brief 
 * tear down bit_fields test
 */
error_t tearDown_bit_fieldsTest(void);

error_t bit_fieldsTest1(void);

#endif /*VARIOUSTESTS_H_*/
