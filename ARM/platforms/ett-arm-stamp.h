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
 * ett-arm-stamp.h
 * 
 * Sivan Toledo.
 */

#ifndef ETT_ARM_STAMP_H
#define ETT_ARM_STAMP_H
 
#define F_OSC 19660800

#ifndef LPC2119
#error You must define LPC2119 to use this platform
#endif

// a macro to indicate an error; it can turn a led on, etc
#define error()

#endif

