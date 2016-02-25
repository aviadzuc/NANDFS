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
 * gpio.c
 * 
 * Sivan Toledo.
 */
 
#include <lpc2000/gpio.h>


#if defined(LPC2119) || defined(LPC2129) || defined(LPC2292) || defined(LPC2214)
/*
 * We normally don't need to enable GPIO's: most (all?)
 * are enabled at reset.
 */
void gpioEnable(uint8_t port, uint8_t gpio, gpio_dir_t direction) {
  uint32_t mask;
  switch (port) {

  	case 0:

  	if (direction == GPIO_INPUT)
  	  IODIR0 &= ~(0x00000001 << gpio);
  	else
  	  IODIR0 |=  (0x00000001 << gpio);  	

  	mask = 0x00000003;
  	if (gpio <= 15) {
	  mask <<= gpio;
  	  mask <<= gpio;
  	  PINSEL0 &= ~mask;        
  	} else {
  	  if (gpio == 26 || gpio == 31)
  	  	error();
  	  gpio -= 16;
	  mask <<= gpio;
  	  mask <<= gpio;
  	  PINSEL1 &= ~mask;
  	}
  	break;

  	case 1:
  	if (gpio < 16) 
  	  error();
  	if (gpio >= 26 && gpio <= 31)
  	  PINSEL2 &= ~(0x00000001 << 2);
  	if (gpio >= 16 && gpio <= 25)
  	  PINSEL2 &= ~(0x00000001 << 3);
  	if (direction == GPIO_INPUT)
  	  IODIR1 &= ~(0x00000001 << gpio);
  	else
  	  IODIR1 |=  (0x00000001 << gpio);  	

  	default:
  	error();
  }
}
#endif

bool gpioRead(uint8_t port, uint8_t gpio) {
  switch (port) {
  	case 0:
  	  return (IOPIN0 >> gpio) & 1;
  	case 1:
  	  return (IOPIN1 >> gpio) & 1;
  }
}

void gpioSet(uint8_t port, uint8_t gpio) {
  switch (port) {
  	case 0:
  	  IOSET0 |= (1 << gpio);
  	case 1:
  	  IOSET1 |= (1 << gpio);
  }
}

void gpioClear(uint8_t port, uint8_t gpio) {
  switch (port) {
  	case 0:
  	  IOCLR0 |= (1 << gpio);
  	case 1:
  	  IOCLR1 |= (1 << gpio);
  }
}


