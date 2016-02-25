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

//#include "simulator.h"
//#include "../system.h"
//#include "nand.h"

#include <system.h>
#include <peripherals/nand.h>
#include <stdio.h>

void *uart0SendByte;

void busywait(void *a){
}

void print(void* a, void* b,void *msg){
	printf(msg);
	fflush(stdout);
}

void printNum(void* a, void* b,int32_t num){
	printf("%d", num);
	fflush(stdout);
}

void printHex(void* a, void* b,uint32_t num, void *c){
	printf("%x", num);
	fflush(stdout);
}

void verifyPageErased(uint8_t *buf, uint32_t pageNum){
	uint32_t res;
	// verify all the pages were erased
    nandReadPage(buf, pageNum, 0, NAND_TOTAL_SIZE);
    // verify erase was succesful for all bytes
	for(res=0; res<NAND_TOTAL_SIZE ;res++){
		if (buf[res] != 0xff){			
			print(uart0SendByte,0,"ERASE ERROR - the following byte wasn't as expected: "); 			
			printNum(uart0SendByte,0,res);
			print(uart0SendByte,0,"\n"); 
			break;
		}
	}  
	
	if(res==NAND_TOTAL_SIZE){
		print(uart0SendByte,0,"ERASE OK for page "); 
		printNum(uart0SendByte,0,pageNum);
		print(uart0SendByte,0,"\n"); 
	}
}

void printPageBytes(uint32_t pageNum){  	
	uint32_t res;
	uint8_t buf1[NAND_TOTAL_SIZE];
	
	print(uart0SendByte,0,"\n"); 
	print(uart0SendByte,0,"CHECK BLOCKS - lets begin"); 			
	print(uart0SendByte,0,"\n"); 
	
  	// init buffer
  	for(res=0; res<NAND_TOTAL_SIZE; res++){
  		buf1[res] = 0x00;
  	}   	
 	
    nandReadPage(buf1, pageNum, 0, NAND_TOTAL_SIZE);  
    print(uart0SendByte,0,"page bytes are as such");
    print(uart0SendByte,0,"\n");
    for(res=0;res<NAND_TOTAL_SIZE; res++){
    	if(buf1[res]!=0xff){
	      printNum(uart0SendByte,0,res);
	      print(uart0SendByte,0,".");
	      printHex(uart0SendByte,0,buf1[res],8);
	      print(uart0SendByte,0,"\n");
    	}
    }
    print(uart0SendByte,0,"\n");
    // test byte 518 (starting from 0 it's actually NAND_VERIFY_BYTE_LOCATION)
    if (buf1[NAND_VERIFY_BYTE_LOCATION] != 0xff){ 
      print(uart0SendByte,0,"error byte 518 is not 0xff in page ");      	    
    }
    else{
  	  print(uart0SendByte,0,"page ok ");  
    }
    printNum(uart0SendByte,0,pageNum);
    print(uart0SendByte,0,"\n");
}

void run_test1(){
	uint32_t res, pageNum = 20;
	uint8_t buf[NAND_TOTAL_SIZE];
	
	print(uart0SendByte,0,"\n"); 
	print(uart0SendByte,0,"TEST 1 - lets begin"); 			
	print(uart0SendByte,0,"\n"); 
	
  	for(res=0; res<NAND_TOTAL_SIZE ;res++){
		buf[res] = 'q'+(res%10);
	}  
	  
	nandProgramPageA(buf, pageNum, 0, NAND_TOTAL_SIZE);

	// verify program was succesful for all bytes
	for(res=0; res < NAND_TOTAL_SIZE ;res++){
		if (buf[res] != 'q'+(res%10)){			
			print(uart0SendByte,0,"PROGRAM ERROR, the following byte wasn't as expected: "); 
			print(uart0SendByte,0,"\n"); 
			printNum(uart0SendByte,0,res);
			break;
		}
	}  
	
	if(res==NAND_TOTAL_SIZE){
		print(uart0SendByte,0,"PROGRAM OK for page "); 
		printNum(uart0SendByte,0,pageNum);
		print(uart0SendByte,0,"\n"); 
	}
	
	nandEUErase(pageNum-1);
	res = nandReadStatus();	
	print(uart0SendByte,0,"status is: ");    
    printNum(uart0SendByte,0,res);
    print(uart0SendByte,0,"\n"); 
    
    verifyPageErased(buf, pageNum);
}

