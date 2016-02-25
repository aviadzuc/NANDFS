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
 
#ifndef TIMERS_H
#define TIMERS_H

#include <system.h>

typedef enum {
  TIMER0 = 0,
  TIMER1 = 1
} timer_t;

typedef enum {
  TIMER_MATCH_0 = 0,
  TIMER_MATCH_1 = 1,
  TIMER_MATCH_2 = 2,
  TIMER_MATCH_3 = 3
} timer_match_t;

typedef enum {
  TIMER_MATCH_NOTHING = 0,
  TIMER_MATCH_CLEAR   = 1,
  TIMER_MATCH_SET     = 2,
  TIMER_MATCH_TOGGLE  = 3
} timer_match_action_t;

void timerInit(timer_t t, uint32_t prescale);
void timerEnable(timer_t t);
void timerDisable(timer_t t);
void timerSetMatch(timer_t       t, 
                   timer_match_t m,
                   uint32_t      match_value,
                   bool          interrupt,
                   bool          reset,
                   bool          stop);
void timerSetExternalMatch(timer_t              t, 
                           timer_match_t        m,
                           timer_match_action_t action);

void timerClearMatchInterrupt(timer_t t, timer_match_t m);

#endif


