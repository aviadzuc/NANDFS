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
 * i2c Slave code
 * 
 * Slave code adapted from Philips/NXP Technical Note TN06005
 *
 * Sivan Toledo
 */

#include <LPC21xx.h>
#include <types.h>
#include <clocks.h>
#include <interrupts.h>
#include <i2c.h>

enum {
  I2C_ASSERT_ACK = 0x04,
  I2C_INTERRUPT  = 0x08,
  I2C_STOP       = 0x10,
  I2C_START      = 0x20,
  I2C_ENABLE     = 0x40
};

// for debugging...

#include "uart.h"
#include "timers.h"
#include "interrupts.h"
#include "print.h"


/*
static unsigned char ADC_Read(void)
{
unsigned int i;
AD0CR = 0x00200302; // Init ADC (Pclk = 12MHz) and select channel AD0.1
AD0CR |= 0x01000000; // Start A/D Conversion
do
{
i = AD0DR; // Read A/D Data Register
} while ((i & 0x80000000) == 0); // Wait for end of A/D Conversion
return (i >> 8) & 0x00FF; // bit 8:15 is 8-bit AD value
}
void main(void)
{
PINSEL1 |= 0x01000000; // P0.28 = AD0.1
IODIR1 = 0x00FF0000; // P1.16..23 defined as Outputs
I2_Init(); // initialize I2C bus
while (1)
{
SlaveSnd = ADC_Read(); // convert and send channel AD0.1
}
}
*/

/*
 * A master transaction.
 * The master sends to the named slave the command,
 * and expects a response of a given length, which
 * it writes into the response buffer. It returns
 * the number of bytes received from the slave, or
 * a negative number in case of an error.
 */
 
static volatile uint8_t  mt_slave;
static volatile uint8_t* mt_command;
static volatile int32_t mt_command_len;
static volatile int32_t mt_command_ptr;
static volatile uint8_t* mt_response;
static volatile int32_t mt_response_len;
static volatile int32_t mt_response_ptr;
static volatile bool     mt_done;
 
int32_t i2cMasterTransact(uint8_t  slave, 
                          uint8_t* command, 
                          int32_t  command_len,
                          uint8_t* response,
                          int32_t  response_len)
{
  mt_done = false;
  mt_slave        = slave;
  mt_command      = command;
  mt_command_len  = command_len;
  mt_response     = response;
  mt_response_len = response_len;
  mt_command_ptr  = 0;
  mt_response_ptr = 0;
  // we should disable i2c interrupts first
  
  //if (command_len > 0) {
  //  print(uart0SendByte,0,"command: ");
  //  printHex(uart0SendByte,0,command[0],2);
  //  print(uart0SendByte,0,"\n");
  //}
  //print(uart0SendByte,0,"start asserted...\n");

  // here we go...
  I2CONSET = I2C_START;
  
  // now enable interrupts
  
  while (!mt_done);
	
  //print(uart0SendByte,0,"i2c result/length: ");
  //printNum(uart0SendByte,0,mt_response_ptr);
  //print(uart0SendByte,0,"\n");	
  
  return mt_response_ptr;
}