void run_test2(){
	uint32_t res, pageNum = 51*32, write_size = 4, spare_write_size = 10;
	uint8_t temp_buf[NAND_TOTAL_SIZE];
	
	print(uart0SendByte,0,"\n"); 
	print(uart0SendByte,0,"TEST 2 - lets begin"); 			
	print(uart0SendByte,0,"\n"); 
		
  	for(res=0; res<NAND_PAGE_SIZE / 2 ;res++){
		temp_buf[res] = 'q'+(res%10);
	}  
	
	for(; res<NAND_PAGE_SIZE ;res++){
		temp_buf[res] = '1'+(res%10);
	}  
	
	for(; res<NAND_TOTAL_SIZE ;res++){
		temp_buf[res] = 'a'+(res%10);
	}  
	
	nandProgramPageA(temp_buf, pageNum, 0, write_size);
	if (nandReadStatus() == 1){
		print(uart0SendByte,0,"PROGRAM ERROR - status is 1"); 			
		print(uart0SendByte,0,"\n"); 
		return;
	}
	nandProgramPageB(temp_buf+NAND_PAGE_SIZE/2, pageNum+1, 0,write_size);
	if (nandReadStatus() == 1){
		print(uart0SendByte,0,"PROGRAM ERROR - status is 1"); 			
		print(uart0SendByte,0,"\n"); 
		return;
	}
	nandProgramPageC(temp_buf+NAND_PAGE_SIZE, pageNum+2, 0,spare_write_size);
	if (nandReadStatus() == 1){
		print(uart0SendByte,0,"PROGRAM ERROR - status is 1"); 			
		print(uart0SendByte,0,"\n"); 
		return;
	}
				
	// read page pageNum
	print(uart0SendByte,0,"verifying page "); 
	printNum(uart0SendByte,0,pageNum);
	print(uart0SendByte,0,"\n"); 	
    nandReadPage(temp_buf, pageNum, 0, NAND_TOTAL_SIZE);
    // verify program was succesful for first write_size bytes
	for(res=0; res< write_size ;res++){
		if (temp_buf[res] != 'q'+(res%10)){			
			print(uart0SendByte,0,"PROGRAM ERROR - the following byte wasn't as expected: "); 			
			printNum(uart0SendByte,0,res);
			print(uart0SendByte,0,"\n"); 
			break;
		}
	}  
	// and verify all the rest remained the same
	for(; res<NAND_TOTAL_SIZE ;res++){
		if (temp_buf[res] != 0xff){			
			print(uart0SendByte,0,"PROGRAM ERROR - the following byte wasn't as expected: "); 			
			printNum(uart0SendByte,0,res);
			print(uart0SendByte,0," ( ");
			printHex(uart0SendByte,0,temp_buf[res],0);
			print(uart0SendByte,0,")\n");			 
			break;
		}
	} 
	
	if(res==NAND_TOTAL_SIZE){
		print(uart0SendByte,0,"PROGRAM OK for page "); 		
		printNum(uart0SendByte,0,pageNum);		
		print(uart0SendByte,0,"\n"); 
	}
	
	// read page pageNum+1
	print(uart0SendByte,0,"verifying page "); 
	printNum(uart0SendByte,0,pageNum+1);
	print(uart0SendByte,0,"\n");
    nandReadPage(temp_buf, pageNum+1, 0, NAND_TOTAL_SIZE);
    // verify program was succesful for first NAND_PAGE_SIZE / 2 bytes
	for(res=0; res<NAND_PAGE_SIZE / 2 ;res++){
		if (temp_buf[res] != 0xff){			
			print(uart0SendByte,0,"PROGRAM ERROR - the following byte wasn't as expected: "); 			
			printNum(uart0SendByte,0,res);
			print(uart0SendByte,0," ( ");
			printHex(uart0SendByte,0,temp_buf[res],0);
			print(uart0SendByte,0,")\n");
			break;
		}
	}  
	
	if (res == NAND_PAGE_SIZE / 2){
		// and verify all the rest remained the same
		for(; res<NAND_PAGE_SIZE / 2+ write_size ;res++){
			if (temp_buf[res] != '1'+(res%10)){			
				print(uart0SendByte,0,"PROGRAM ERROR - the following byte wasn't as expected: "); 				
				printNum(uart0SendByte,0,res);
				print(uart0SendByte,0,", "); 				
				printHex(uart0SendByte,0,temp_buf[res],8);
				print(uart0SendByte,0," instead of "); 				
				printHex(uart0SendByte,0, '1'+(res%10), 8);
				print(uart0SendByte,0,"\n"); 
				break;
			}
		} 
	}	
	
	if (res == NAND_PAGE_SIZE / 2 + write_size){
		// verify program was succesful for first last bytes
		for(; res<NAND_TOTAL_SIZE ;res++){
			if (temp_buf[res] != 0xff){			
				print(uart0SendByte,0,"PROGRAM ERROR - the following byte wasn't as expected: "); 					
				printNum(uart0SendByte,0,res);
				print(uart0SendByte,0,"\n"); 
				break;
			}
		}  
	}	
	
	if(res==NAND_TOTAL_SIZE){
		print(uart0SendByte,0,"PROGRAM OK for page "); 
		printNum(uart0SendByte,0,pageNum);
		print(uart0SendByte,0,"\n"); 
	}
	
	// read page pageNum+2
	print(uart0SendByte,0,"verifying page "); 
	printNum(uart0SendByte,0,pageNum+2);
	print(uart0SendByte,0,"\n"); 	
    nandReadPage(temp_buf, pageNum+2, 0, NAND_TOTAL_SIZE);
    // verify program was succesful for first NAND_PAGE_SIZE bytes
	for(res=0; res<NAND_PAGE_SIZE ;res++){
		if (temp_buf[res] != 0xff){			
			print(uart0SendByte,0,"PROGRAM ERROR - the following byte wasn't as expected: "); 			
			printNum(uart0SendByte,0,res);
			print(uart0SendByte,0,"\n"); 
			break;
		}
	}  
	
	// and verify all the rest remained the same
	for(; res<NAND_PAGE_SIZE+spare_write_size ;res++){
		if (temp_buf[res] != 'a'+(res%10)){			
			print(uart0SendByte,0,"PROGRAM ERROR - the following byte wasn't as expected: "); 			
			printNum(uart0SendByte,0,res);
			print(uart0SendByte,0," (");
			printHex(uart0SendByte,0,temp_buf[res],0);		
			print(uart0SendByte,0,")\n");
			break;
		}
	} 
	
	if (res==NAND_PAGE_SIZE+spare_write_size){
		for(; res<NAND_TOTAL_SIZE ;res++){
			if (temp_buf[res] != 0xff){			
				print(uart0SendByte,0,"PROGRAM ERROR - the following byte wasn't as expected: "); 			
				printNum(uart0SendByte,0,res);
				print(uart0SendByte,0," (");
				printHex(uart0SendByte,0,temp_buf[res],0);		
				print(uart0SendByte,0,")\n");
				break;
			}
		} 
	} 
	
	if(res==NAND_TOTAL_SIZE){
		print(uart0SendByte,0,"PROGRAM OK for page "); 
		printNum(uart0SendByte,0,pageNum);
		print(uart0SendByte,0,"\n"); 
	}			
	else{
		printPageBytes(pageNum);
		printPageBytes(pageNum+1);
		printPageBytes(pageNum+2);		
	}
	
	
	nandEUErase(pageNum);
	if (nandReadStatus() == 1){
		print(uart0SendByte,0,"ERASE ERROR - status is 1"); 			
		print(uart0SendByte,0,"\n"); 
		return;
	}
	verifyPageErased(temp_buf, pageNum);
	verifyPageErased(temp_buf, pageNum+1);
	verifyPageErased(temp_buf, pageNum+2);		
}

