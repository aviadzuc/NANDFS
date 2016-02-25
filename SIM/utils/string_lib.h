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

#ifndef STRING_LIB_H_
#define STRING_LIB_H_

#include <system.h>

/** 
 * compare strings s1,s2
 * 
 * @param s1 first string
 * @param s2 second string
 * @return 0 if identical, difference otherwise
 * */ 
//uint8_t fsStrcmp(uint8_t *s1, uint8_t *s2);
int32_t fsStrcmp(uint8_t *s1, uint8_t *s2);

int32_t fsStrncmp(uint8_t *s1, uint8_t *s2, uint32_t n);

/**
 * copy s2 to s1
 * 
 * @param s1
 * @param s2
 * @erturn s1
 */
uint8_t *fsStrcpy(uint8_t *s1, uint8_t *s2);
#endif /*STRING_LIB_H_*/
