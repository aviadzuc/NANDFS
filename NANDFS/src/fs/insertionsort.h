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

/** @file insertionsort.h
 * Insertion sort for pointer to pointer arrays. Used in scansir()
 */
 
#ifndef INSERTIONSORT_H_
#define INSERTIONSORT_H_

#include <system.h>

void insertion_sort(void **base, int32_t num_elements, int32_t element_size,
                    int32_t (*comparer)(void **, void **));

#endif /*INSERTIONSORT_H_*/
