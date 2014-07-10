//*********************************************************************
// Copyright (C) 2014 Hell Prototypes / www.hellprototypes.com
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
//********************************************************************
#include <avr/io.h>
#include "i2c.h"


bool i2c_dev_cmd(uint8_t addr, uint8_t reg, uint8_t cmd);
bool i2c_dev_read(uint8_t addr, uint8_t start_reg, uint8_t *buffer, uint8_t len);

bool i2c_dev_cmd(uint8_t addr, uint8_t reg, uint8_t cmd)
{
	if(!i2c_start(addr)) {
		return false;
	}

	if(!i2c_write(reg)) {
		return false;
	}
	if(!i2c_write(cmd)) {
		return false;
	}

	i2c_stop();

	return true;

}

bool i2c_dev_write(uint8_t addr, uint8_t start_reg, uint8_t *buffer, uint8_t len)
{
	uint8_t i=0;

	if(!i2c_start(addr)) {
		return false;
	}

	if(!i2c_write(start_reg)) {
		return false;
	}

	for(i=0; i<len; i++) {
		if(!i2c_write(buffer[i])) {
			i2c_stop();
			return false;
		}
	}

	i2c_stop();

	return true;
}


bool i2c_dev_read(uint8_t addr, uint8_t start_reg, uint8_t *buffer, uint8_t len)
{
	uint8_t i=0;

	if(!i2c_start(addr)) {
		return false;
	}

	if(!i2c_write(start_reg)) {
		return false;
	}

	if(!i2c_start(addr | 0x01)) {
		return false;
	}

	for(i=0; i<len-1; i++) {
		if(!i2c_read(&buffer[i], I2C_ACK)) {
			i2c_stop();
			return false;
		}
	}

	if(!i2c_read(&buffer[i], I2C_NACK)) {
		return false;
	}

	i2c_stop();

	return true;
}
