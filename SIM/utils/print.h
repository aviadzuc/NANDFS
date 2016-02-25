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
 
#ifndef PRINT_H
#define PRINT_H
 
#include <system.h>

typedef void (*printer)(void* env,uint8_t c); 

void print(printer p, void* env, uint8_t* s);
void printHex(printer p, void* env, uint32_t v, uint8_t digits);
void printNum(printer p, void* env, int32_t v);

#endif
