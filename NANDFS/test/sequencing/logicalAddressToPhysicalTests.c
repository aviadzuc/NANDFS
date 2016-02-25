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

/** @file logicalAddressToPhysicalTests.c  */
#include <test/sequencing/testsHeader.h>
#include <test/sequencing/logicalAddressToPhysicalTests.h>

/**
 * run all logicalAddressToPhysical tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllLogicalAddressToPhysicalTests(){
//	PRINT("\nrunAllLogicalAddressToPhysicalTests() - run test 2");
	RUN_TEST(logicalAddressToPhysical, 2);
//	PRINT("\nrunAllLogicalAddressToPhysicalTests() - run test 3");
	RUN_TEST(logicalAddressToPhysical, 3);
	RUN_TEST(logicalAddressToPhysical, 4);	
	RUN_TEST(logicalAddressToPhysical, 6);
	
	return 0;
}

/**
 * @brief
 * tear down logicalAddressToPhysical test
 * @return 1 if succesful, 0 otherwise
 */
error_t init_logicalAddressToPhysicalTest(){
	if(nandInit())
		return -1;		
		
	init_seg_map();	
	init_flash();
	
	// mark two segments in the map	
	mark_reserve_segment_headers();	
	
	return 1;
}

/**
 * @brief
 * tear down logicalAddressToPhysical test
 * @return 1 if successful, 0 otherwise
 */
error_t tearDown_logicalAddressToPhysicalTest(){	
	init_seg_map();
	init_flash();		
	nandTerminate();		
	
	return 1;
}

///**
// * translate non-existent logical address to physical - slot is not legal
// * @return 1 if succesful, 0 otherwise
// */
//error_t logicalAddressToPhysicalTest1(){		
//	uint32_t res, empty_seg = 5;
//	test_seg_map_ptr->seg_to_slot_map[5] = SEQ_NO_SLOT;
//	
//	log_address->segment_num = empty_seg;
//	log_address->page_offset = 6;
//	
//	res = COMPARE(logicalAddressToPhysical(log_address), SEQ_PHY_ADDRESS_EMPTY);
//	
//	return res;
//}

/**
 * call once 
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToPhysicalTest2(){		
	uint32_t phy_addr, res, seg1=3, slot1 = 5, page_offset = 50;
//	PRINT("\nlogicalAddressToPhysicalTest2() - starting");
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
	while(page_offset >= SEQ_PAGES_PER_SLOT){
		page_offset /= 2;
	}
	
	init_logical_address(log_address);
	
//	PRINT("\nlogicalAddressToPhysicalTest2() - set seg map");
	SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr, slot1, seg1);	
	
	SET_LOGICAL_SEGMENT(log_address, seg1);
//	SET_BYTE(log_address,1, ((GET_BYTE(log_address,1) & 0x0f) | ((seg1 & 0x0f) << 4))) ;
//	PRINT_MSG_AND_HEX("\nafter setting byte 1 hex addr=", *((uint32_t*)(log_address))); 
//	SET_BYTE(log_address,2, (seg1 & 0xff0) >> 4);
//	PRINT_MSG_AND_HEX("\nafter setting byte 2 hex addr=", *((uint32_t*)(log_address)));
	
//	PRINT_MSG_AND_NUM("\nafter setting seg=", GET_LOGICAL_SEGMENT(log_address));
//	PRINT_MSG_AND_HEX(" hex addr=", *((uint32_t*)(log_address)));
	
	SET_LOGICAL_OFFSET(log_address, page_offset);
//	SET_BYTE(log_address,0, page_offset & 0xff); 
//	PRINT_MSG_AND_HEX("\nafter setting byte 0 hex addr=", *((uint32_t*)(log_address)));
//	SET_BYTE(log_address,1, (((page_offset & 0xf00) >> 4) | (GET_BYTE(log_address,1) & 0xf0)));
//	PRINT_MSG_AND_HEX("\nafter setting byte 1 hex addr=", *((uint32_t*)(log_address)));

//	PRINT_MSG_AND_NUM("\nafter setting  offset=", GET_LOGICAL_OFFSET(log_address));	
//	PRINT_MSG_AND_HEX(" hex addr=", *((uint32_t*)(log_address)));
				
	phy_addr = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg1],0, GET_LOGICAL_OFFSET(log_address));	

//	assert(0);	
	res = COMPARE(logicalAddressToPhysical(log_address), phy_addr);
	
	return res;
}

/**
 * call twice
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToPhysicalTest3(){		
	uint32_t phy_addr, res;
	uint32_t seg1=3, slot1 = 5, page_offset1 = 50;
	uint32_t seg2=7, slot2 = 8, page_offset2 = 285;
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
	while(page_offset2 >= SEQ_PAGES_PER_SLOT || page_offset1 >= SEQ_PAGES_PER_SLOT){
		page_offset1 /= 2;
		page_offset2 /= 2;
	}
	
	seg_map_ptr->seg_to_slot_map[seg1] = slot1;	
	seg_map_ptr->seg_to_slot_map[seg2] = slot2;	
	
	SET_LOGICAL_SEGMENT(log_address, seg1);
	SET_LOGICAL_OFFSET(log_address, page_offset1);
				
	phy_addr = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg1],0, GET_LOGICAL_OFFSET(log_address));	
	res = COMPARE(logicalAddressToPhysical(log_address), phy_addr);
	if(!res)
		return res;
	
	SET_LOGICAL_SEGMENT(log_address, seg2);
	SET_LOGICAL_OFFSET(log_address, page_offset2);
	
	phy_addr = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg2],0, GET_LOGICAL_OFFSET(log_address));	
	res = COMPARE(logicalAddressToPhysical(log_address), phy_addr);
	if(!res)
		return res;
	
	return res;
}

/**
 * call once, for an address in a bad page
 * @return 0 if succesful, 1 otherwise
 */
