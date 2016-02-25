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

/** @file nightlySequencingTests.c  */
#include <test/sequencing/nightlySequencingTests.h>
#include <test/sequencing/testsHeader.h>
#include <test/sequencing/bit_generator_mechanismTests.h>
#include <test/sequencing/markAsObsoleteTests.h>

/* run these seldom becuase they takes a long time... */
error_t runAllNightlySequencingTests(){
//	PRINT("\nrun nightly tests");
	RUN_TEST(bit_generator_mechanism,1);
	RUN_TEST(markAsObsolete, 6);
	
	return 0;
}

