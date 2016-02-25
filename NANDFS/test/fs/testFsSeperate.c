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

/** @file testFsSeperate.c  */
#include <system.h>
#include <test/fs/testsHeader.h>

////#include <test/fs/lruTests.h>
////#include <test/fs/auxiliaryFuncsTests.h>
////#include <test/fs/openTests.h>
//#include <test/fs/creatTests.h>
//#include <test/fs/writeTests.h>
#include <test/fs/closeTests.h>
#include <test/fs/readTests.h>
#include <test/fs/unlinkTests.h>
#include <test/fs/mkdirTests.h>
#include <test/fs/rmdirTests.h>
#include <test/fs/fsyncTests.h>
#include <test/fs/opendirTests.h>
#include <test/fs/readdirTests.h>
#include <test/fs/variousFuncsTests.h>
#include <test/fs/scandirTests.h>
#include <test/fs/tearTests.h>

//#include <test/fs/ecosTests.h>

void runAllFsTests(void){
#ifdef ECOSTESTS_H_
	runAllEcosTests();
#endif

#ifdef LRUTESTS_H_
	runAllLruTests();
#endif

#ifdef AUXILIARYFUNCSTESTS_H_
	runAllAuxiliaryFuncsTests();
#endif
#ifdef OPENTESTS_H_
	runAllOpenTests();
#endif
#ifdef CREATTESTS_H_
	runAllCreatTests();
#endif
#ifdef WRITETESTS_H_
	runAllwriteTests();
#endif
#ifdef CLOSETESTS_H_
	runAllcloseTests();
#endif
#ifdef READTESTS_H_
	runAllreadTests();
#endif
#ifdef UNLINKTESTS_H_
	runAllunlinkTests();
#endif
#ifdef FSBOOTINGTESTS_H_
	runAllfsBootingTests();
#endif
#ifdef MKDIRTESTS_H_
	runAllMkdirTests();
#endif
#ifdef RMDIRTESTS_H_
	runAllRmdirTests();
#endif
#ifdef FSYNCTESTS_H_
	runAllfsyncTests();
#endif
#ifdef OPENDIRTESTS_H_
	runAllOpendirTests();
#endif
#ifdef READDIRTESTS_H_
	runAllReaddirTests();
#endif
#ifdef VARIOUSFUNCSTESTS_H_
	runAllVariousFuncsTests();
#endif
#ifdef SCANDIRTESTS_H_
	runAllScandirTests();
#endif
#ifdef NightlyTests
	runAllNightlyFsTests();
#endif

}
