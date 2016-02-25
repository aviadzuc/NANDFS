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

/*
 * NAND
 * 
 * this is a functions file for nandx8 device , using an lpc2xxx ARM-based microcontroller.
 * The functions assume the pins of the microcontroller are connected to the nand device pins as follows - 
 * IO.00-IO.07 <- P0.8 - P0.15 (correspondingly)
 * ~WP <-P0.16
 * ~WE <-P0.17
 * ALE <-P0.18
 * CLE <-P0.19
 * ~C  <-P0.20
 * ~RE <-P0.21
 * RB  <-P0.22
 *
 * Sivan Toledo
 */

#include <system.h>
#include <peripherals/nand.h>
#include <lpc2000/busywait.h>

#include <lpc2000/uart.h>
#include <utils/print.h>

// masks for each nand pin, indicating which pin in the microcontroller controls it
// (used with IOSET0 etc.)
// LOW actually means 'not'
#define WRITE_PROTECT_LOW 1 << 16
#define WRITE_ENABLE_LOW  1 << 17
#define ADDRESS_LATCH     1 << 18
#define COMMAND_LATCH     1 << 19
#define CHIP_ENABLE_LOW   1 << 22   
#define READ_ENABLE_LOW   1 << 21
#define NAND_IS_READY     1 << 20 // 1 ready, 0 busy (from the nand chip)
#define IO_0			  1 << 8
#define IO_1			  1 << 9
#define IO_2			  1 << 10
#define IO_3			  1 << 11
#define IO_4			  1 << 12
#define IO_5			  1 << 13
#define IO_6			  1 << 14
#define IO_7			  1 << 15

// custom masks
#define IO_MASK 		  (IO_0 | IO_1 | IO_2 | IO_3 | IO_4 | IO_5 | IO_6 | IO_7)

// commands
#define CMD_READ_ID         0x90
#define CMD_READ_PAGE       0x00
#define CMD_READ_PAGE_SPARE 0x50
#define CMD_PROGRAM_PAGE    0x80
#define CMD_PROGRAM_PAGE_A  0x00
#define CMD_PROGRAM_PAGE_B  0x01
#define CMD_PROGRAM_PAGE_C  0x50
#define CMD_PROGRAM_CONFIRM 0x10
#define CMD_ERASE_SETUP     0x60
#define CMD_ERASE_CONFIRM   0xD0
#define CMD_READ_STATUS     0x70

//macros
#define NAND_IS_BUSY()           (!(IOPIN0 & NAND_IS_READY))
#define SHIFT_TO_IO(UINT)        (((UINT) << 8) & IO_MASK)
#define SHIFT_FROM_IO(UINT)      ((UINT) >> 8)
#define SHIFT_OFFSET_TO_IO(UINT) (((UINT) >> 8) & IO_MASK) 
#define SHIFT_PAGE_1_TO_IO(UINT) (((UINT) << 8) & IO_MASK) 
#define SHIFT_PAGE_2_TO_IO(UINT) ((UINT) & IO_MASK) 
#define SHIFT_STATUS(UINT)		 ((UINT) >> 8)

#define ADD_OFFSET_TO_ADDRESS(PAGE, OFFSET) (PAGE | (OFFSET << 16))


void zeroize_buf(uint8_t *buf, uint32_t size){
	while(size>0){
		buf[size-1] = 0;
		size--;
	}			
}

#ifdef DO_ECC
/**
 * @brief
 * verify that the expected ECC of the page is as expected
 * @param buf buffer with page data
 * @return 1 if ecc failed, 0 if succeeded
 */
