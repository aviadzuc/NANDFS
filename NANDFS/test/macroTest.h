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

/** @file macroTest.h
 *  Header file for various macros used in tests and debugging
 */
#include <peripherals/nand.h>
#include <src/sequencing/sequencing.h>
#include <test/testUtils.h>
#include <lpc2000/uart.h>
#include <utils/print.h>

/* run the SEQ test of FUNC_NAME */
#define PRINT_MSG_AND_NUM(MSG,NUM){ print(uart0SendByte,0,MSG);\
								   printNum(uart0SendByte,0,(NUM));}
#define PRINT_MSG_AND_HEX(MSG,NUM) {print(uart0SendByte,0,MSG);\
								   printHex(uart0SendByte,0,(NUM), 8);}
#define PRINT_MSG_AND_STR(MSG,STR) {print(uart0SendByte,0,MSG);\
								   print(uart0SendByte,0,STR);}
#define PRINT(MSG)                 print(uart0SendByte,0,MSG);
#define ADDR_PRINT(LOG_ADDR)       *((uint32_t*)(LOG_ADDR))
#define RUN_TEST(FUNC_NAME, SEQ){ \
	init_ ## FUNC_NAME ## Test() ; \
	if(!FUNC_NAME ## Test ## SEQ())\
		print(uart0SendByte,0,"\nfailure - ");  \
	else\
		print(uart0SendByte,0,"\nsuccess - "); \
	print(uart0SendByte,0,#FUNC_NAME "Test" #SEQ); \
	tearDown_ ## FUNC_NAME ## Test() ;}

#ifdef SIM
	#include <stdio.h>
	#include <stdlib.h>
	#define  FENT()     printf("\nTRACE %s:%d. %s() - starting ", __FILE__, __LINE__, __func__);fflush(stdout)
	#define L(fmt, ...) printf("\nTRACE %s:%d. %s() - ", __FILE__, __LINE__, __func__);printf(fmt, ## __VA_ARGS__);fflush(stdout)
//	#define  L(f, s...) printf("\nTRACE %s:%d. %s() - ", __FILE__, __LINE__, __func__); printf(f, s);fflush(stdout)
	#define  FAILURE()  printf("\nTRACE %s:%d. %s() - verification failure ", __FILE__, __LINE__, __func__);fflush(stdout)
#else
	#define  FENT()
	#define  L(f, s...)
	#define  FAILURE()
#endif

//#define printf()
//#define PRINT_MSG_AND_NUM(MSG,NUM)
//#define PRINT_MSG_AND_HEX(MSG,NUM)
//#define PRINT_MSG_AND_STR(MSG,STR)
//#define PRINT(MSG)
//#define ADDR_PRINT(LOG_ADDR)       *((uint32_t*)(LOG_ADDR))
//
//
//#define  FENT()
//#define  L(f, s...)
//#define  FAILURE()


#define NOT_RESERVE_TEST 0
#define RESERVE_TEST 	 1
#define VERIFY_NOT(TEST) if (!(TEST)) return 0;
#define VERIFY(TEST)     if (!(TEST)){ FAILURE(); return 0;}
