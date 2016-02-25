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

#include <utils/memlib.h>

// source - http://www.vik.cc/daniel/portfolio/memcpy.htm
void *fsMemcpy(void* dest, void* src, uint32_t count){	    
	uint8_t *dst8 = (uint8_t *)dest;
	uint8_t *src8 = (uint8_t *)src;
	
	if (count & 1) {
	    dst8[0] = src8[0];
	    dst8 += 1;
	    src8 += 1;
	}
	
	count = count >> 1;
	while (count--) {
	    dst8[0] = src8[0];
	    dst8[1] = src8[1];
	    
	    dst8 += 2;
	    src8 += 2;
	}
	return dest;    
}

void *fsMemset(void *s, uint8_t c, uint32_t n)
{
    uint8_t *us = s;
    uint8_t uc = c;
    while (n-- != 0)
        *us++ = uc;
        
    return s;
}