error_t logicalAddressToPhysicalTest4(){		
	uint32_t orig_phy_addr, phy_addr, res, seg1=3, slot1 = 5, slot_page_offset = 50, eu_page_offset = CALC_OFFSET_IN_EU(slot_page_offset);
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	
	seg_map_ptr->seg_to_slot_map[seg1] = slot1;	
	
	SET_LOGICAL_SEGMENT(log_address, seg1);
	SET_LOGICAL_OFFSET(log_address, slot_page_offset);
		
	orig_phy_addr = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg1],0, GET_LOGICAL_OFFSET(log_address));	
	markEuAsMockBad(orig_phy_addr);			
	phy_addr = write_to_reserve_eu(orig_phy_addr);	
	res = COMPARE(logicalAddressToPhysical(log_address), phy_addr+eu_page_offset);
	
	return res;
}

///**
// * translate non-existent logical address to physical - page offset not legal
// * @return 1 if succesful, 0 otherwise
// */
//error_t logicalAddressToPhysicalTest5(){		
//	uint32_t res, empty_seg = 5;
//	test_seg_map_ptr->seg_to_slot_map[5] = 1;
//	
//	log_address->segment_num = 5;
//	log_address->page_offset = SEQ_PAGES_PER_SLOT + 5;
//	
//	res = COMPARE(logicalAddressToPhysical(log_address), SEQ_PHY_ADDRESS_EMPTY);
//	
//	return res;
//}

/**
 * @brief
 * test that reading from a page in a reclaimed address is found properly
 * @return 0 if succesful, 1 otherwise
 */
error_t logicalAddressToPhysicalTest6(){	
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);	
	uint32_t seg = 5, reclaimed_slot = 1, new_slot = 10, eu_page_offset = 20;
	seg_map_ptr->seg_to_slot_map[seg] = reclaimed_slot;
	
	init_logical_address(log_address);		
	SET_LOGICAL_SEGMENT(log_address, seg);
	SET_LOGICAL_OFFSET(log_address, eu_page_offset-1);
			
	/* set seg map*/	
	seg_map_ptr->new_slot_id = reclaimed_slot;	
	SET_RECLAIMED_SEGMENT(seg);
	SET_RECLAIMED_OFFSET(eu_page_offset);
	//SET_SEGMENG_MAP_RECLAIMED_ADDRESS(test_seg_map_ptr, seg, eu_page_offset);
	seg_map_ptr->nSegments = SEQ_SEGMENTS_COUNT;	
	
	// try reading from the address. since we have no new slot stated, should get from old address
	VERIFY(COMPARE(logicalAddressToPhysical(log_address), CALC_ADDRESS(reclaimed_slot, 0, eu_page_offset-1)));
	// now try reading from already reclaimed address - should get page from new_slot			
	seg_map_ptr->new_slot_id = new_slot;	
	
	VERIFY(COMPARE(logicalAddressToPhysical(log_address), CALC_ADDRESS(new_slot, 0, eu_page_offset-1)));

	// and now try reading from already not reclaiemd segment
	SET_LOGICAL_OFFSET(log_address, eu_page_offset+1);		
	VERIFY(COMPARE(logicalAddressToPhysical(log_address), CALC_ADDRESS(reclaimed_slot, 0, eu_page_offset+1)));
			
	return 1;
}

/**
 * @brief
 * call once, for an address in a supposedly in reclaimed page, but the state is not reclamation
 * @return 0 if succesful, 1 otherwise
 */
error_t logicalAddressToPhysicalTest7(){		
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);	
	uint32_t seg = 5, reclaimed_slot = 1, eu_page_offset = 20;
	
	init_logical_address(log_address);	
	
	SET_SEG_MAP_SLOT_FOR_SEG(seg_map_ptr,reclaimed_slot,seg);	
			
	SET_LOGICAL_SEGMENT(log_address, seg);
	SET_LOGICAL_OFFSET(log_address,eu_page_offset);	
	VERIFY(IS_ADDRESS_IN_RECLAIMED_SEGMENT(log_address, seg_map_ptr) &&	
		   IS_ADDRESS_IN_RECLAIMED_PAGE(log_address, seg_map_ptr));	
	// try reading from the address. since we have no reclaimed segment stated, should get from old address
	VERIFY(COMPARE(logicalAddressToPhysical(log_address), CALC_ADDRESS(reclaimed_slot, 0, eu_page_offset)));	
			
	return 1;
}
