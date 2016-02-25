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
 * clocks.c
 * 
 * Sivan Toledo.
 */
 
#include <lpc2000/clocks.h>

/*****************************************************************/
/* PLL Setup                                                     */
/*                                                               */
/* Must be performed with interrupts disabled!                   */
/*****************************************************************/

/*
 The PLL equations use the following parameters:
   Fosc the frequency from the crystal oscillator
   Fcco the frequency of the PLL current controlled oscillator
   Fcclk the PLL output frequency (also the processor clock frequency)
   M PLL Multiplier value from the MSEL bits in the PLLCFG register
   P PLL Divider value from the PSEL bits in the PLLCFG register
   The PLL output frequency (when the PLL is both active and connected) is given by:
     Fcclk = M * Fosc 
  The CCO frequency can be computed as:
     Fcco = Fcclk * 2 * P = Fosc * M * 2 * P
  The PLL inputs and settings must meet the following:
  + Fosc is in the range of 10 MHz to 25 MHz.
  + cclk is in the range of 10 MHz to Fmax (the maximum allowed frequency for the LPC2119/2129/2194/2292/2294).
  + Fcco is in the range of 156 MHz to 320 MHz.
  
  Note that M is encoded in 5 bits that specify M-1
  and that the base-2 of P is encoded in another 2 bits.
 */

#define MAX_CCLK  60000000
#define MIN_CCO  156000000

void clocksInit(uint32_t min_cclk, uint32_t vpb_divider) {
  uint32_t M_minus_one;     // 5 bits, binary encoding of (M-1)
  uint32_t log_P;           // 2 bits, binary encoding of log P
  uint32_t cclk = 0, pclk;
/*
  for (M_minus_one = 0; 
      (M_minus_one+1)*F_OSC < min_cclk; 
      M_minus_one++);
  if ((M_minus_one+1)*F_OSC > MAX_CCLK) M_minus_one = 0; // make sure we don't go over 60MHz
*/

  for (M_minus_one = 0, cclk = F_OSC; 
      cclk < min_cclk; 
      M_minus_one++, cclk += F_OSC);
  if (cclk > MAX_CCLK) M_minus_one = 0; // make sure we don't go over 60MHz

  for (log_P = 0; F_OSC*(M_minus_one+1)*2*(1<<log_P) < MIN_CCO; log_P++);
	
  PLLCFG  = M_minus_one | (log_P << 5);
  //PLLCFG  = 0x00000060;
  
  // Enable the PLL
  
  PLLCON  = 0x00000001; 
  
  // Tell the PLL to configure itself
  
  PLLFEED = 0x000000AA;
  PLLFEED = 0x00000055;
  
  // Wait for the lock bit to set
  
  while (!(PLLSTAT & 0x00000400)); 

  // Locked, connect it!
    
  PLLCON = 0x00000003; 

  PLLFEED = 0x000000AA;
  PLLFEED = 0x00000055;
  
  // Set up the VPB Clock
  
  switch (vpb_divider) {
  	case 1: 
  	  VPBDIV = 0x00000001; 
  	  pclk = cclk;
  	  break;
  	case 2: 
  	  VPBDIV = 0x00000002; 
  	  pclk = cclk >> 1;
  	  break;
  	case 4: 
  	  VPBDIV = 0x00000000; 
  	  pclk = cclk >> 2;
  	  break;
  }
  
  // set up the frequency scaling for the real-time clock
    
  PREINT  = (pclk >> 15) - 1;
  PREFRAC = pclk - ((PREINT+1) << 15);
}

uint32_t clocksGetCclkFreq() {
  return F_OSC * ((PLLCFG & 0x0000001F)+1);
}

/*
 * reading from VPBDIV is buggy on some LPC2000's;
 * the workaround is to read it twice. The second read
 * returns the correct value;
 */
uint32_t clocksGetPclkFreq() {
  uint32_t cclk_freq = F_OSC * ((PLLCFG & 0x0000001F)+1);
  uint32_t vpbdiv;
  
  // read and dummy use, to make sure VPBDIV is read
  vpbdiv = VPBDIV;
  if (vpbdiv == 0xFFFFFFFF) return vpbdiv; 

  vpbdiv = VPBDIV;  
  switch (vpbdiv & 0x00000003) {
  	case 0x00000001: return cclk_freq; break;
  	case 0x00000002: return cclk_freq >> 1; break;
  	case 0x00000000: return cclk_freq >> 2; break;
  }
}