error_t verifyECC(uint8_t *buf){
	uint8_t expected_ecc[ECC_BYTS_PER_256_BYTES], read_ecc[ECC_BYTS_PER_256_BYTES];
	zeroize_buf(expected_ecc, ECC_BYTS_PER_256_BYTES);
	zeroize_buf(read_ecc, ECC_BYTS_PER_256_BYTES);
	INIT_FLAGS_POINTER_TO_BUFFER(flags, buf);
	uint32_t i,j;
	
	for(i=0;i<NAND_PAGE_SIZE >> POWER_256; i++){	
		yaffs_ECCCalculate(&(buf[i*256]), expected_ecc);
		
		for(j=0;j<ECC_BYTS_PER_256_BYTES;j++){
			read_ecc[j] = flags->bytes[ECC_LOCATION+i*ECC_BYTS_PER_256_BYTES+j];
		}		

		for(j=0;j<ECC_BYTS_PER_256_BYTES;j++){
//			PRINT_MSG_AND_HEX("\nverifyECC() - j=",j);
//			PRINT_MSG_AND_HEX("\n expected_ecc[j]=",expected_ecc[j]);
//			PRINT_MSG_AND_HEX("\n read_ecc[j]=",read_ecc[j]);
			if(expected_ecc[j]!=read_ecc[j]){
				/* try to correct*/
				if (yaffs_ECCCorrect(&(buf[i*256]), read_ecc, expected_ecc) == -1)				
					return 1;	
			}
		}	
	}	
	
	return 0;
}

/**
 * @brief
 * set ecc field of page flags
 * @param buf buffer with page data
 * @return 1 if error, return 0 if successful
 */
error_t setECC(uint8_t *buf){	
	INIT_FLAGS_POINTER_TO_BUFFER(flags, buf);
	uint32_t i;
//	return 0;
//	PRINT_MSG_AND_HEX("\nsetECC() - ECC_LOCATION=",ECC_LOCATION);
	for(i=0;i<NAND_PAGE_SIZE >> POWER_256; i++){
//		PRINT_MSG_AND_NUM("\nsetECC() - i=",i);
//		PRINT("\n");
		yaffs_ECCCalculate(&buf[i*256], &flags->bytes[i*3+ECC_LOCATION]);
				
//		PRINT_MSG_AND_HEX("\nsetECC() - ecc[i*3] = ",flags->bytes[i*3+ECC_LOCATION]);
//		PRINT_MSG_AND_HEX("\nsetECC() - ecc[i*3+1] = ",flags->bytes[i*3+1+ECC_LOCATION]);
//		PRINT_MSG_AND_HEX("\nsetECC() - ecc[i*3+2] = ",flags->bytes[i*3+2+ECC_LOCATION]);
	}	
	
	return 0;
}

#endif 

static void nandOpInit(){
//	print(uart0SendByte,0,"nandOpInit(): ");       
//	print(uart0SendByte,0,"\n");
	
	IOSET0 |= (CHIP_ENABLE_LOW  |
			   WRITE_ENABLE_LOW |	
			   READ_ENABLE_LOW);
	
//    print(uart0SendByte,0,"after IOSET0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
			   
	IOCLR0 |= (COMMAND_LATCH | ADDRESS_LATCH | WRITE_PROTECT_LOW);	
	
//    print(uart0SendByte,0,"after IOCLR0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
}

error_t nandInit(void){	
	/* initialize modules*/
	clocksInit(1,4);
	busywaitInit();
	uart0Init(38400);
	
  // set pinsel to ensure that P0.8 to P0.22 are gpio         	
	// eace pin is set by 2 bits, so -	
	// P0.8 - P0.15 mask	
	PINSEL0 &= ~(0xFFFF0000); // ~ because 00 sets the pin to gpio
	// P0.16 - P0.22
	PINSEL1 &= ~(0x00003FFF);
	
	//busywait(100000);
//    print(uart0SendByte,0,"nandInit(): ");       
//	print(uart0SendByte,0,"\n");
//     	 
//    print(uart0SendByte,0,"PINSEL0: ");       
//    printHex(uart0SendByte,0,PINSEL0,8);
//	print(uart0SendByte,0,"\n");
//    
//    print(uart0SendByte,0,"PINSEL1: ");       
//    printHex(uart0SendByte,0,PINSEL1,8);
//    print(uart0SendByte,0,"\n");   
  // set IODIR except for IO0-IO7
	// 0-input, 1-output, therefore
	IODIR0 |= (WRITE_PROTECT_LOW |
			   WRITE_ENABLE_LOW  |
			   ADDRESS_LATCH     |
		       COMMAND_LATCH     |
			   CHIP_ENABLE_LOW   |
			   READ_ENABLE_LOW);

//    print(uart0SendByte,0,"IODIR0: ");       
//    printNum(uart0SendByte,0,IODIR0);			
//	print(uart0SendByte,0,"\n");
  // set initial values of pins:	  
  //   CLE = 0
  //   ALE = 0
  //   -CE = 1
  //   ...
  //   -WP = 1
	nandOpInit();
	
	return 0;
}

