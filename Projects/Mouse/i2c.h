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
#ifndef I2C_H_
#define I2C_H_

#include <stdbool.h>

#define I2C_NACK	false
#define I2C_ACK		true

void i2c_init(void);
void i2c_restart(void);
bool i2c_start(uint8_t addr);
void i2c_stop(void);
bool i2c_write(uint8_t _data);
bool i2c_read(uint8_t*, bool);

#endif /* I2C_H_ */