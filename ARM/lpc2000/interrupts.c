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
 * interrupts.c
 * 
 * Sivan Toledo.
 */
 
#include <lpc2000/interrupts.h>

volatile uint32_t* address_register = &VICVectAddr0;
volatile uint32_t* control_register = &VICVectCntl0;

// the address of the FIQ handler must be stored in
// the interrupt vector table, which might be in flash
void interruptEnableFIQ(interrupt_channel_t c) {
  VICIntSelect |= c; // 1 means generate FIQ                             	  
  VICIntEnable |= c;
}

void interruptEnableVIC(interrupt_channel_t c, 
                        uint16_t            priority,
                        interrupt_handler_t h) {
  address_register[priority] = (uint32_t) h;
  control_register[priority] = c | (1<<5);
  VICIntSelect &= ~(1 << c); // 0 means generate a vectored interrupt                            	  
  VICIntEnable |= (1 << c);
}
