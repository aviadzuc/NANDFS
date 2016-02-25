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
 * print.c
 * 
 * Sivan Toledo.
 */
 
#include <utils/print.h>

static uint8_t buffer[8];

void print(printer p, void* env, uint8_t* s) {
  while (*s) {
    (*p)(env,*s);
    s++;
  }
}

void printHex(printer p, void* env, uint32_t v, uint8_t digits) {
  int8_t   i;
  uint8_t  d;
  
  for (i=0; i<digits; i++) {
  	d = v & 0x0000000F;
  	buffer[i] = d < 10 ? d + '0' : d - 10 + 'A';
  	v >>= 4;
  }
  
  for (i=digits-1; i>=0; i--) {
    (*p)(env,buffer[i]);
  }
}

void printNum(printer p, void* env, int32_t v) {
  int8_t   i;
  uint8_t  d;
  uint8_t  digits;
  bool_t   n;

  if (v==0) {
    (*p)(env,'0');
    return;
  }

  if (v < 0) {
  	n = true;
  	v = -v;
  } else
    n = false; 
  
  digits = 0;
  for (i=0; v != 0; i++) {
  	buffer[i] = (v % 10) + '0';
  	v /= 10;
  	digits++;
  }
  
  if (n) (*p)(env,'-');
  for (i=digits-1; i>=0; i--) {
    (*p)(env,buffer[i]);
  }
}