static void nandCommand(uint32_t cmd){		
//	print(uart0SendByte,0,"nandCommand(): ");       
//	print(uart0SendByte,0,"\n");
//	
//	print(uart0SendByte,0,"after IOSET0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
  // set io to output
  	// 0-input, 1-output, therefore
	IODIR0 |= IO_MASK;
	
  // set io to cmd high
  	// shift cmd to location of IO0 (P0.8 in microcontroller)
  	// set all 1's to 1's  	
	IOSET0 |= SHIFT_TO_IO(cmd);
	
//	print(uart0SendByte,0,"after IOSET0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
  // set io to cmd low
  	// set all 0's to 0's  	
    IOCLR0 |= SHIFT_TO_IO(~cmd);
    
//    print(uart0SendByte,0,"after IOCLR0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
  // set CLE high
  	IOSET0 |= COMMAND_LATCH;
  	
//  	print(uart0SendByte,0,"after IOSET0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
  // set -WE, -CE low
	IOCLR0 |= (WRITE_ENABLE_LOW | CHIP_ENABLE_LOW);  
  busywait(1); // 10us, too long
  
//  	print(uart0SendByte,0,"after IOCLR0: ");       
//    printHex(uart0SendByte,0,IO#define CMD_ERASE_SETUP     0x60PIN0,8);
//	print(uart0SendByte,0,"\n");
  // set -WE high // now the chip latches
	IOSET0 |= WRITE_ENABLE_LOW;  
  
//    print(uart0SendByte,0,"after IOSET0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
  // set CLE low
  	IOCLR0 |= COMMAND_LATCH;  
  	
//  	print(uart0SendByte,0,"after IOCLR0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
}

static void nandAddressOneCycle(uint32_t addr) {
//	print(uart0SendByte,0,"nandAddressOneCycle(): ");       
//	print(uart0SendByte,0,"\n");
  // set io to output
	// 0-input, 1-output, therefore
	IODIR0 |= IO_MASK; 	
	
  // set io to addr high
    // shift cmd to location of IO0 (P0.8 in microcontroller)
  	IOSET0 |= SHIFT_TO_IO(addr);
  	
//  	print(uart0SendByte,0,"after IOSET0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
  // set io to addr low
    IOCLR0 |= SHIFT_TO_IO(~addr);
    
//    print(uart0SendByte,0,"after IOCLR0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");  	
  // set ALE high
    IOSET0 |= ADDRESS_LATCH;
    
//    print(uart0SendByte,0,"after IOSET0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
  // set -WE, low
  	IOCLR0 |= WRITE_ENABLE_LOW;  	
    busywait(1); // 1us, too long    
  
//	print(uart0SendByte,0,"after IOCLR0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
  // set -WE high // now the chip latches
    IOSET0 |= WRITE_ENABLE_LOW;    
    busywait(1); // 1us, too long
  
//  	print(uart0SendByte,0,"after IOSET0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
  // set ALE low
  	IOCLR0 |= ADDRESS_LATCH;
  	
//  	print(uart0SendByte,0,"after IOCLR0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");  	
}

static uint32_t nandRead(){	
//	print(uart0SendByte,0,"nandRead(): ");       
//	print(uart0SendByte,0,"\n");
	
	uint32_t id0;	
  // set io to input
	// 0-input, 1-output, therefore
	IODIR0 &= ~IO_MASK;
  // set -RE low
	IOCLR0 |= READ_ENABLE_LOW;  
  // wait tREA
    busywait(1); // 1us, should be > 35ns      
  
  // copy the data from the pins
  	id0 = SHIFT_FROM_IO(IOPIN0 & IO_MASK);  		
  // set -RE high
	IOSET0 |= READ_ENABLE_LOW;
	
//	print(uart0SendByte,0,"after IOSET0: ");       
//    printHex(uart0SendByte,0,IOPIN0,8);
//	print(uart0SendByte,0,"\n");
	
	return id0;
}