// test 3
void run_test3(){
	uint32_t i, res, pageNum = 320*10;
	uint8_t buf[NAND_TOTAL_SIZE];
	
	print(uart0SendByte,0,"\n"); 
	print(uart0SendByte,0,"TEST 3 - lets begin"); 			
	print(uart0SendByte,0,"\n"); 
	
	for(res=0; res<NAND_TOTAL_SIZE ;res++){
		buf[res] = 'a'+(res%10);
	}  
	
	// program 32 pages in erase unit
	for(i=0; i<32; i++){		
		nandProgramPageA(buf, pageNum+i, 0, NAND_TOTAL_SIZE);
		if (nandReadStatus() == 1){
			print(uart0SendByte,0,"PROGRAM ERROR - status is 1"); 			
			print(uart0SendByte,0,"\n"); 
			return;
		}
	}
	
	print(uart0SendByte,0,"programmed whole erase unit"); 			
	print(uart0SendByte,0,"\n"); 
	
	for(res=0; res<NAND_TOTAL_SIZE ;res++){
		buf[res] = 'A'+(res%10);
	} 
	
	// now program first page in the next erase unit
	nandProgramPageA(buf, pageNum+i, 0, NAND_TOTAL_SIZE);
	if (nandReadStatus() == 1){
		print(uart0SendByte,0,"PROGRAM ERROR - status is 1"); 			
		print(uart0SendByte,0,"\n"); 
		return;
	}	 	 	
	
	print(uart0SendByte,0,"programmed last page in previous erase unit"); 			
	print(uart0SendByte,0,"\n"); 
	
	// now program last page in the previous erase unit
	nandProgramPageA(buf, pageNum-1, 0, NAND_TOTAL_SIZE);
	if (nandReadStatus() == 1){
		print(uart0SendByte,0,"PROGRAM ERROR - status is 1"); 			
		print(uart0SendByte,0,"\n"); 
		return;
	}	 	 	
	
	print(uart0SendByte,0,"programmed first page in following erase unit"); 			
	print(uart0SendByte,0,"\n"); 
	
	// zeroize buf to verify no accidents
	for(res=0; res<NAND_TOTAL_SIZE ;res++){
		buf[res] = 0;
	} 
	
	// make sure programming for all pages
	for(i=0; i<32; i++){	
	// read to make sure programming
		for(res=0; res<NAND_TOTAL_SIZE ;res++){
			buf[res] = 0;
		} 
	
		nandReadPage(buf, pageNum+i, 0, NAND_TOTAL_SIZE);
		for(res=0; res<NAND_TOTAL_SIZE ;res++){
			if (buf[res] != 'a'+(res%10)){
				print(uart0SendByte,0,"PROGRAM ERROR - 1st page. the following byte wasn't as expected: "); 			
				printNum(uart0SendByte,0,res);
				print(uart0SendByte,0,"\n"); 
				break;	 
			}
		} 
		
		if(res !=NAND_TOTAL_SIZE){
			return;
		}
	}	
	
	print(uart0SendByte,0,"all pages in first erase unit were programmed propely"); 			
	print(uart0SendByte,0,"\n"); 
	// read to make sure programming
	nandReadPage(buf, pageNum+32, 0, NAND_TOTAL_SIZE);
	for(res=0; res<NAND_TOTAL_SIZE ;res++){
		if (buf[res] != 'A'+(res%10)){
			print(uart0SendByte,0,"PROGRAM ERROR - 1st page. the following byte wasn't as expected: "); 			
			printNum(uart0SendByte,0,res);
			print(uart0SendByte,0,"\n"); 
			break;	 
		}
	} 
	
	if(res !=NAND_TOTAL_SIZE){
		return;
	}
	
	print(uart0SendByte,0,"first page of following erase unit were programmed propely"); 			
	print(uart0SendByte,0,"\n"); 
	
	// read to make sure programming
	nandReadPage(buf, pageNum-1, 0, NAND_TOTAL_SIZE);
	for(res=0; res<NAND_TOTAL_SIZE ;res++){
		if (buf[res] != 'A'+(res%10)){
			print(uart0SendByte,0,"PROGRAM ERROR - last page of previous. the following byte wasn't as expected: "); 			
			printNum(uart0SendByte,0,res);
			print(uart0SendByte,0,"\n"); 
			break;	 
		}
	} 
	
	if(res !=NAND_TOTAL_SIZE){
		return;
	}
	
	print(uart0SendByte,0,"last page of previous erase unit were programmed propely"); 			
	print(uart0SendByte,0,"\n"); 
	
	// delete first page
	nandEUErase(pageNum);
	if (nandReadStatus() == 1){
		print(uart0SendByte,0,"ERASE ERROR - status is 1"); 			
		print(uart0SendByte,0,"\n"); 
		return;
	}
	
	for(i=0; i<32; i++){	
		verifyPageErased(buf, pageNum+i);
	}
	
	print(uart0SendByte,0,"all pages of first erase unit were erased as expected"); 			
	print(uart0SendByte,0,"\n"); 
	
	// read to make sure erasing didn't affect second page
	nandReadPage(buf, pageNum+i, 0, NAND_TOTAL_SIZE);
	for(res=0; res<NAND_TOTAL_SIZE ;res++){
		if (buf[res] != 'A'+(res%10)){
			print(uart0SendByte,0,"ERASE ERROR - 1st page of following. the following byte wasn't as expected: "); 			
			printNum(uart0SendByte,0,res);
			print(uart0SendByte,0,"\n"); 
			break;	 
		}
	} 
	
	if(res !=NAND_TOTAL_SIZE){
		return;
	}
	
	print(uart0SendByte,0,"first page of following erase unit wasn't erased as expected"); 			
	print(uart0SendByte,0,"\n");
	
	// read to make sure erasing didn't affect second page
	nandReadPage(buf, pageNum-1, 0, NAND_TOTAL_SIZE);
	for(res=0; res<NAND_TOTAL_SIZE ;res++){
		if (buf[res] != 'A'+(res%10)){
			print(uart0SendByte,0,"ERASE ERROR - last page of previous. the following byte wasn't as expected: "); 			
			printNum(uart0SendByte,0,res);
			print(uart0SendByte,0,"\n"); 
			break;	 
		}
	} 
	
	if(res !=NAND_TOTAL_SIZE){
		return;
	}
	
	print(uart0SendByte,0,"last page of previous erase unit wasn't erased as expected"); 			
	print(uart0SendByte,0,"\n");
	
	nandEUErase(pageNum+i); 
	verifyPageErased(buf,pageNum+i);
	nandEUErase(pageNum-1); 
	verifyPageErased(buf,pageNum-1);
}

