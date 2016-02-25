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
 * timers.c
 * 
 * Sivan Toledo.
 */
 
#include "types.h"
#include "lpc21xx.h"
#include "timers.h"
#include "uart.h"
#include "print.h"

volatile uint32_t* timers[] = { &T0IR, &T1IR };

typedef enum {
  TxIR  = 0,
  TxTCR = 1,
  TxTC  = 2,
  TxPR  = 3,
  TxPC  = 4,
  TxMCR = 5,
  TxMR0 = 6,
  TxMR1 = 7,
  TxMR2 = 8,
  TxMR3 = 9,
  TxCCR = 10,
  TxCR0 = 11,
  TxCR1 = 12,
  TxCR2 = 13,
  TxCR3 = 14,
  TxEMR = 15
} timer_registers;

void timerInit(timer_t t, uint32_t prescale)
{
  volatile uint32_t* timer = timers[t];
  
  timer[TxTCR] = 0; // disabled
  timer[TxPR]  = prescale-1;
}

void timerEnable(timer_t t)
{
  volatile uint32_t* timer = timers[t];
  
  timer[TxTCR] = 3; // enable, reset the counter
  timer[TxTCR] = 1; // enabled and running
}

void timerDisable(timer_t t)
{
  volatile uint32_t* timer = timers[t];
  
  timer[TxTCR] = 0; // disable
}

void timerSetMatch(timer_t       t, 
                   timer_match_t m,
                   uint32_t      match_value,
                   bool          interrupt,
                   bool          reset,
                   bool          stop)
{
  volatile uint32_t* timer = timers[t];
  uint32_t bits;

  timer[TxMR0+m] = match_value;

  bits = 7;   // set the three lower bits and shift
  bits <<= m;
  bits <<= m;
  bits <<= m;
  timer[TxMCR] &= ~bits; // zero the relevant bits
  if (interrupt) bits  = 1;
  if (reset    ) bits |= 2;
  if (stop     ) bits |= 4;
  bits <<= m;
  bits <<= m;
  bits <<= m;
  timer[TxMCR] |= bits; // set the appropriate bits    
  printHex(uart0SendByte,0,timer[TxMCR],8);	
}

void timerClearMatchInterrupt(timer_t t, timer_match_t m) {
  volatile uint32_t* timer = timers[t];

  timer[TxIR] = 1 << m;
}

void timerSetExternalMatch(timer_t              t, 
                           timer_match_t        m,
                           timer_match_action_t action)
{
  volatile uint32_t* timer = timers[t];
  uint32_t emr = action;

  action <<= 4; // skip the state bits
  // then two bits per match register
  action <<= m;
  action <<= m;

  timer[TxEMR] = action;  
}