void nandRelease(){
	// TODO: do we need to release -CE?
}

uint32_t nandReadID()
{
	uint8_t id1, id2;
	
	nandOpInit();
	nandCommand(CMD_READ_ID);
	//busywait(100);
	nandAddressOneCycle(0x00);
	// wait tWHR1
	busywait(1); // 1us, should be 60ns
	//maker code    
	busywait(1); //
	id1 = nandRead();  
	// device code
	id2 = nandRead();
	
	nandRelease(); // do we need to release -CE?
	
    return (id1 << 8) | id2;  
}

static void nandAddressTwoCycles(uint32_t addr) {	
    // set io to output
    // 0-input, 1-output, therefore
    IODIR0 |= IO_MASK; 	
	
	// set A9-A16
    // set io to addr high
    // shift cmd to location of IO0 (P0.8 in microcontroller)    
  	IOSET0 |= SHIFT_PAGE_1_TO_IO(addr);
    // set io to addr low
    IOCLR0 |= SHIFT_PAGE_1_TO_IO(~addr);
  	
    // set ALE high
    IOSET0 |= ADDRESS_LATCH;
    // set -WE, low
  	IOCLR0 |= WRITE_ENABLE_LOW;
    busywait(1); // 1us, too long    
    // set -WE high // now the chip latches
    IOSET0 |= WRITE_ENABLE_LOW;    
        
    // set A17-A24 
    // set io to addr high
    // shift cmd to location of IO0 (P0.8 in microcontroller)    
  	IOSET0 |= SHIFT_PAGE_2_TO_IO(addr);
    // set io to addr low
    IOCLR0 |= SHIFT_PAGE_2_TO_IO(~addr);
    
    // set -WE, low
  	IOCLR0 |= WRITE_ENABLE_LOW;
    busywait(1); // 1us, too long    
    // set -WE high // now the chip latches
    IOSET0 |= WRITE_ENABLE_LOW;    
    
    busywait(1); // 1us, too long
    // set ALE low
  	IOCLR0 |= ADDRESS_LATCH;  		
}

static void nandAddressThreeCycles(uint32_t addr) {	
    // set io to output
    // 0-input, 1-output, therefore
    IODIR0 |= IO_MASK; 	
	
	// set A0-A7
    // set io to addr high
    // shift cmd to location of IO0 (P0.8 in microcontroller)    
  	IOSET0 |= SHIFT_OFFSET_TO_IO(addr);
    // set io to addr low
    IOCLR0 |= SHIFT_OFFSET_TO_IO(~addr);
  	
    // set ALE high
    IOSET0 |= ADDRESS_LATCH;
    // set -WE, low
  	IOCLR0 |= WRITE_ENABLE_LOW;
    busywait(1); // 1us, too long    
    // set -WE high // now the chip latches
    IOSET0 |= WRITE_ENABLE_LOW;    
    
    // set A9-A16
    // set io to addr high
    // shift cmd to location of IO0 (P0.8 in microcontroller)    
  	IOSET0 |= SHIFT_PAGE_1_TO_IO(addr);
    // set io to addr low
    IOCLR0 |= SHIFT_PAGE_1_TO_IO(~addr);
    
    // set -WE, low
  	IOCLR0 |= WRITE_ENABLE_LOW;
    busywait(1); // 1us, too long    
    // set -WE high // now the chip latches
    IOSET0 |= WRITE_ENABLE_LOW;    
    
    // set A17-A24 
    // set io to addr high
    // shift cmd to location of IO0 (P0.8 in microcontroller)    
  	IOSET0 |= SHIFT_PAGE_2_TO_IO(addr);
    // set io to addr low
    IOCLR0 |= SHIFT_PAGE_2_TO_IO(~addr);
    
    // set -WE, low
  	IOCLR0 |= WRITE_ENABLE_LOW;
    busywait(1); // 1us, too long    
    // set -WE high // now the chip latches
    IOSET0 |= WRITE_ENABLE_LOW;    
    
    busywait(1); // 1us, too long
    // set ALE low
  	IOCLR0 |= ADDRESS_LATCH;  		
}

