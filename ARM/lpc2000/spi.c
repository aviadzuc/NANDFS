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
 * SPI
 * 
 * Sivan Toledo
 */

#include <LPC21xx.h>
#include <types.h>
#include <clocks.h>
#include <interrupts.h>
#include <gpio.h>
#include <spi.h>

// for debugging...

#include "uart.h"
#include "timers.h"
#include "interrupts.h"
#include "print.h"

/*
 * A master transaction.
 * The master sends to the named slave the command,
 * and expects a response of a given length, which
 * it writes into the response buffer. It returns
 * the number of bytes received from the slave, or
 * a negative number in case of an error.
 */

static volatile uint32_t status;
 
static volatile uint8_t* mt_command;
static volatile int32_t  mt_command_len;
static volatile int32_t  mt_command_ptr;
static volatile uint8_t* mt_response;
static volatile int32_t  mt_response_len;
static volatile int32_t  mt_response_ptr;
static volatile bool     mt_done;
 
int32_t spiMasterTransact(uint8_t* command, 
                          int32_t  command_len,
                          uint8_t* response,
                          int32_t  response_len)
{
  mt_done = false;
  mt_command      = command;
  mt_command_len  = command_len;
  mt_response     = response;
  mt_response_len = response_len;
  mt_command_ptr  = 0;
  mt_response_ptr = 0;
  
  //if (command_len > 0) {
  //  print(uart0SendByte,0,"command: ");
  //  printHex(uart0SendByte,0,command[0],2);
  //  print(uart0SendByte,0,"\n");
  //}
  //print(uart0SendByte,0,"start asserted...\n");

  // here we go...
  
  print(uart0SendByte,0,"spi status register: ");
  printHex(uart0SendByte,0,S0SPSR,2);
  print(uart0SendByte,0,"\n");	

  gpioClear(0,2);  // CS
  gpioClear(0,8);
  mt_command_ptr++;
  S0SPDR = *mt_command;	// Send first byte
  
  while (!mt_done) {
    print(uart0SendByte,0,"spi status register: ");
    printHex(uart0SendByte,0,S0SPSR,2);
    print(uart0SendByte,0,"\n");	
  }
	
  print(uart0SendByte,0,"spi status register: ");
  printHex(uart0SendByte,0,S0SPSR,2);
  print(uart0SendByte,0,"\n");	
  
  return mt_response_ptr;
}

void 
  __attribute__ ((interrupt("IRQ")))
  SPI_ISR(void) 
{
  printNum(uart0SendByte,0,mt_command_ptr);
  print(uart0SendByte,0," ");	
  printNum(uart0SendByte,0,mt_response_ptr);
  print(uart0SendByte,0,"\n");	

  if (mt_command_ptr < mt_command_len) {
    S0SPDR = mt_command[mt_command_ptr];
    mt_command_ptr++;
  } else {
  	if (mt_response_ptr < mt_response_len) {
      mt_command[mt_response_ptr] = S0SPDR;
      mt_response_ptr++;
  	}
  }
  
  if (mt_command_ptr >= mt_command_len && mt_response_ptr >= mt_response_len) {
    gpioSet(0,3);  // !CS
    gpioSet(0,8);
  	mt_done = true;
  }
  
  S0SPINT =	0x01;
  interruptUpdatePriority();
}


void spiInit(uint32_t bitrate)
{
  // 
  PINSEL0 &= 0xFFFF00FF;          // P0.4,5,6,7
  PINSEL0 |= 0x00005500;          // P0.4,5,6,7

  gpioEnable(0,3,GPIO_OUTPUT);    // !WP
  gpioEnable(0,2,GPIO_OUTPUT);    // !CS
  gpioSet(0,3);
  gpioSet(0,2);
  
  //busywait(10000000);

  if (bitrate != 0) {
  	uint32_t divider;
  	uint32_t bitrate_times_divider;
  	uint32_t pclk = clocksGetPclkFreq();
  	for (divider = 1, bitrate_times_divider = bitrate; 
  	     bitrate_times_divider <= pclk; 
  	     divider++, bitrate_times_divider += bitrate);
  	divider--; // we went over the desired bit rate, adjust
  	
  	if (divider < 8) divider = 8; // must be 8 or larger
  	if (divider & 1) divider++;   // must be even
  	
    print(uart0SendByte,0,"spi pclk: ");
    printNum(uart0SendByte,0,pclk);
    print(uart0SendByte,0,"\n");

    print(uart0SendByte,0,"spi bitrate: ");
    printNum(uart0SendByte,0,bitrate);
    print(uart0SendByte,0,"\n");
  
    print(uart0SendByte,0,"spi divider: ");
    printNum(uart0SendByte,0,divider);
    print(uart0SendByte,0,"\n"); 
    
    S0SPCCR = divider;
	S0SPCR = 0x000000A0;			//Configure as SPI Master interrupts enabled
  } 
    
  interruptEnableVIC(INT_CHANNEL_SPI0, 
                     8, // priority
                     SPI_ISR);
  //VICVectAddr7  = (uint32_t) &I2_Isr;
  //VICVectCntl7  = 0x29;         // Channel1 on Source#9 ... enabled
  //VICIntEnable |= 0x200;        // 9th bit is the I2C
}

