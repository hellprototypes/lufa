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

#define CPU_SPEED		F_CPU
#define BAUDRATE		100000
#define TWI_BAUD(F_SYS, F_TWI)	((F_SYS / (2 * F_TWI)) - 5)
#define TWI_BAUDSETTING			TWI_BAUD(CPU_SPEED, BAUDRATE)


void i2c_init()
{
	TWIC.MASTER.BAUD = TWI_BAUDSETTING;
	TWIC.MASTER.CTRLA = TWI_MASTER_ENABLE_bm;
	//TWIC.MASTER.CTRLB = TWI_MASTER_SMEN_bm;
	TWIC.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
}

bool i2c_start(uint8_t addr)
{
	TWIC.MASTER.ADDR = addr;
	while(!(TWIC.MASTER.STATUS & (TWI_MASTER_WIF_bm | TWI_MASTER_RIF_bm)));

	if(TWIC.MASTER.STATUS & TWI_MASTER_RXACK_bm) {
		return false;
	}
	return true;
}

void i2c_stop()
{
	TWIC.MASTER.CTRLC = TWIC.MASTER.CTRLC | TWI_MASTER_CMD_STOP_gc;//Stop
}

bool i2c_write(uint8_t _data)
{
	TWIC.MASTER.DATA = _data;
	while(!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm));

	if(TWIC.MASTER.STATUS & TWI_MASTER_RXACK_bm) {
		return false;
	}
	return true;
}

bool i2c_read(uint8_t* data, bool ack)
{
	//uint16_t start;

	//start = millis_get();
	while(!(TWIC.MASTER.STATUS & TWI_MASTER_RIF_bm)) {
		//if((millis_get() - start) > 20) {//20ms timeout
		//	return false;
		//}
	}
	*data = TWIC.MASTER.DATA;
	
	if(ack) {
		TWIC.MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
	} else {
		TWIC.MASTER.CTRLC = TWI_MASTER_ACKACT_bm;
	}
	
	return true;
}
