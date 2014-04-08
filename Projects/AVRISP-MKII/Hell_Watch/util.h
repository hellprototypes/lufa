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

#ifndef UTIL_H_
#define UTIL_H_
#include <stdbool.h>

#define noinline __attribute__ ((__noinline__))

#define delay(ms) _delay_ms(ms)
#define delay_us(us) _delay_us(us)

// Quick and easy macro for a for loop
#define LOOP(count, var) for(uint8_t var=0;var<count;var++)

// Reverse loop sometimes produces smaller/faster code
#define LOOPR(count, var) for(uint8_t var=count;var--;)

//============================================================
#define PORT_CTRL_OUT		 PORTA.OUT
#define PORT_CTRL_IN		 PORTA.IN
#define PIN_KEY_MAIN        7
#define PIN_PWR_LOCK        6
#define PIN_RTC_ACC_INT     5
#define PIN_VIBRATOR        4
#define PIN_ROM_CS          3
#define PIN_ADC_EN          2
#define PIN_KEY_ADC         1
#define PIN_BAT_ADC         0

//PORTB
#define PIN_GPIO_0          0
#define PIN_GPIO_1          1
#define PIN_GPIO_2          2
#define PIN_GPIO_3          3

//PORTE
#define GPIO_H_OUT		 	PORTE.OUT
#define PIN_GPIO_4          0
#define PIN_GPIO_5          1
#define PIN_GPIO_6          2
#define PIN_GPIO_7          3

//PORTC
#define PIN_I2C_SCL         1
#define PIN_I2C_SDA         0

#define PORT_NRF_OUT		 PORTC.OUT
#define PIN_NRF_IRQ         2
#define PIN_NRF_CE          3
#define PIN_NRF_CSN         4

#define PORT_SPI_OUT		 PORTC.OUT
#define PORT_SPI_IN		 PORTC.IN
#define PIN_SPI_MOSI        5
#define PIN_SPI_MISO        6
#define PIN_SPI_SCK         7

//PORTD
#define PORT_OLED_OUT		 PORTD.OUT
#define PIN_OLED_CS         0
#define PIN_OLED_SCLK       1
#define PIN_OLED_DC         2
#define PIN_OLED_SDATA      3
#define PIN_OLED_RST        4

#define PORT_BEEP_OUT		PORTD.OUT
#define PORT_BEEP_IN		PORTD.IN
#define PIN_BEEPER          5


/** **********************************************************/
#define setbit(port, bit) ((port) |= (uint8_t)_BV(bit))
#define setbits(port, mask) ((port) |= (uint8_t)(mask))
#define clrbit(port, bit) ((port) &= (uint8_t)~_BV(bit))
#define clrbits(port, mask) ((port) &= (uint8_t)~(mask))
#define testbit(port, bit) (uint8_t)(((uint8_t)port & (uint8_t)_BV(bit)))
#define togglebit(port, bit) (port ^= (uint8_t)_BV(bit))
#define	hibyte(x) (uint8_t)(x>>8)
#define	lobyte(x) (uint8_t)(x&0x00FF)

/** **********************************************************/
#define pwr_lock_free() 	clrbit(PORT_CTRL_OUT, PIN_PWR_LOCK)
#define vbat_m_pwr_off() 	clrbit(PORT_CTRL_OUT, PIN_ADC_EN)
#define vbat_m_pwr_on() 	setbit(PORT_CTRL_OUT, PIN_ADC_EN)
/** **********************************************************/

#endif /* UTIL_H_ */