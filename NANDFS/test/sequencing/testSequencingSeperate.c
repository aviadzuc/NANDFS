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

/** @file testSequencingSeperate.c  */
#include <system.h>
#include <test/sequencing/testsHeader.h>

#include <test/sequencing/logicalAddressToReservePhysicalTests.h>
//#include <test/sequencing/logicalAddressToPhysicalTests.h>
//#include <test/sequencing/increment_counterTests.h>
//
//#include <test/sequencing/markAsObsoleteTests.h>
//#include <test/sequencing/performWearLevelingTests.h>
//#include <test/sequencing/allocAndWriteBlockTests.h>
//
//#include <test/sequencing/readBlockTests.h>
//#include <test/sequencing/commitTests.h>
//#include <test/sequencing/findCheckpointAndTruncateTests.h>
//#include <test/sequencing/readVOTsAndPrevTests.h>//
//
//#include <test/sequencing/variousTests.h>
//#include <test/sequencing/sequencingBootingTests.h>
//#include <test/sequencing/verifyECCTests.h>
//
//#include <test/sequencing/markAsObsoleteTests.h>
//
//#include <test/sequencing/bit_generator_mechanismTests.h>
//#include <test/sequencing/nightlySequencingTests.h>

/**
 * run all sequencing tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllSequencingTests(void){
//	PRINT("\nabout to run tests");	
#ifdef LOGICALADDRESSTORESERVEPHYSICALTESTS_H_
	runAllLogicalAddressToReservePhysicalTests();
#endif

#ifdef LOGICALADDRESSTOPHYSICALTESTS_H_	
	runAllLogicalAddressToPhysicalTests();
#endif

#ifdef INCREMENT_COUNTERTESTS_H_
	runAllincrement_counterTests();
#endif
	
#ifdef MARKASOBSOLETETESTS_H_	
	runAllMarkAsObsoleteTests();
#endif
	
#ifdef	READBLOCKTESTS_H_
	runAllReadBlockTests();
//	PRINT("\nfinished read tests");
#endif
#ifdef	PERFORMWEARLEVELINGTESTS_H_
	runAllPerformWearLevelingTests();
#endif

#ifdef COMMITTESTS_H_	
	runAllCommitTests();
#endif

#ifdef	FINDCHECKPOINTANDTRUNCATETESTS_H_
	runAllfindCheckpointAndTruncateTests();
#endif

#ifdef	READVOTSANDPREVTESTS_H_	
	runAllReadVOTsAndPrevTests();
#endif

#ifdef ALLOCANDWRITEBLOCKTESTS_H_	
	runAllallocAndWriteBlockTests();
#endif	
	
#ifdef VARIOUSTESTS_H_
	runAllVariousTests();
#endif

#ifdef SEQUENCINGBOOTINGTESTS_H_			
	runAllSequencingBootingTests();
#endif

#ifdef VERIFYECCTESTS_H_
#ifdef DO_ECC
	runAllVerifyECCTests();
#endif	
#endif

#ifdef NightlyTests
	runAllNightlySequencingTests();
#endif				
	return 0;
}
