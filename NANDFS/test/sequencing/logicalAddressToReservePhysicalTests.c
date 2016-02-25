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

/** @file logicalAddressToReservePhysicalTests.c  */
#include <test/sequencing/logicalAddressToReservePhysicalTests.h>
#include <test/sequencing/testsHeader.h>

/**
 * run all logicalAddressToPhysical tests
 * @return 0 if succesful, 1 otherwise
 */
error_t runAllLogicalAddressToReservePhysicalTests(){
	RUN_TEST(logicalAddressToReservePhysical, 1);
	RUN_TEST(logicalAddressToReservePhysical, 2);
	RUN_TEST(logicalAddressToReservePhysical, 3);
	RUN_TEST(logicalAddressToReservePhysical, 4);
	RUN_TEST(logicalAddressToReservePhysical, 5);
	RUN_TEST(logicalAddressToReservePhysical, 6);

	return 0;
}

/**
 * @brief
 * init logicalAddressToReservePhysical test
 * @return 1 if succesful, 0 otherwise
 */
error_t init_logicalAddressToReservePhysicalTest(){
	int32_t i;
	
//	PRINT("\ninit_logicalAddressToReservePhysicalTest() - starting");
	if(nandInit())
		return -1;		
//	PRINT("\ninit_logicalAddressToReservePhysicalTest() - nandInit finished");	
	for(i=0; i< SEQ_N_SLOTS_COUNT; i++){
		seg_map_ptr->seg_to_slot_map[i] = SEQ_NO_SLOT;
	}
	
	// mark two segments in the map
	seg_map_ptr->seg_to_slot_map[0] = 3;
	seg_map_ptr->seg_to_slot_map[1] = 2;
	seg_map_ptr->seg_to_slot_map[2] = 1;
	seg_map_ptr->seg_to_slot_map[3] = 0;
	
	for(i=4; i< SEQ_SEGMENTS_COUNT; i++){
		seg_map_ptr->seg_to_slot_map[i] = i;
	}	
	
//	PRINT("\ninit_logicalAddressToReservePhysicalTest() - mark_reserve_segment_headers()");
	mark_reserve_segment_headers();
	
//	PRINT("\ninit_logicalAddressToReservePhysicalTest() - finished");
	return 1;
}
	
/**
 * @brief
 * tear down logicalAddressToReservePhysical test
 * @return 1 if succesful, 0 otherwise
 */
error_t tearDown_logicalAddressToReservePhysicalTest(){	
	uint32_t i, j;
	
//	PRINT("\ntear down"); 
	
	for(i=0; i< SEQ_N_SLOTS_COUNT; i++){
		seg_map_ptr->seg_to_slot_map[i] = SEQ_NO_SLOT;
	}

	// delete segments
	for(i=0; i<SEQ_N_SLOTS_COUNT; i++){		
		/* erase eu's one by one from the segment */
		j = i * SEQ_PAGES_PER_SLOT; 
		for(; j< (i+1)* SEQ_PAGES_PER_SLOT; j+=NAND_PAGES_PER_ERASE_UNIT){			
			// if we encounter the first valid EU
			if(nandCheckEuStatus(j)){
				/* erase it */
				nandErase(j);
			}
		}		
	}
	
	nandTerminate();
	
	return 1;
}