error_t nandReadPageGeneric(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len, uint32_t cmd){
	uint32_t i, idx, counter = 0;
	
#ifdef DO_ECC
	/* if we're reading an entire page*/
	if(CMD_PROGRAM_PAGE_A == cmd){	
		if(verifyECC(buf)){
			return 1;
		}
	}
#endif	

	nandOpInit();
	nandCommand(cmd);
	idx = 0;
	nandAddressThreeCycles(ADD_OFFSET_TO_ADDRESS(page, offset));

	//wait tRW
	busywait(15); // 10us, too long. should be 15us max. IMPORTANT: when 1us - erroneous	
//	if(NAND_IS_BUSY()) idx = 1;
	while(NAND_IS_BUSY()) ;//counter++;	
	
//	if (!NAND_IS_BUSY() && idx){ 		
//		print(uart0SendByte,0,"READY/BUSY did go low then up");       
//		print(uart0SendByte,0,"\n");
//	}
//	else{
//		print(uart0SendByte,0,"error - READY/BUSY didn't go low at all!");       
//		print(uart0SendByte,0,"\n");
//	}
	
	// set io to input
	// 0-input, 1-output, therefore
	IODIR0 &= ~IO_MASK;
	
	// read sequentially
	for(idx=0; idx<len; idx++){
		// set -RE low
		IOCLR0 |= READ_ENABLE_LOW;  
		// wait tREA, tOH
		//busywait(1); // 1us too long. should be 35ns and 15ns respectively
	    // copy the data from the pins
	  	buf[idx] = SHIFT_FROM_IO(IOPIN0);  		
	    // set -RE high
		IOSET0 |= READ_ENABLE_LOW;		
	}
	
//	print(uart0SendByte,0,"read finished. ready/busy loop iterations ");       
//  printNum(uart0SendByte,0,counter);
//	print(uart0SendByte,0,"\n");
	return 0;
}

error_t nandReadPageSpare(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len){
	return nandReadPageGeneric(buf, page, offset, len, CMD_READ_PAGE_SPARE);
}

error_t nandReadPage(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len){
	return nandReadPageGeneric(buf, page, offset, len, CMD_READ_PAGE);
}

static void nandProgramInit(){
	IOSET0 |= WRITE_PROTECT_LOW;
}

static void nandProgramFinish(){
	// reset pointer to beginning of page
	nandCommand(CMD_PROGRAM_PAGE_A); /** NEW! 10.04.07*/
	
	IOCLR0 |= WRITE_PROTECT_LOW;
}

