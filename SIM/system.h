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


#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdlib.h> /* for int32_t */
#include <assert.h>

#if 	defined(SYSTEM_TYPES)
	#include <nandfs_types.h>
#endif 

#define CAST_TO_UINT32(PTR)   ((uint32_t*)(PTR))
#define CAST_TO_INT32(PTR)    ((int32_t*)(PTR))
#define CAST_VAL_TO_UINT32(VAL)   ((uint32_t)(VAL))
#define CAST_VAL_TO_INT32(VAL)    ((int32_t)(VAL))
#define GET_BYTE0(A) ((uint8_t)(A))
#define GET_BYTE1(A) ((uint8_t)((A) >> 8))
#define GET_BYTE2(A) ((uint8_t)((A) >> 16))
#define GET_BYTE3(A) ((uint8_t)((A) >> 24))

#define GET_BIT(PTR, BIT_NUM, OFFSET)      (((PTR)->bytes[((BIT_NUM) >> 3) + OFFSET] >> ((BIT_NUM) & 7)) & 1)
#define SET_BIT(PTR, BIT_NUM, OFFSET, VAL) if(VAL) {(PTR)->bytes[((BIT_NUM) >> 3)+(OFFSET)] |= (1 << ((BIT_NUM) & 7));}\
 										   else    {(PTR)->bytes[((BIT_NUM) >> 3)+(OFFSET)] &= (~(1 << ((BIT_NUM) & 7)));}

#define SET_BYTE(PTR, BYTE_NUM,VAL)   (PTR)->bytes[BYTE_NUM] = VAL
#define GET_BYTE(PTR, BYTE_NUM)       ((uint32_t)(PTR->bytes[BYTE_NUM]))
#define GET_BYTE_PTR(PTR, BYTE_NUM)   (&(PTR->bytes[BYTE_NUM]))

#define GET_UINT32_PTR(PTR, BYTE_NUM)  (CAST_TO_UINT32(&((PTR)->bytes[BYTE_NUM])))
#define GET_INT32_PTR(PTR, BYTE_NUM)   (CAST_TO_INT32(&((PTR)->bytes[BYTE_NUM])))
#define GET_UINT32(PTR, BYTE_NUM)      (GET_BYTE((PTR),BYTE_NUM) | (GET_BYTE((PTR),BYTE_NUM+1) << 8) |  (GET_BYTE((PTR),BYTE_NUM+2) << 16) |  (GET_BYTE((PTR),BYTE_NUM+3) << 24))
#define GET_INT32(PTR, BYTE_NUM)       CAST_VAL_TO_INT32(GET_UINT32((PTR), BYTE_NUM))
#define SET_UINT32(PTR, BYTE_NUM, VAL) {SET_BYTE((PTR), BYTE_NUM,  GET_BYTE0(VAL));\
										SET_BYTE((PTR), (BYTE_NUM)+1,  GET_BYTE1(VAL));\
										SET_BYTE((PTR), (BYTE_NUM)+2,  GET_BYTE2(VAL));\
										SET_BYTE((PTR), (BYTE_NUM)+3,  GET_BYTE3(VAL));}
#define SET_INT32(PTR, BYTE_NUM, VAL)   SET_UINT32((PTR), BYTE_NUM, CAST_VAL_TO_UINT32(VAL))
//typedef int int32_t; 

#define SUCCESS			0
#define ERROR_GENERIC	-1
#define ERROR_BUSY		-2

typedef uint32_t bitfield_t;
typedef uint32_t user_id;
typedef uint32_t processid_t;

#ifndef ECOS_OS
typedef int32_t nandfs_mutex_t;
#else
#endif

#define CAST_TO_MUTEX(PTR) ((nandfs_mutex_t*)(PTR))     

typedef int error_t;
typedef void (*task_t )(error_t e, void* state); 
typedef void (*event_t)(error_t e, void* state);
// define callback function writeCheckpoint
typedef error_t (*checkpoint_writer)(bool_t isPartOfHeader);
  
typedef void (*interrupt_handler_t)(void);

#if defined(OLIMEX_LPC_H2214)
#include <platforms/olimex-lpc-h2214.h>
#endif

#if defined(ETT_ARM_STAMP)
#include <platforms/ett-arm-stamp.h>
#endif

#if    defined(LPC2119) \
    || defined(LPC2148)
#include <lpc2000/lpc21xx.h>
#endif

#if    defined(LPC2214)
#include <lpc2000/lpc22xx.h>
#endif

#endif
