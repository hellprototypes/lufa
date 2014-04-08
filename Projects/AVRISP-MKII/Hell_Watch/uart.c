// UART driver

#include <avr/io.h>
#include <stdio.h>
#include "util.h"
#include "uart.h"

static int put(char, FILE*);

FILE uart_io = FDEV_SETUP_STREAM(put, NULL, _FDEV_SETUP_WRITE);

void uart_init()
{

#if DEBUG_PIN_EN
	PORTE.DIR		|= 0x0C;
	PORTE.OUT		|= 0x0C;
	PORTE.PIN2CTRL	= 0x18; 
	PORTE.PIN3CTRL	= 0x18; 
#else
	PORTE.DIR		|= 0x08;
	PORTE.OUT		|= 0x08;
	PORTE.PIN3CTRL	= 0x18;//PIN_GPIO_7
#endif

	// Initialize USARTE0 for External Interface Port
	USARTE0.BAUDCTRLA = 0x40;	// BSCALE = -6, BSEL = 1047
	USARTE0.BAUDCTRLB = 0xA4;	// ==> 57600 bps
	USARTE0.CTRLC	  = 0x03;	// Async, No Parity, 1 stop bit, 8 data bits
	USARTE0.CTRLB	  = 0x08;	// Enable TX

	stdout = &uart_io;
}

static int put(char c, FILE* stream)
{
	(void)(stream); // Get rid of unused variable warning
	while ( !testbit(USARTE0.STATUS,USART_DREIF_bp) ) ;
    USARTE0.DATA = c;
	return 0;
}
