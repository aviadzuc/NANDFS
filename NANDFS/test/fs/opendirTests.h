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

#ifndef OPENDIRTESTS_H_
#define OPENDIRTESTS_H_

#include <system.h>

void runAllOpendirTests(void);

/**
 * @brief 
 * init opendir test
 * 
 */
void init_opendirTest(void);

/**
 * @brief 
 * tear down opendir test
 * 
 */
void tearDown_opendirTest(void);

/**
 * @brief
 * open an existing directory for reading.
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest1(void);

/**
 * @brief
 * open a directory that doesn't exists for reading.
 * should fail
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest2(void);

/**
 * @brief
 * open existing directory twice from 2 different users
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest5(void);

/**
 * @brief
 * open a file for writing, then try to open it's directory for reading
 * should succeed
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest6(void);

/**
 * @brief
 * open a directory that exists for reading, the try to unlink a file in it
 * should fail (write exclusive)
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest7(void);

/**
 * @brief
 * open FS_MAX_OPEN_DIRESTREAMS different directories for reading , then another directory and making sure
 * the last open fails with the matching error 
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest8(void);

/**
 * @brief
 * open FS_MAX_VNODES different files for reading , then try to open a directory.
 * should fail.
 * @return 1 if successful, 0 otherwise
 */
error_t opendirTest9(void);


#endif /*OPENDIRTESTS_H_*/
