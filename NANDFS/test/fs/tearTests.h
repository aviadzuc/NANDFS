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

#ifndef FSBOOTINGTESTS_H_
#define FSBOOTINGTESTS_H_

#include <system.h>

void runAllfsBootingTests(void);

/**
 * @brief 
 * init fsBooting test
 */
void init_fsBootingTest(void);

/**
 * @brief 
 * tear down fsBooting test
 */
void tearDown_fsBootingTest(void);
	
/**
 * @brief
 * boot empty flash.
 * verify inode0, root directory exists. 
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest1(void);

/**
 * @brief
 * booting failed before creating inode0. verify recovery
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest3(void);

/**
 * @brief
 * in allocation mode - creat files write to them, close.
 * verify on reboot we return state where all files are empty 
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest4(void);

/**
 * @brief
 * in reclamation mode -  creat file write , close.
 * verify on reboot we return to same state
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest5(void);

/**
 * @brief
 * in reclamation mode -  creat files write to them, close, re-open and write uncommited data.
 * verify on reboot we return to last commited state
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest6(void);

/**
 * @brief
 * crash in the middle of writing, verify success
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest7(void);

/**
 * @brief
 * crash just before commiting transaction at the end of creat. 
 * verify returning to stable state (file doesn't exist)
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest9(void);

/**
 * @brief
 * crash right after writing new checkpoint at the end of creat. 
 * verify returning to stable state (file exists)
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest10(void);

/**
 * @brief
 * crash during unlink. 
 * verify returning to stable state (file exists) 
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest11(void);

/**
 * @brief
 * crash during unlink, right after writing new checkpoint. 
 * verify returning to stable state (file exists)
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest12(void);

/**
 * @brief
 * write to file and close. crash in the middle of handling transaction vots.
 * verify transaction completes upon reboot.
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest13(void);

/**
 * @brief
 * unlink file and close. crash in the middle of handling transaction vots.
 * verify transaction completes upon reboot (file doesn't exist)
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest14(void);

/**
 * @brief
 * write checkpoint when we have an open transaction, and verify
 * that we abort it when we reboot
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest15(void);

/**
 * @brief
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest16(void);

/**
 * @brief
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest17(void);

/**
 * @brief
 * 
 * @return 1 if successful, 0 otherwise
 */
error_t fsBootingTest18(void);


#endif /*FSBOOTINGTESTS_H_*/
