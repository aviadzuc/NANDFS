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

/******************************************************/
/* SERIAL0.C : Initial UART0 For ET-ARM STAMP LPC2119 */
/******************************************************/

#include <lpc2000/uart.h>
#include <lpc2000/clocks.h>

/******************************/
/* Initial UART0 = 9600,N,8,1 */
/* VPB(pclk) = 29.4912 MHz    */
/******************************/
void uart0Init(uint32_t baud_rate)  
{
  uint32_t pclk_freq;	
  uint32_t divisor;
	
  PINSEL0 &= 0xFFFFFFF0;						// Reset P0.0,P0.1 Pin Config
  PINSEL0 |= 0x00000001;						// Select P0.0 = TxD(UART0)
  PINSEL0 |= 0x00000004;						// Select P0.1 = RxD(UART0)

  U0LCR &= 0xFC;								// Reset Word Select(1:0)
  U0LCR |= 0x03;								// Data Bit = 8 Bit
  U0LCR &= 0xFB;								// Stop Bit = 1 Bit
  U0LCR &= 0xF7;								// Parity = Disable
  U0LCR &= 0xBF;								// Disable Break Control
  U0LCR |= 0x80;								// Enable Programming of Divisor Latches

  // U0DLM:U0DLL = 29.4912MHz / [16 x Baud]
  //             = 29.4912MHz / [16 x 9600]
  //             = 192 = 0x00C0
  
  pclk_freq = clocksGetPclkFreq();
  divisor   = pclk_freq / (baud_rate << 4);     // divide by baud_rate*16
  
  U0DLM = (divisor & 0x0000FF00) >> 8;
  U0DLL = (divisor & 0x000000FF);

  /*
  {
  	char s[17];
    sprintf(s,"%08x",U0DLL);
    lcd_goto_cursor(0x00);
  	lcd_print(s);
    sprintf(s,"%08x",U0DLM);
    lcd_goto_cursor(0x40);
  	lcd_print(s);
  }
  */

  U0LCR &= 0x7F;								// Disable Programming of Divisor Latches

  U0FCR |= 0x01;								// FIF0 Enable
  U0FCR |= 0x02;								// RX FIFO Reset
  U0FCR |= 0x04;								// TX FIFO Reset
  U0FCR &= 0x3F;                      

}

/****************************/
/* Write Character To UART0 */
/****************************/
void uart0SendByte(void* env, uint8_t ch)  
{                  
  if (ch == '\n')  
  {
    while (!(U0LSR & 0x20));  					// Wait TXD Buffer Empty
    U0THR = CR;                          		// Write CR
  }
  while (!(U0LSR & 0x20));						// Wait TXD Buffer Empty
  U0THR = ch;
  //return (U0THR = ch);							// Write Character
}

int putchar (int ch)  
{                  
  if (ch == '\n')  
  {
    while (!(U0LSR & 0x20));  					// Wait TXD Buffer Empty
    U0THR = CR;                          		// Write CR
  }
  while (!(U0LSR & 0x20));						// Wait TXD Buffer Empty
  return (U0THR = ch);							// Write Character
}

/*****************************/
/* Read Character From UART0 */
/*****************************/
int getchar (void)  
{                    
  while (!(U0LSR & 0x01));						// Wait RXD Receive Data Ready
  return (U0RBR);								// Get Receice Data & Return
}
