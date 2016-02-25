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

/** @file markAsObsoleteTests.c  */
#include <test/sequencing/markAsObsoleteTests.h>
#include <test/sequencing/testsHeader.h>

/**
 * run all markAsObsolete tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllMarkAsObsoleteTests(){

	RUN_TEST(markAsObsolete, 0);	
	RUN_TEST(markAsObsolete, 1);
	RUN_TEST(markAsObsolete, 2);
	RUN_TEST(markAsObsolete, 3);
	RUN_TEST(markAsObsolete, 4);
#ifndef EXACT_COUNTERS		
//	RUN_TEST(markAsObsolete, 5);
//	RUN_TEST(markAsObsolete, 7);
#else
	RUN_TEST(markAsObsolete, 8);
	RUN_TEST(markAsObsolete, 9);
#endif	
	return 0;
}

/**
 * @brief
 * init markAsObsolete test
 * @return 1 if succesful, 0 otherwise
 */
error_t init_markAsObsoleteTest(){
	if(nandInit())
		return 0;		
			
	init_flash();	
	init_seg_map();	
	init_obs_counters()	;
		
	init_obs_byte();
//	PRINT("\ninit_markAsObsoleteTest()- ");
	mark_reserve_segment_headers();
	
	return 1;
}	

/**
 * @brief
 * tear down markAsObsolete test
 * @return 1 if succesful, 0 otherwise
 */
error_t tearDown_markAsObsoleteTest(){	
	init_seg_map();
	init_flash();		
	init_obs_counters();	
	nandTerminate();	
	
	return 1;
}

