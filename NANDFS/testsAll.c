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

/** @file testsAll.c
 * main file for the file system tests*/
#include <test/sequencing/testSequencingSeperate.h>
//#include <test/fs/testFsSeperate.h>

#include <system.h>

int main(void){
	assert(sizeof(uint8_t)  == 1);
	assert(sizeof(uint16_t) == 2);
	assert(sizeof(uint32_t) == 4);

#ifdef TESTSEQUENCINGSEPERATE_H_
	runAllSequencingTests();
#endif

#ifdef TESTSFSSEPERATE_H_
	runAllFsTests();
#endif

	return 0;
}


