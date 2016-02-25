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
 * interrupts.h
 * 
 * Sivan Toledo.
 */

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <system.h>

typedef enum {
  INT_CHANNEL_WDT    = 0,
  INT_CHANNEL_SWI    = 1,
  INT_CHANNEL_CORE1  = 2,
  INT_CHANNEL_CORE2  = 3,
  INT_CHANNEL_TIMER0 = 4,
  INT_CHANNEL_TIMER1 = 5,
  INT_CHANNEL_UART0  = 6,
  INT_CHANNEL_UART1  = 7,
  INT_CHANNEL_PWM0   = 8,
  INT_CHANNEL_I2C    = 9,
  INT_CHANNEL_SPI0   = 10,
  INT_CHANNEL_SPI1   = 11,
  INT_CHANNEL_PLL    = 12,
  INT_CHANNEL_RTC    = 13,
  INT_CHANNEL_EINT0  = 14,
  INT_CHANNEL_EINT1  = 15,
  INT_CHANNEL_EINT2  = 16,
  INT_CHANNEL_EINT3  = 17,
  INT_CHANNEL_ADC    = 18,
  INT_CHANNEL_CANACC = 19,
  INT_CHANNEL_CAN1TX = 20,
  INT_CHANNEL_CAN2TX = 21,
  INT_CHANNEL_CAN3TX = 22,
  INT_CHANNEL_CAN4TX = 23,
  INT_CHANNEL_CAN1RX = 26,
  INT_CHANNEL_CAN2RX = 27,
  INT_CHANNEL_CAN3RX = 28,
  INT_CHANNEL_CAN4RX = 29
} interrupt_channel_t;

void interruptEnableFIQ(interrupt_channel_t c);
void interruptEnableVIC(interrupt_channel_t c, 
                        uint16_t            priority,
                        interrupt_handler_t h);
                        
// this must be done near the end of an ISR to
// update the priority hardware
                        
#define interruptUpdatePriority() (VICVectAddr=0xff)

#define disable_interrupts()											\
  asm volatile (															\
		"STMDB	SP!, {R0}		\n\t"	/* Push R0.						*/	\
		"MRS	R0, CPSR		\n\t"	/* Get CPSR.					*/	\
		"ORR	R0, R0, #0xC0	\n\t"	/* Disable IRQ, FIQ.			*/	\
		"MSR	CPSR, R0		\n\t"	/* Write back modified value.	*/	\
		"LDMIA	SP!, {R0}			" )	/* Pop R0.						*/
			
#define enable_interrupts()												\
  asm volatile (															\
		"STMDB	SP!, {R0}		\n\t"	/* Push R0.						*/	\
		"MRS	R0, CPSR		\n\t"	/* Get CPSR.					*/	\
		"BIC	R0, R0, #0xC0	\n\t"	/* Enable IRQ, FIQ.				*/	\
		"MSR	CPSR, R0		\n\t"	/* Write back modified value.	*/	\
		"LDMIA	SP!, {R0}			" )	/* Pop R0.						*/

#endif                             