/**
 * @brief
 * check that a regular page is not marked as obsolete
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest0(){		
	uint32_t res, seg = 5, page_offset = 120;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
	init_logical_address(log_address);
	
	seg_map_ptr->seg_to_slot_map[seg] = 1;	
	SET_LOGICAL_SEGMENT(log_address, seg);
	SET_LOGICAL_OFFSET(log_address, page_offset);

	res = !is_page_marked_obsolete(logicalAddressToPhysical(log_address));
	
	return res;
}

/**
 * @brief
 * mark a page as obsolete and check if it done
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest1(){		
	uint32_t res, seg = 5, page_offset = 120;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
	init_logical_address(log_address);
	
	seg_map_ptr->seg_to_slot_map[seg] = 1;	
	
//	uint8_t buf[NAND_TOTAL_SIZE];
//	int32_t i;
//	
//	init_buf(buf);
//	for(i=0; i< NAND_PAGE_SIZE; i++){
//		buf[i] = 'a';
//	}
	
	SET_LOGICAL_SEGMENT(log_address, seg);
	SET_LOGICAL_OFFSET(log_address, page_offset);
	
	if(markAsObsolete(log_address, MARK_OBSOLETE_NOT_AFTER_REBOOT)){
		PRINT("\nfailed marking page as obsolete ");
		return 0;	
	}	 	
	 		
	res = is_page_marked_obsolete(logicalAddressToPhysical(log_address));
	
//	nandReadPageTotal(buf, logicalAddressToPhysical(log_address));
//	for(i=0; i< NAND_TOTAL_SIZE; i++){
//		PRINT_MSG_AND_NUM("\n", i);
//		PRINT_MSG_AND_HEX(". ", buf[i]);
//	}
	
	return res;
}

/**
 * @brief
 * mark EU as bad, allocate a reserve EU replacing it
 * and mark a page in original EU as obsolete, verify that the page in the reserve
 * EU was marked obsolete
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest2(){		
	uint32_t seg = 5, slot_id = 5, seg_eu_offset = 3, eu_page_offset = 20;
	uint32_t phy_addr, orig_phy_addr;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	initFlags(flags);
	
	init_logical_address(log_address);
	
	seg_map_ptr->seg_to_slot_map[seg] = slot_id;	
	orig_phy_addr = CALC_ADDRESS(slot_id, seg_eu_offset, eu_page_offset);
	phy_addr      = write_to_reserve_eu(orig_phy_addr);
	// phy_addr is of the reserve eu only, so add the page_offset
	phy_addr += eu_page_offset;
	
	SET_LOGICAL_SEGMENT(log_address, seg);
	SET_LOGICAL_OFFSET(log_address, CALC_SLOT_OFFSET(seg_eu_offset,eu_page_offset));
	
	// mark original eu as bad
//	PRINT("\nmakr eu mock bad");
	VERIFY_NOT(markEuAsMockBad(orig_phy_addr));
	
//	PRINT("\nmakr obsolete");
	// do mark as obsolete on matching logical address
	if(markAsObsolete(log_address, MARK_OBSOLETE_NOT_AFTER_REBOOT))
		return 0;	
	
//	PRINT("\nverify");
	// verify that indeed the page in the reserve eu was marked as obsolete		
	VERIFY(is_page_marked_obsolete(logicalAddressToReservePhysical(flags, orig_phy_addr)));		
	// and verify that when not specifying we are searching for the reserve address, it still returns it
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(log_address)));
		
	return 1;
}

/**
 * @brief
 * mark 2 pages as obsolete and check if it done
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest3(){		
	uint32_t slot0 = 30, seg0 = 5, page_offset0 = 30, slot1 = 5, seg1 = 8, page_offset1 = 42;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address0);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address1);		
	
	init_logical_address(log_address0);
	init_logical_address(log_address1);
	
	seg_map_ptr->seg_to_slot_map[seg0] = slot0;
	seg_map_ptr->seg_to_slot_map[seg1] = slot1;
	
	SET_LOGICAL_ADDRESS(log_address0, seg0, page_offset0);
	SET_LOGICAL_ADDRESS(log_address1, seg1, page_offset1);	
	
	if(markAsObsolete(log_address0, MARK_OBSOLETE_NOT_AFTER_REBOOT))
		return 0;	
//	PRINT("\n1st markAsObsolete() success");
	if(markAsObsolete(log_address1, MARK_OBSOLETE_NOT_AFTER_REBOOT))
		return 0;			
//	PRINT("\n2nd markAsObsolete() success");
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(log_address0)));		
//	PRINT("\n1st obsolete success");
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(log_address1)));
//	PRINT("\n2nd obsolete success");
		
	return 1;
}

/**
 * @brief
 * mark 2 pages as obsolete and check if it done, verify pages near them are not obsolete
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest4(){		
	uint32_t slot0 = 30, seg0 = 5, page_offset0 = 30, slot1 = 5, seg1 = 8, page_offset1 = 42;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address0);	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address1);	

	seg_map_ptr->seg_to_slot_map[seg0] = slot0;
	seg_map_ptr->seg_to_slot_map[seg1] = slot1;	
		
	init_logical_address(log_address0);
	init_logical_address(log_address1);
	
	SET_LOGICAL_ADDRESS(log_address0, seg0, page_offset0);
	SET_LOGICAL_ADDRESS(log_address1, seg1, page_offset1);		
	
	if(markAsObsolete(log_address0, MARK_OBSOLETE_NOT_AFTER_REBOOT))
		return 0;

	if(markAsObsolete(log_address1, MARK_OBSOLETE_NOT_AFTER_REBOOT))
		return 0;	
		
	VERIFY_NOT(is_page_marked_obsolete(logicalAddressToPhysical(log_address0)));		
	VERIFY_NOT(is_page_marked_obsolete(logicalAddressToPhysical(log_address1)));
	
	// verify other addresses are not obsolete
	SET_LOGICAL_ADDRESS(log_address0, seg0, page_offset0+1);
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(log_address1)));	
	
	SET_LOGICAL_ADDRESS(log_address0, seg0, page_offset0-10);
	VERIFY(is_page_marked_obsolete(logicalAddressToPhysical(log_address1)));	
				
	return 1;
}

/**
 * @brief
 * mark a page as obsolete and check if counter was incremented
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest5(){		
	uint32_t seg = 5, page_offset = 120;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
	seg_map_ptr->seg_to_slot_map[seg] = 1;		
	init_logical_address(log_address);
	
	SET_LOGICAL_SEGMENT(log_address, seg);
	SET_LOGICAL_OFFSET(log_address, page_offset);
	
	if(markAsObsolete(log_address, MARK_OBSOLETE_NOT_AFTER_REBOOT)){
		print(uart0SendByte,0,"\nfailed marking page as obsolete "); 
		return 0;	
	}	 	
	 		
	VERIFY(COMPARE(OBS_COUNT_LEVEL_0, get_counter_level(GET_LOGICAL_SEGMENT(log_address))));
	
	return 1;
}

/**
 * @brief
 * mark a page as obsolete and check if counter was incremented
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest6(){		
	uint32_t j,i, segs_to_mark = 30, obs_counter_level = 0;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
	/* mark segments in seg_map */
	for(i = 0; i< segs_to_mark; i++){
		seg_map_ptr->seg_to_slot_map[i] = i;
	} 
	
	/* mark all pages in those segments as obsolete*/
	for(i = 0; i< segs_to_mark; i++){
		SET_LOGICAL_SEGMENT(log_address, i);
		for(j=0;j< SEQ_PAGES_PER_SLOT; j++){
			SET_LOGICAL_OFFSET(log_address, j);
			
			/* mark page as obsolete */
			markAsObsolete(log_address, MARK_OBSOLETE_NOT_AFTER_REBOOT);
		}
	}	
	
	/* calcualte average */
    for(i = 0; i< segs_to_mark; i++){
    	obs_counter_level += get_counter_level(i);
    }
    
    obs_counter_level /= segs_to_mark;

   	PRINT_MSG_AND_NUM("\naverage= - ",obs_counter_level);
    
    /* verify that indeed the expectancy isn't far from OBS_COUNT_LEVEL_7 (max level)*/
    if(obs_counter_level >= OBS_COUNT_LEVEL_7-1)
    	return 1;
	
	return 0;
}

