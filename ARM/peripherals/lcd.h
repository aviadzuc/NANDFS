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

#ifndef LCD_H
#define LCD_H

void lcdInit();											// Initial LCD
//void lcdGotoCursor(unsigned char);						// Set Position Cursor LCD
void lcdSendByte(void* env, uint8_t c);						// Print Display to LCD
void lcdSendCommand(unsigned char); 					// Write Instruction

#define  lcdClear()         lcdSendCommand(0x01)	// Clear Display
#define  lcdCursorHome()    lcdSendCommand(0x02)	// Set Cursor = 0
#define  lcdOn()     		lcdSendCommand(0x0E)	// LCD Display Enable
#define  lcdOff()    		lcdSendCommand(0x08)	// LCD Display Disable
#define  lcdCursorBlink()   lcdSendCommand(0x0F)	// Set Cursor = Blink
#define  lcdCursorOn()      lcdSendCommand(0x0E)	// Enable LCD Cursor
#define  lcdCursorOff()     lcdSendCommand(0x0C)	// Disable LCD Cursor
#define  lcdCursorLeft()    lcdSendCommand(0x10)	// Shift Left Cursor
#define  lcdCursorRight()   lcdSendCommand(0x14)	// Shift Right Cursor
#define  lcdShiftLeft()  	lcdSendCommand(0x18)	// Shift Left Display
#define  lcdShitRight() 	lcdSendCommand(0x1C)	// Shift Right Display
#define  lcdGotoCursor(x)	lcdSendCommand(0x80 | x)
//void lcdInit();											// Initial LCD
//void lcdGotoCursor(unsigned char);						// Set Position Cursor LCD
//void lcd_print(unsigned char*);							// Print Display to LCD
//void lcdSendByte(void* env, uint8_t c);						// Print Display to LCD

//void lcd_out_data4(unsigned char);						// Strobe 4-Bit Data to LCD
//void lcd_write_byte(unsigned char);						// Write 1 Byte Data to LCD
//void lcdSendCommand(unsigned char); 					// Write Instruction
//void lcd_write_ascii(unsigned char); 					// Write LCD Display(ASCII)
//void delay(unsigned long int);							// Delay Function
														
#endif
