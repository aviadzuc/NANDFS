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

/** @file increment_counterTests.c  */
#include <test/sequencing/increment_counterTests.h>
#include <test/sequencing/testsHeader.h>

/**
 * run all increment_counter tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllincrement_counterTests(){
#ifndef EXACT_COUNTERS	
	RUN_TEST(increment_counter, 1);
	RUN_TEST(increment_counter, 2);
	RUN_TEST(increment_counter, 3);
	RUN_TEST(increment_counter, 4);
#else
	RUN_TEST(increment_counter, 5);	
#endif
	return 0;
}

/**
 * @brief
 * tear down increment_counter test
 * @return 1 if succesful, 0 otherwise
 */
error_t init_increment_counterTest(){
	uint32_t i;
	
	if(nandInit())
		return -1;		
	
	for(i=0; i< SEQ_OBS_COUNTERS; i++){
#ifndef EXACT_COUNTERS		
		change_counter_level(OBS_COUNT_LEVEL_0, i);
#else
		INIT_EXACT_COUNTER(i);
		INC_EXACT_COUNTER(i);
#endif
	}	

	return 1;
}

/**
 * @brief
 * tear down increment_counter test
 * @return 1 if successful, 0 otherwise
 */
error_t tearDown_increment_counterTest(){
	uint32_t i;
	
	for(i=0; i< SEQ_OBS_COUNTERS; i++){
#ifndef EXACT_COUNTERS		
		change_counter_level(OBS_COUNT_NO_OBSOLETES, i);
#else
		INIT_EXACT_COUNTER(0);
#endif
	}
		
	nandTerminate();	

	return 1;
}

/**
 * @brief 
 * verify initialization of counters was successful.
 * change one counter and verify it was indeed changed
 */	
error_t increment_counterTest1(){			
	uint32_t i, seg_id = 5;	
	
	for(i=0; i< SEQ_OBS_COUNTERS; i++){					
		VERIFY(COMPARE(get_counter_level(seg_id), OBS_COUNT_LEVEL_0));		
	}
	
	increment_counter(seg_id);
	VERIFY(COMPARE(get_counter_level(seg_id), OBS_COUNT_LEVEL_1));
	
	return 1;
}

/**
 * @brief 
 * increase adjacent counters to various levels and verify they were indeed incremented properly
 * change one counter and verify it was indeed changed
 */	
error_t increment_counterTest2(){			
	uint32_t seg_id0 = 4, seg_id1 = seg_id0+1;		
		
	increment_counter(seg_id0);
	VERIFY(COMPARE(get_counter_level(seg_id0), OBS_COUNT_LEVEL_1));	
	increment_counter(seg_id0);
	VERIFY(COMPARE(get_counter_level(seg_id0), OBS_COUNT_LEVEL_2));	
	
	increment_counter(seg_id1);
	VERIFY(COMPARE(get_counter_level(seg_id1), OBS_COUNT_LEVEL_1));	
	
	increment_counter(seg_id1);
	VERIFY(COMPARE(get_counter_level(seg_id1), OBS_COUNT_LEVEL_2));
	
	// verify other counter hasn't changed	
	VERIFY(COMPARE(get_counter_level(seg_id0), OBS_COUNT_LEVEL_2));	
	VERIFY(COMPARE(get_counter_level(seg_id0-1), OBS_COUNT_LEVEL_0));	
	VERIFY(COMPARE(get_counter_level(seg_id1+1), OBS_COUNT_LEVEL_0));	
	
	return 1;
}

/**
 * @brief 
 * increment counter to maximum level, verify it was incremented,
 * and cant be incremented anymore
 */	
error_t increment_counterTest3(){			
	uint32_t i, seg_id0 = 4;
	
	for(i=0; i<8; i++)	
		increment_counter(seg_id0);
	VERIFY(COMPARE(get_counter_level(seg_id0), OBS_COUNT_LEVEL_7));		
	
	for(i=0; i<8; i++)	
		increment_counter(seg_id0);
	VERIFY(COMPARE(get_counter_level(seg_id0), OBS_COUNT_LEVEL_7));		
	
	return 1;
}

/**
 * @brief 
 * increment all counters, and verify all were incremented
 * @return 1 if successful, 0 otherwise
 */	
error_t increment_counterTest4(){			
	uint32_t i;
	
	for(i=0; i<SEQ_OBS_COUNTERS; i++)	
		increment_counter(i);
		
	for(i=0; i<SEQ_SEGMENTS_COUNT; i++){		
//		print(uart0SendByte,0,"\ncounter ");
//		printNum(uart0SendByte,0,i);
//		print(uart0SendByte,0," = ");
//		printNum(uart0SendByte,0,get_counter_level(i));
		VERIFY(COMPARE(get_counter_level(i), OBS_COUNT_LEVEL_1));			
	}
	
	return 1;
}
#ifdef EXACT_COUNTERS
/**
 * @brief 
 * verify initialization of counters was successful.
 * change one counter and verify it was indeed changed
 */	
error_t increment_counterTest5(){			
	uint32_t i, seg_id = 5;	
	
	for(i=0; i< SEQ_OBS_COUNTERS; i++){					
		VERIFY(COMPARE(get_counter_level(seg_id), 1));		
	}
//	PRINT_MSG_AND_NUM("\nb4 incrementing level=", get_counter_level(seg_id));
	INC_EXACT_COUNTER(seg_id);
//	PRINT_MSG_AND_NUM("\nafteer incrementing level=", get_counter_level(seg_id));
	VERIFY(COMPARE(get_counter_level(seg_id), 2));
	
	return 1;
}
#endif
