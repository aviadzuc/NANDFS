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
#include <stdio.h>

void print(printer p, void* env, uint8_t* s) {
  printf("%s",s);
  fflush(stdout);
}

void printHex(printer p, void* env, uint32_t v, uint8_t digits) {
  printf("%x", v);
  fflush(stdout);
}

void printNum(printer p, void* env, int32_t v) {
  printf("%d", v);
  fflush(stdout);
}

