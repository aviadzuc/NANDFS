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
 * tau-arm.c
 * 
 * Sivan Toledo.
 */
 
#include <lpc2000/busywait.h>
#include <lpc2000/clocks.h>

/*
 * Here msec is 1/1024 of a second,
 * and a usec is 1/1048576 of a second
 */

volatile uint32_t busywait_dummy;
uint32_t busywait_iters_per_msec;

/* 
 * wait u microseconds
 * or   u/1024 = u >> 10 milliseconds
 * or   (u>>10) * busywait_iters_per_msec iterations
 * 
 * We have to be careful of overflows...
 */

void busywait(uint32_t usec) {
  uint32_t i, iters;
  if (usec > 32768)
    iters = ((usec >> 5)*busywait_iters_per_msec) >> 5;
  else
    iters = (usec*busywait_iters_per_msec) >> 10;
  for (i = 0; i<iters; i++)
    busywait_dummy++;
}

/*****************************************************************/
/* Caliberate the delay loop                                     */
/* Resources: uses timer 0                                       */
/*****************************************************************/

void busywaitInit() {
  uint32_t pclk_count;
  uint32_t iter_count = 1048576;    
  uint32_t pclk_freq = clocksGetPclkFreq();

  busywait_iters_per_msec = 1;

  T0TCR = 2; // disable and reset timer 0
  T0PR  = 0; // no prescaling
  T0TCR = 1; // enable the timer
  
  busywait(iter_count);
  
  pclk_count = T0TC; // read the counter of Timer 0

  T0TCR = 2; // disable and reset timer 0
  
  
  /*
   * the delay loop performs about
   *   pclk_freq*iter_count / (pclk_count * 1000000)
   * iterations per microsecond
   */
   
  busywait_iters_per_msec = pclk_freq / pclk_count;
  
  /*
  lcd_init();                                            
  lcd_goto_cursor(0x40);
  sprintf(string,"c=%u",pclk_count);
  lcd_print(string);
  //printf("c=%u\n",pclk_count);
  lcd_goto_cursor(0x00);
  sprintf(string,"ipm=%u",busywait_iters_per_msec);
  lcd_print(string);
  //printf("ipm=%u\n",busywait_iters_per_msec);
  */
}