/**
 * @brief
 * mark an eu as a reserve of some eu, and try reading it
 * write to phy_addr a reserve eu replacing the eu of orig_phy_addr 
 * and try to calculate it again using logicalAddressToReservePhysical
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToReservePhysicalTest1(){	
	uint32_t slot_num = 1, eu_offset = 7, page_offset = 0;
	uint32_t res,phy_addr, orig_phy_addr;		
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	
	init_logical_address(log_address);
//	PRINT("\nlogicalAddressToReservePhysicalTest1() - starting");
	SET_LOGICAL_SEGMENT(log_address, 0);
	SET_LOGICAL_OFFSET(log_address, page_offset);
	
	orig_phy_addr = CALC_ADDRESS(slot_num,eu_offset,page_offset);			
	phy_addr      = write_to_reserve_eu(orig_phy_addr);				
	
	init_flags(flags);		
	// remember to compare to phy_addr+page offset (since phy_addr is the address of the EU containg the actual page...)
	res = COMPARE(logicalAddressToReservePhysical(flags, orig_phy_addr), phy_addr );			
//	PRINT("\nlogicalAddressToReservePhysicalTest1() - finished");
	return res;
}

/**
 * @brief
 * mark 2 eus as a reserve of 2 other eu's, and try reading the second one
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToReservePhysicalTest2(){	
	uint32_t seg_num0 = 1, eu_offset0 = 1, page_offset0 = 0;
	uint32_t seg_num1 = 3, eu_offset1 = 2, page_offset1 = 7;
	uint32_t res,phy_addr, orig_phy_addr;		
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	
	init_logical_address(log_address);
	
	SET_LOGICAL_SEGMENT(log_address, seg_num1);
	SET_LOGICAL_OFFSET(log_address, page_offset1);
	
	// write first time
	orig_phy_addr = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg_num0],eu_offset0,page_offset0);	
	phy_addr      = write_to_reserve_eu(orig_phy_addr);			
	
	// write second time
	orig_phy_addr = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg_num1],eu_offset1,page_offset1);
	phy_addr      = write_to_reserve_eu(orig_phy_addr);				
	
	init_flags(flags);	
	// remember to compare to phy_addr+page offset (since phy_addr is the address of the EU containg the actual page...)		
	res = COMPARE(logicalAddressToReservePhysical(flags, orig_phy_addr) , phy_addr + page_offset1);			
	
	return res;
}

/**
 * @brief
 * mark 3 eus as a reserve of 3 other eu's, and try reading the second one,
 * then the third one
 * @return 0 if succesful, 1 otherwise
 */
error_t logicalAddressToReservePhysicalTest3(){	
	uint32_t seg_num0 = 1, eu_offset0 = 1, page_offset0 = 0;
	uint32_t seg_num1 = 3, eu_offset1 = 2, page_offset1 = 7;
	uint32_t seg_num2 = 2, eu_offset2 = 14, page_offset2 = 25;	
	uint32_t res,phy_addr1, orig_phy_addr1, phy_addr2, orig_phy_addr2;		
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
		
	init_logical_address(log_address);
		
	// write first time
	orig_phy_addr1 = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg_num0],eu_offset0,page_offset0);	
	phy_addr1      = write_to_reserve_eu(orig_phy_addr1);			
	
	// write second time
	orig_phy_addr1 = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg_num1],eu_offset1,page_offset1);
	phy_addr1      = write_to_reserve_eu(orig_phy_addr1);				
	
	// write third time
	orig_phy_addr2 = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg_num2],eu_offset2,page_offset2);
	phy_addr2      = write_to_reserve_eu(orig_phy_addr2);				

	init_flags(flags);
	SET_LOGICAL_SEGMENT(log_address, seg_num1);
	SET_LOGICAL_OFFSET(log_address, page_offset1);	
	// remember to compare to phy_addr+page offset (since phy_addr is the address of the EU containg the actual page...)		
	res = COMPARE(logicalAddressToReservePhysical(flags, orig_phy_addr1) , phy_addr1 + page_offset1);			

	// verify not failed
	if(!res)
		return res;
	
	SET_LOGICAL_SEGMENT(log_address, seg_num2);
	SET_LOGICAL_OFFSET(log_address, page_offset2);
	res = COMPARE(logicalAddressToReservePhysical(flags, orig_phy_addr2) , phy_addr2 + page_offset2);
	
	return res;
}

//error_t compare_expected_and_result(logical_addr_t *log_address,uint32_t seg_id, uint32_t eu_offset, uint32_t page_offset, uint32_t expected, bool isReserveTest){
//	log_address->segment_num = seg_id;
//	log_address->page_offset = page_offset;	
//	
//	uint32_t phy_addr = CALC_ADDRESS(test_seg_map_ptr->seg_to_slot_map[seg_id],eu_offset, page_offset);	
//	
//	if(isReserveTest)
//		return COMPARE(logicalAddressToReservePhysical(log_address, flags, phy_addr) , expected+page_offset);					
//	else
//		return COMPARE(logicalAddressToReservePhysical(log_address, flags, phy_addr) , expected+page_offset);					
//}