error_t nandProgramPage(uint8_t *buf, uint32_t addr, uint32_t offset, uint32_t len, uint32_t area_cmd){
	uint32_t idx = 0, counter = 0;
	
//	print(uart0SendByte,0,"\nnandProgramPage() - program page ");
//	printNum(uart0SendByte,0,addr);      
//	print(uart0SendByte,0," len=");
//	printNum(uart0SendByte,0,len);
//	print(uart0SendByte,0," offset=");
//	printNum(uart0SendByte,0,offset);
//	print(uart0SendByte,0," buf[0]=");
//	printNum(uart0SendByte,0,buf[0]);
//	print(uart0SendByte,0," area_cmd A?=");
//	printNum(uart0SendByte,0,area_cmd==CMD_PROGRAM_PAGE_A);
#ifdef DO_ECC	
	/* if we're writing an entire page*/
	if(CMD_PROGRAM_PAGE_A == area_cmd){
		/* set ecc flags, if we program whole page*/
		if(len >= NAND_TOTAL_SIZE-1){
			setECC(buf);
		}
	}
#endif
	
	nandOpInit(); // also sets READ_ENABLE, no need
	nandProgramInit();
	nandCommand(area_cmd); /** NEW! 10.04.07*/
	nandCommand(CMD_PROGRAM_PAGE);
	nandAddressThreeCycles(ADD_OFFSET_TO_ADDRESS(addr, offset));
	
	
	// set io to output
	// 0-input, 1-output, therefore
	IODIR0 |= IO_MASK;
	
	// write sequentially
	for(idx=0; idx<len; idx++){ /** len - NEW 10.04.07*/ 
		// set -RE low
		busywait(1); // too long, should be 25ns	    
		IOCLR0 |= WRITE_ENABLE_LOW;  		
	    // copy the data to the pins
	    // set 1's
	    IOSET0 |= SHIFT_TO_IO(buf[idx]);	  	
	    //set 0's
	    IOCLR0 |= SHIFT_TO_IO(~(buf[idx]));	  	
	    busywait(1); // too long, should be 10ns	  
	    // set -RE high
		IOSET0 |= WRITE_ENABLE_LOW;		
		  
	}
	idx = 0;
	
	nandCommand(CMD_PROGRAM_CONFIRM);	
//	if(NAND_IS_BUSY()) idx = 1;
	
	while(NAND_IS_BUSY());//counter++;	
	
//	if (!NAND_IS_BUSY() && idx){ 			
//		print(uart0SendByte,0,"READY/BUSY did go low then up");       
//		print(uart0SendByte,0,"\n");
//	}
//	else{
//		print(uart0SendByte,0,"error - READY/BUSY didn't go low at all!");       
//		print(uart0SendByte,0,"\n");
//	}
//	
//	print(uart0SendByte,0,"READY/BUSY counter loops ");       
//	printNum(uart0SendByte,0,counter);
//	print(uart0SendByte,0,"\n");

	// reset pointer to beginning of page
	nandProgramFinish();
	
	return nandReadStatus();
}

error_t nandProgramPageA(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len){
	return nandProgramPage(buf, page, offset, len, CMD_PROGRAM_PAGE_A);
}

error_t nandProgramPageB(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len){
	return nandProgramPage(buf, page, offset, len, CMD_PROGRAM_PAGE_B);
}

error_t nandProgramPageC(uint8_t *buf, uint32_t page, uint32_t offset, uint32_t len){
	return nandProgramPage(buf, page, offset, len, CMD_PROGRAM_PAGE_C);
}

error_t nandReadStatus(void){
	uint32_t i;
	nandOpInit();
	nandCommand(CMD_READ_STATUS);
	// set io0 to input
	// 0-input, 1-output, therefore
	IODIR0 &= ~IO_0;
	// wait tCEA
	busywait(1); // too long, should be 45ns
	IOCLR0 |= READ_ENABLE_LOW;		
	// wait tRE
	busywait(1); // too long, should be 35ns
	i = SHIFT_STATUS(IOPIN0 & IO_0);	
	IOSET0 |= READ_ENABLE_LOW;
	return i;	
}

#ifdef Debug
error_t nandBruteErase(uint32_t page_in_erase_unit){	
	nandOpInit();
//	print(uart0SendByte,0,"nandOpInit()");       	
//	print(uart0SendByte,0,"\n");
	nandProgramInit();
//	print(uart0SendByte,0,"nandProgramInit()");       	
//	print(uart0SendByte,0,"\n");
	nandCommand(CMD_ERASE_SETUP);
//	print(uart0SendByte,0,"nandCommand(CMD_ERASE_SETUP)");       	
//	print(uart0SendByte,0,"\n");
	// wait tWC
	busywait(1); // 1us too long, should be 50ns
	nandAddressTwoCycles(page_in_erase_unit);		
//	print(uart0SendByte,0,"nandAddressTwoCycles(addr)");       	
//	print(uart0SendByte,0,"\n");
	nandCommand(CMD_ERASE_CONFIRM);
//	print(uart0SendByte,0,"nandCommand(CMD_ERASE_CONFIRM)");       	
//	print(uart0SendByte,0,"\n");
	//wait tWB
	busywait(1); //us too long ,should be max 100ns
	while(NAND_IS_BUSY());
	nandProgramFinish();
	
	return nandReadStatus();
}
#endif

