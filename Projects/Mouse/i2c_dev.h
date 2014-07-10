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
#ifndef I2C_DEV_H_
#define I2C_DEV_H_

bool i2c_dev_cmd(uint8_t addr, uint8_t reg, uint8_t cmd);
bool i2c_dev_write(uint8_t addr, uint8_t start_reg, uint8_t *buffer, uint8_t len);
bool i2c_dev_read(uint8_t addr, uint8_t start_reg, uint8_t *buffer, uint8_t len);

#endif /* I2C_H_ */