unsigned char SlaveRcv  = 0xAA;
unsigned char SlaveSnd  = 234;
void 
  __attribute__ ((interrupt("IRQ")))
  I2_Isr(void) 
{
  uint8_t status = I2STAT;
  
  //print(uart0SendByte,0,"i2c isr: \n");
  //printHex(uart0SendByte,0,status,2);
  //print(uart0SendByte,0,"\n");	
  //gpioClear(0,8);
  
  switch(status) {
  	case 0x08: // START has been transmitted
  	case 0x10: // repeated START has been transmitted
  	if (mt_command_ptr == mt_command_len) // no more to send
  	  I2DAT = mt_slave | 0x01; // set read bit in address LSB
  	else
  	  I2DAT = mt_slave; // write bit is zero in address LSB
  	I2CONCLR = I2C_INTERRUPT | I2C_STOP | I2C_START;
  	break;

  	case 0x18: // slave address + W transmitted
  	case 0x28: // Data has been transmitted in master transmitter mode
  	if (mt_command_ptr < mt_command_len) { // send more
  	  I2DAT = mt_command[ mt_command_ptr ];
  	  mt_command_ptr++;
      I2CONCLR = I2C_INTERRUPT | I2C_START | I2C_STOP;
  	} else { // nothing more to send
  	  if (mt_response_len == 0) { // no response required
        I2CONSET = I2C_STOP;
        I2CONCLR = I2C_INTERRUPT | I2C_START;
        mt_done = true;
  	  } else { // send repeated start
        I2CONSET = I2C_START;
        I2CONCLR = I2C_INTERRUPT | I2C_STOP;
  	  }
  	}
  	break;
  	
  	// I am assuming that the slave might respond with a NACK
  	// when it knows that the master has sent all the data.
  	case 0xFF: // 0x30: // NOT ACK, master transmitter mode after data
  	if (mt_command_ptr < mt_command_len) { // send more
      I2CONSET = I2C_STOP;
      I2CONCLR = I2C_INTERRUPT | I2C_START;
      mt_done = true;
      mt_response_ptr = -1; // NACK error
  	} else { // nothing more to send
  	  if (mt_response_len == 0) { // no response required
        I2CONSET = I2C_STOP;
        I2CONCLR = I2C_INTERRUPT | I2C_START;
        mt_done = true;
  	  } else { // send repeated start
        I2CONSET = I2C_START;
        I2CONCLR = I2C_INTERRUPT | I2C_STOP;
  	  }
  	}
    break;        

  	case 0x20: // NOT ACK, master transmitter mode after SLA+W
  	case 0x48: // NOT ACK, master receiver mode after SLA+R
  	case 0x30: // NOT ACK, master transmitter mode after data
    I2CONSET = I2C_STOP;
    I2CONCLR = I2C_INTERRUPT | I2C_START;
    mt_done = true;
    mt_response_ptr = -status; // NACK error
    break;        
  	
  	case 0x38: // arbitration lost in master mode
    I2CONCLR = I2C_INTERRUPT | I2C_START | I2C_STOP;
    mt_done = true;
    mt_response_ptr = -status; // arbitration error
    break;
    
  	case 0x40: // ACK after SLA+R in master receiver mode
  	if (mt_response_ptr < mt_response_len-1)
      I2CONSET = I2C_ASSERT_ACK; // master wants another byte after next
    else
      I2CONCLR = I2C_ASSERT_ACK; // after next byte, master received enough data  		
    I2CONCLR = I2C_INTERRUPT | I2C_START | I2C_STOP;
    break;        

  	case 0x58: // master received byte, sent NOT ACK
  	mt_response[ mt_response_ptr ] = I2DAT;
  	mt_response_ptr++;
    I2CONSET = I2C_STOP;
    I2CONCLR = I2C_INTERRUPT | I2C_START;
    mt_done = true;
    break;
    
  	case 0x50: // master received byte, sent ACK
  	mt_response[ mt_response_ptr ] = I2DAT;
  	mt_response_ptr++;
  	if (mt_response_ptr < mt_response_len-1)
      I2CONSET = I2C_ASSERT_ACK; // master wants another byte after next
    else
      I2CONCLR = I2C_ASSERT_ACK; // after next byte, master received enough data  		
    I2CONSET = I2C_STOP;
    I2CONCLR = I2C_INTERRUPT | I2C_START | I2C_STOP;
    break;        
   
    
    // slave mode from here        
  	
    case 0x60: // own SLA+W received, Ack returned (slave receiver)
	case 0x68: // Addressed as slave
    I2CONCLR = 0x2C; // clear STA (start), AA (ack) and SI (interrupt flag)
	I2CONSET = 0x04; // set AA, return ACK on first byte
	break;

	case 0x80:        // Data received, ACK returned
    I2CONCLR = 0x2C; // clear STA (start), AA (ack) and SI (interrupt flag)
	SlaveRcv = I2DAT; // read and store data, NACK on next byte (we are done)
	break;
	
	case 0x88: // data received, NACK returned
	case 0xA0: // STOP or REP.START received while addressed as slave
	case 0xC0: // Data transmitted, NOT ACK received
	case 0xC8: // Last data transmitted, ACK received
    I2CONCLR = 0x2C; // clear STA (start), AA (ack) and SI (interrupt flag)
	I2CONSET = 0x04; // set AA, switch to not addressed slave mode
	break;
	
	case 0xA8: // own SLA+R received, Ack returned (slave transmitter)
	case 0xB8: // Data transmitted, ACK received
    I2CONCLR = 0x2C; // clear STA (start), AA (ack) and SI (interrupt flag)
	I2DAT = SlaveSnd; // Transmit last data AA = 0
	break;

	default:
	break;	
  }

  interruptUpdatePriority();
}


/*
 * if the lsb of the slave address is set (it is not part of the
 * address since the lsb of the address from the master is used 
 * for the r/w flag) then the MCU will respond to the general
 * call address; if it is 0, it will ignore general calls.
 */
 
void i2cInit(uint32_t bitrate, uint8_t slave_address)
{
  PINSEL0      |= 0x50;          // P0.3 = SDA, P0.2 = SCL
  
  I2ADR       = slave_address;
  
  if (bitrate != 0) {
  	uint32_t divider;
  	uint32_t bitrate_times_divider;
  	uint32_t pclk = clocksGetPclkFreq();
  	for (divider = 1, bitrate_times_divider = bitrate; 
  	     bitrate_times_divider <= pclk; 
  	     divider++, bitrate_times_divider += bitrate);
  	divider--; // we went over the desired bit rate, adjust
  	
    print(uart0SendByte,0,"pclk: ");
    printNum(uart0SendByte,0,pclk);
    print(uart0SendByte,0,"\n");

    print(uart0SendByte,0,"bitrate: ");
    printNum(uart0SendByte,0,bitrate);
    print(uart0SendByte,0,"\n");
  
    print(uart0SendByte,0,"divider: ");
    printNum(uart0SendByte,0,divider);
    print(uart0SendByte,0,"\n"); 
    
    I2SCLH = (divider >> 1);
    I2SCLL = divider - (divider >> 1);
    
    print(uart0SendByte,0,"I2SCLH: ");
    printNum(uart0SendByte,0,I2SCLH);
    print(uart0SendByte,0,"\n"); 

    print(uart0SendByte,0,"I2SCLL: ");
    printNum(uart0SendByte,0,I2SCLL);
    print(uart0SendByte,0,"\n"); 

    //busywait(10*1000000); 
  } 
  
  I2CONSET    = I2C_ENABLE | I2C_ASSERT_ACK;
  
  interruptEnableVIC(INT_CHANNEL_I2C, 
                     7, // priority
                     I2_Isr);
  //VICVectAddr7  = (uint32_t) &I2_Isr;
  //VICVectCntl7  = 0x29;         // Channel1 on Source#9 ... enabled
  //VICIntEnable |= 0x200;        // 9th bit is the I2C
}