void check_bad_blocks(){
	uint32_t i, res;
	uint8_t buf[NAND_TOTAL_SIZE];
	
	print(uart0SendByte,0,"check for bad blocks on flash ");   	  
	print(uart0SendByte,0,"\n");   
	for(i=0; i< 65536; i++){
	  	for(res=0; res<NAND_TOTAL_SIZE ;res++){
	 		buf[res] = 0x00;
	  	}  
		busywait(100);
	  	if (i%(100*32) == 0){
		  	print(uart0SendByte,0,"now reached page no. ");   
		  	printNum(uart0SendByte,0,i);
		    print(uart0SendByte,0,"\n");   	    
	  	}
	  		    		   
	    nandReadPage(buf, i, 0, NAND_TOTAL_SIZE);  
	  	  
	    // check for bad block
	    if (buf[NAND_VERIFY_BYTE_LOCATION] != 0xff){
	      print(uart0SendByte,0,"error in page ");   
	      printNum(uart0SendByte,0,i);
	      print(uart0SendByte,0,"\n");   	    
	      // print all the defected page bytes
	      //printPageBytes(i);
	    }			
	    busywait(100);
	}
	  
	print(uart0SendByte,0,"finished checking blocks");     
	print(uart0SendByte,0,"\n");    
}


int main(void){
	uint8_t	buf[NAND_TOTAL_SIZE];
	uint32_t i;
	
	if(nandInit())
		return -1;
		
//	run_test1();	
	
	run_test2();	
	
//	run_test3();	
	
	print(uart0SendByte,0,"finished !");  
	
	nandTerminate();	
}
