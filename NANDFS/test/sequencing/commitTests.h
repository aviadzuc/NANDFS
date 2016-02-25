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

#ifndef COMMITTESTS_H_
#define COMMITTESTS_H_

#include <system.h>

/**
 * run all Commit tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllCommitTests(void);

/**
 * @brief 
 * init commit test
 */
error_t init_commitTest(void);				

/**
 * @brief 
 * tear down commit test
 */
error_t tearDown_commitTest(void);

/**
 * @brief
 * write a checkpoint as part a segment header and verify it was indeed written - 
 * compare expected and read values of segment map
 * assumes that the checkpoint is under one page
 */
error_t commitTest1(void);

/**
 * @brief
 * write 2 checkpoints,one as part a segment header, and one afterwords with changes.
 * verify the first was indeed written - compare expected and read values of segment map
 * assumes that the checkpoint is under one page
 */
error_t commitTest2(void);

/**
 * @brief
 * write a checkpoint as part a segment header and verify it was indeed written - 
 * compare expected and read values of segment map
 * checkpoint is more then 1 page now
 */
error_t commitTest3(void);

/**
 * @brief
 * write a checkpoint as part a segment header, the first EU of the segment is bad.
 * verify it was indeed written - compare expected and read values of segment map
 * checkpoint is more then 1 page now
 * @return 1 if successful. if the commit failed 0 is returned.
 * if retrieved checkpoint data is not as expected 0 is returned
 */
error_t commitTest4(void);

/**
 * @brief
 * write a checkpoint which is larger then one page to the last page of a segment - 
 * verify the commit was successful
 * @return 1 if successful, 0 if the commit failed
 * 
 */
error_t commitTest5(void);

#endif /*COMMITTESTS_H_*/