error_t nandErase(uint32_t page_in_erase_unit){	
	if(!nandCheckEuStatus(page_in_erase_unit)){
		return 0;
	}
	
	nandOpInit();
//	print(uart0SendByte,0,"nandOpInit()");       	
//	print(uart0SendByte,0,"\n");
	nandProgramInit();
//	print(uart0SendByte,0,"nandProgramInit()");       	
//	print(uart0SendByte,0,"\n");
	nandCommand(CMD_ERASE_SETUP);
//	print(uart0SendByte,0,"nandCommand(CMD_ERASE_SETUP)");       	
//	print(uart0SendByte,0,"\n");
	// wait tWC
	busywait(1); // 1us too long, should be 50ns
	nandAddressTwoCycles(page_in_erase_unit);		
//	print(uart0SendByte,0,"nandAddressTwoCycles(addr)");       	
//	print(uart0SendByte,0,"\n");
	nandCommand(CMD_ERASE_CONFIRM);
//	print(uart0SendByte,0,"nandCommand(CMD_ERASE_CONFIRM)");       	
//	print(uart0SendByte,0,"\n");
	//wait tWB
	busywait(1); //us too long ,should be max 100ns
	while(NAND_IS_BUSY());
	nandProgramFinish();
	
	return nandReadStatus();
}

void nandTerminate(){
}

/**
 * verify eu status. read phy_addr of an eu, and perform eu validiy check
 * return 1 if eu is ok, 0 otherwise
 */
error_t nandCheckEuStatus(uint32_t phy_addr){	
	uint8_t spare_buf[NAND_SPARE_SIZE];	
	
//	print(uart0SendByte,0,"\nnandCheckEuStatus() - starting. read");
//	printNum(uart0SendByte,0,phy_addr & NAND_EU_MASK);			
	nandReadPageSpare(spare_buf, phy_addr & NAND_EU_MASK,0,NAND_SPARE_SIZE);	 			 	
//	print(uart0SendByte,0,"\nnandCheckEuStatus() - nandReadPageSpare success");
	
	/* verify the eu isn't bad
	 * check bad eu flag in first two pages of the EU (as required by the datasheet)*/
	if (spare_buf[NAND_BAD_EU_FLAG_BYTE_NUM-1] == 0xff){				
		nandReadPageSpare(spare_buf, (phy_addr & NAND_EU_MASK) + 1,0,NAND_SPARE_SIZE);			 

//		print(uart0SendByte,0,"\nnandCheckEuStatus() - secong page erased?");
//		printNum(uart0SendByte,0,(spare_buf[NAND_BAD_EU_FLAG_BYTE_NUM-1] == 0xff));	
		return (spare_buf[NAND_BAD_EU_FLAG_BYTE_NUM-1] == 0xff);
	}	
	
//	if(phy_addr == 2610){
//		print(uart0SendByte,0,"\nnandCheckEuStatus() - first page failure");
//		uint32_t i;
//		for(i=0; i< NAND_SPARE_SIZE; i++){
//			print(uart0SendByte,0,"\n");
//			printNum(uart0SendByte,0,i);
//			print(uart0SendByte,0,"\n. ");
//			printNum(uart0SendByte,0,spare_buf[i]);		
//		}
//	}
	
	return 0;
}

error_t markEuAsBad(uint32_t page_addr){
	uint8_t bad_byte = 0x00;
//	PRINT_MSG_AND_NUM("\nmarkEuAsBad() - marking eu as bad in address ",page_addr);
	// mark eu as bad  (if not already marked)
	if(!nandCheckEuStatus(page_addr)){		
//		PRINT("\nmarkEuAsBad() - EU is already bad");
		return 1;
	}
	
//	PRINT_MSG_AND_NUM("\nmarkEuAsBad() - mark page as bad ",page_addr & NAND_EU_MASK);
	if(nandProgramPageC(&bad_byte, page_addr & NAND_EU_MASK, NAND_BAD_EU_FLAG_BYTE_NUM-1, 1)){		
		return 0;
	}
//	PRINT("\nmarkEuAsBad() - EU is marked as bad");
	return 1;
}
