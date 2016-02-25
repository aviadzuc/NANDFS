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

// LCD Routines for "ET-ARM7 KIT V1.0" 
// Character 16x2 4-Bit Mode Interface 
// D4 = P1.16						   
// D5 = P1.17						   
// D6 = P1.18						   
// D7 = P1.19						   
// RS = P1.20						   
// EN = P1.21						   

#include <system.h>
#include <peripherals/nand.h>
#include <lpc2000/busywait.h>

#include <peripherals/lcd.h>

// Define LCD PinIO Mask 
#define  LCD_D4     0x00010000   						// P1.16
#define  LCD_D5     0x00020000   						// P1.17
#define  LCD_D6     0x00040000   						// P1.18
#define  LCD_D7     0x00080000   						// P1.19
#define  LCD_EN     0x00100000   						// P1.20
#define  LCD_RS     0x00200000   						// P1.21
#define  LCD_DATA   (LCD_D4|LCD_D5|LCD_D6|LCD_D7)
#define  LCD_IOALL  (LCD_D4|LCD_D5|LCD_D6|LCD_D7|LCD_EN|LCD_RS)

#define  lcd_rs_set() IOSET1 |= LCD_RS	 				// RS = 1 (Select Instruction)
#define  lcd_rs_clr() IOCLR1 |= LCD_RS					// RS = 0 (Select Data)
#define  lcd_en_set() IOSET1 |= LCD_EN					// EN = 1 (Enable)
#define  lcd_en_clr() IOCLR1 |= LCD_EN					// EN = 0 (Disable)


/****************************/
/* Write Data 1 Byte to LCD */
/****************************/
static void lcd_write_byte(unsigned char val)
{  
  IOCLR1 |= (LCD_DATA);	  								// Reset 4-Bit Pin Data
  IOSET1 |= ((val>>4)&0x0F) << 16;						// 0000 0000 00,EN,RS DDDD 0000 0000 0000 0000
  //lcd_out_data4((val>>4)&0x0F);						// Strobe 4-Bit High-Nibble to LCD
  lcd_en_set();											// EN = 1 = Strobe Signal  
  lcd_en_clr();											// EN = 0
  busywait(500);										// Wait LCD Execute Complete

  IOCLR1 |= (LCD_DATA);	  								// Reset 4-Bit Pin Data
  IOSET1 |= (val&0x0F) << 16; 							// 0000 0000 00,EN,RS DDDD 0000 0000 0000 0000
  //lcd_out_data4((val)&0x0F);							// Strobe 4-Bit Low-Nibble to LCD
  lcd_en_set();											// EN = 1 = Strobe Signal   
  lcd_en_clr();											// EN = 0   
  busywait(500);										// Wait LCD Execute Complete
}

/****************************/
/* Write Instruction to LCD */
/****************************/
void lcdSendCommand(unsigned char val)
{
  lcd_rs_clr();											// RS = 0 = Instruction Select
  lcd_write_byte(val);									// Strobe Command Byte	  
  busywait(2048);					  					// Approx. 2mS Delay
}


/*******************************/
/* Initial 4-Bit LCD Interface */
/*******************************/
void lcdInit()
{
  IODIR1 |= 0x003F0000;									// P1.16 - P1.21 = Output	
	
  lcd_rs_clr();											// RS = 0 = Instruction Select 
  lcd_en_clr();											// EN = 0  
  busywait(2000);					  						// Approx. 2mS Delay
  
  lcdSendCommand(0x33); 								// Initial (Set DL=1 3 Time, Reset DL=0 1 Time)
  lcdSendCommand(0x32); 
  lcdSendCommand(0x28);  							// Function Set (DL=0 4-Bit,N=1 2 Line,F=0 5X7)
  lcdSendCommand(0x0C);  							// Display on/off Control (Entry Display,Cursor off,Cursor not Blink)
  lcdSendCommand(0x06);  							// Entry Mode Set (I/D=1 Increment,S=0 Cursor Shift)
  lcdSendCommand(0x01);  							// Clear Display  (Clear Display,Set DD RAM Address=0) 
}

/***************************/
/* Set LCD Position Cursor */
/***************************/
void xxxlcdGotoCursor(unsigned char i)
{
  //i |= 0x80;											// Set DD-RAM Address Command
  lcdSendCommand(i | 0x80);  
}

/************************************/
/* Print Display Data(ASCII) to LCD */
/************************************/
void lcdSendByte(void* env, uint8_t c) {
  lcd_rs_set();											// RS = 1 = Data Select
  lcd_write_byte(c);		   							// Strobe 1 Byte to LCD    
  //lcd_write_ascii(c);
}

/*
void lcd_print(unsigned char* str)
{
  int i;

  for (i=0;i<16 && str[i]!=0;i++)  						// 16 Character Print
  {
    lcd_write_ascii(str[i]);							// Print Byte to LCD
  }
}
*/