/**
 * @brief
 * in regulat allocation mode mark pages as obsolete, and verify obsolete pages count increase 
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest7(){		
	uint32_t seg = 5, page_offset = 120, old_free_count, old_obs_count;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, 1, seg);	
	
	init_logical_address(log_address);
	
	SET_LOGICAL_SEGMENT(log_address, seg);
	SET_LOGICAL_OFFSET(log_address, page_offset);
	
	old_obs_count  = GET_OBS_COUNT();
	old_free_count = GET_FREE_COUNTER(); 
	
	if(markAsObsolete(log_address, MARK_OBSOLETE_NOT_AFTER_REBOOT)){
		print(uart0SendByte,0,"\nfailed marking page as obsolete "); 
		return 0;	
	}	 	
	 		
	VERIFY(COMPARE(OBS_COUNT_LEVEL_0, get_counter_level(GET_LOGICAL_SEGMENT(log_address))));
	VERIFY(COMPARE(old_obs_count+1, GET_OBS_COUNT()));
	VERIFY(COMPARE(old_free_count, GET_FREE_COUNTER()));
	
	return 1;
}

/**
 * @brief
 * mark a page as obsolete and check counter incremented
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest8(){		
	uint32_t seg = 7, page_offset = 10;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
	seg_map_ptr->seg_to_slot_map[seg] = 1;		
	init_logical_address(log_address);
	SET_LOGICAL_SEGMENT(log_address, seg);
	SET_LOGICAL_OFFSET(log_address, page_offset);

	if(markAsObsolete(log_address, MARK_OBSOLETE_NOT_AFTER_REBOOT)){
//		PRINT("\nfailed marking page as obsolete"); 
		return 0;	
	}	 	
//	PRINT_MSG_AND_NUM("\ncounter level=", get_counter_level(GET_LOGICAL_SEGMENT(log_address)));
	VERIFY(COMPARE(1, get_counter_level(GET_LOGICAL_SEGMENT(log_address))));
	
	return 1;
}

/**
 * @brief
 * mark mutiple page as obsolete and check counter was incremented properly
 * @return 1 if succesful, 0 otherwise
 */
error_t markAsObsoleteTest9(){
	uint32_t seg = 5, page_offset = 5, i, max=10;	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
	seg_map_ptr->seg_to_slot_map[seg] = 1;		
	init_logical_address(log_address);
	
	SET_LOGICAL_SEGMENT(log_address, seg);
	
	
	for(i=0; i<max; i++){
		SET_LOGICAL_OFFSET(log_address, page_offset+i);
		if(markAsObsolete(log_address, MARK_OBSOLETE_NOT_AFTER_REBOOT)){
			PRINT("\nfailed marking page as obsolete"); 
			return 0;	
		}	 	
	}
	VERIFY(COMPARE(max, get_counter_level(GET_LOGICAL_SEGMENT(log_address))));
	
	return 1;
}
