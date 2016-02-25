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
 * i2c 
 * 
 * Sivan Toledo
 */

#ifndef I2C_H
#define I2C_H

#include <system.h>

/*
 * if the lsb of the slave address is set (it is not part of the
 * address since the lsb of the address from the master is used 
 * for the r/w flag) then the MCU will respond to the general
 * call address; if it is 0, it will ignore general calls.
 */
 
void i2cInit(uint32_t bitrate, uint8_t slave_address);

int32_t i2cMasterTransact(uint8_t  slave, 
                          uint8_t* command, 
                          int32_t  command_len,
                          uint8_t* response,
                          int32_t  response_len);

#endif