/**
 * @brief
 * write SEQ_EUS_PER_SLOT reserve eu's. this will write
 * SEQ_EUS_PER_SLOT-1 reserve eu's to the first segment, and 1 to the next
 * verify one of the first EUs, and one in the second reserve segment
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToReservePhysicalTest4(){	
	uint32_t phy_addr_mid = 0, seg_num1 = 3, eu_offset1 = 2, page_offset1 = 7;	
	uint32_t i, res,phy_addr0, orig_phy_addr0,phy_addr1, orig_phy_addr1, i_mid = 2;		
	INIT_LOGICAL_ADDRESS_STRUCT_AND_PTR(log_address);
	INIT_FLAGS_STRUCT_AND_PTR(flags);
	
	init_logical_address(log_address);
	
	/* write SEQ_EUS_PER_SLOT reserve EUs*/	
	for(i=0; i<SEQ_EUS_PER_SLOT; i++){
		orig_phy_addr0 = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[i],i,i);	
		phy_addr0      = write_to_reserve_eu(orig_phy_addr0);
		
		if(i==i_mid){
			phy_addr_mid = phy_addr0;
		}			
	}	
	
	res = compare_expected_and_reserve(i_mid,i_mid,i_mid,phy_addr_mid);
//	PRINT_MSG_AND_NUM("\nres=", res);			
	if(!res)
		return res;		
	
	/* write another time*/
	orig_phy_addr1 = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[seg_num1],eu_offset1,page_offset1);
	phy_addr1      = write_to_reserve_eu(orig_phy_addr1);				
	
	init_flags(flags);	
	SET_LOGICAL_SEGMENT(log_address, seg_num1);
	SET_LOGICAL_OFFSET(log_address, page_offset1);
	
	/* remember to compare to phy_addr+page offset (since phy_addr is the address of the EU containg the actual page...)*/		
	res = COMPARE(logicalAddressToReservePhysical(flags, orig_phy_addr1) , phy_addr1 + page_offset1);				
	
	return res;
}

/**
 * @brief
 * 3 consecutice calls
 * 
 * @return 1 if succesful, 0 otherwise
 */
error_t logicalAddressToReservePhysicalTest5(){		
	uint32_t i, orig_phy_addr0, phy_addr, i0 = 0, i1 = 1, i2 = 2, phy_addr0 = 0, phy_addr1 = 0, phy_addr2 = 0;		
		
	// write SEQ_EUS_PER_SLOT reserve EUs	
	for(i=0; i<SEQ_EUS_PER_SLOT; i++){
		orig_phy_addr0 = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[i],i,i);	
		phy_addr      = write_to_reserve_eu(orig_phy_addr0);
		
		if(i==i0)
			phy_addr0 = phy_addr;			
			
		if(i==i1)
			phy_addr1 = phy_addr;			
			
		if(i==i2)
			phy_addr2 = phy_addr;				
	}					
	
	VERIFY(compare_expected_and_reserve(i0,i0,i0, phy_addr0));	
//	PRINT("\nsuccess1");
	VERIFY(compare_expected_and_reserve(i1,i1,i1, phy_addr1));
//	PRINT("\nsuccess2");		

	VERIFY(compare_expected_and_reserve(i2,i2,i2, phy_addr2));
//	PRINT("\nsuccess3");	
		
	return 1;	
}

/**
 * @brief
 * SEQ_EUS_PER_SLOT consecutive calls
 * 
 * @return 0 if succesful, 1 otherwise
 */
error_t logicalAddressToReservePhysicalTest6(){		
	uint32_t i, orig_phy_addr0, phy_addr;
	uint32_t phy_addresses[SEQ_EUS_PER_SLOT];	
	
	/* write SEQ_EUS_PER_SLOT reserve EUs */	
	for(i=0; i<SEQ_EUS_PER_SLOT; i++){
//		PRINT_MSG_AND_NUM("\ni=", i);
//		PRINT_MSG_AND_NUM(" SEQ_EUS_PER_SLOT=", SEQ_EUS_PER_SLOT);
		orig_phy_addr0 = CALC_ADDRESS(seg_map_ptr->seg_to_slot_map[i],i,0);	
		phy_addr       = write_to_reserve_eu(orig_phy_addr0);
		
		phy_addresses[i] = 	phy_addr;	
	}					
	
	for(i=0; i<SEQ_EUS_PER_SLOT; i++){
//		PRINT_MSG_AND_NUM("\ncompare_expected_and_reserve i=", i);
		VERIFY(compare_expected_and_reserve(i,i,0, phy_addresses[i]));
//		PRINT(". done");
	}	
		
	return 1;	
}
