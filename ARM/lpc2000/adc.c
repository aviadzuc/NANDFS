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
 * adc.c
 * 
 * Sivan Toledo, 2007
 */
 
#include <lpc21xx.h>
#include <clocks.h>
#include <interrupts.h>
/*
  #include <uart.h>
  #include <print.h>
*/

/* 
 * Initialize the clock divider, but that's it.
 */
void adcInit() {
  uint32_t adcclk;
  uint8_t  clkdiv;
  uint32_t pclk = clocksGetPclkFreq();
  
  for (adcclk = 4500000, clkdiv=0; 
       adcclk < pclk; 
       adcclk+=4500000, clkdiv++);
  
  ADCR = (clkdiv << 8);

/*  
  print(uart0SendByte,0,"pclk: ");
  printNum(uart0SendByte,0,pclk);
  print(uart0SendByte,0,"\n");
  
  print(uart0SendByte,0,"clkdiv: ");
  printNum(uart0SendByte,0,clkdiv);
  print(uart0SendByte,0,"\n");
*/
}

void adcEnable() {
  ADCR |=  (1 << 21); 
}

void adcDisable() {
  // clear power down (bit 21), START=000 (bits 24-26)
  ADCR &= 0xF8DFFFFF; 
}

void adcInputEnable(uint8_t ain) {
  // ADC0 is GPIO-0.27, 01 in bits 22,23
  switch (ain) {
  	case 0:
      PINSEL1 &= 0xFF7FFFFF;
      PINSEL1 |= 0x00400000;
      break;
    case 1:
      PINSEL1 &= 0xFDFFFFFF;
      PINSEL1 |= 0x01000000;
      break;
    case 2:
      PINSEL1 &= 0xF7FFFFFF;
      PINSEL1 |= 0x04000000;
      break;
    case 3:
      PINSEL1 &= 0xDFFFFFFF;
      PINSEL1 |= 0x10000000;
      break;
  }
}

uint16_t adcOnce(uint8_t ain) {
  uint32_t reading;
  
  ADCR |= ain
          | 0x01000000 // start=001, start now
          ;
          
  do {
    reading = ADDR;
  } while ((reading & 0x80000000) == 0); // Wait for DONE bit (31) to set	 

  return reading & 0x0000FFFF;	 	  
}

// read the value, we know it's ready (from an interrupt)

uint16_t adcReadReady(uint8_t ain) {
  uint32_t reading = ADDR;
  return (uint16_t) (reading & 0x0000FFFF);	 	  
}

// this is hardcoded for now to MAT0.1, rising edge
void adcEveryMatch(uint8_t ain) {
  ADCR |= ain
          | 0x0C000000 // start=100, MAT0.1, and rising edge
          ;
}

