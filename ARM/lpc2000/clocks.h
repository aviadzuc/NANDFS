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
 * clocks.h
 * 
 * Sivan Toledo.
 */
 
#ifndef CLOCKS_H
#define CLOCKS_H
 
#include <system.h>

#define clocksInitDefault() clocksInit(1,4)
void clocksInit(uint32_t min_cclk, uint32_t vpb_divider);
uint32_t clocksGetCclkFreq(void);
uint32_t clocksGetPclkFreq(void);

#endif
