/*
             LUFA Library
     Copyright (C) Dean Camera, 2014.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Target-related functions for the PDI Protocol decoder.
 */

#define  INCLUDE_FROM_XPROGTARGET_C
#include "XPROGTarget.h"

#if defined(ENABLE_XPROG_PROTOCOL) || defined(__DOXYGEN__)

/** Flag to indicate if the USART is currently in Tx or Rx mode. */
bool IsSending;

/** Enables the target's PDI interface, holding the target in reset until PDI mode is exited. */
void XPROGTarget_EnableTargetPDI(void)
{
	IsSending = false;

#ifdef HELL_WATCH_PORT
	/* Set Tx and XCK as outputs, Rx as input */
	PORTE.DIRSET =	(1 << 1) | (1 << 3);//1: XCK, 3:TXD
	PORTE.DIRCLR =	(1 << 2);
	PORTE.PIN1CTRL |= PORT_INVEN_bm;//Data changed at the falling XCK clock edge and sampled at the rising XCK clock edge

	/* Set DATA line high for at least 90ns to disable /RESET functionality */
	PORTE.OUTSET = (1 << 3);
	_delay_us(5);

	//uint16_t BaudValue = ((((F_CPU / 16) + (XPROG_HARDWARE_SPEED / 2)) / (XPROG_HARDWARE_SPEED)) - 1)
	USARTE0.BAUDCTRLB = 0x00;
	USARTE0.BAUDCTRLA = 0x00;

	/* Set up the synchronous USART for XMEGA communications - 8 data bits, even parity, 2 stop bits */
	USARTE0.CTRLC	  = USART_CMODE0_bm | USART_PMODE1_bm | USART_SBMODE_bm | USART_CHSIZE0_bm | USART_CHSIZE1_bm;	
	USARTE0.CTRLB	  = USART_TXEN_bm;	// Enable TX
#else
	/* Set Tx and XCK as outputs, Rx as input */
	DDRD |=  (1 << 5) | (1 << 3);
	DDRD &= ~(1 << 2);

	/* Set DATA line high for at least 90ns to disable /RESET functionality */
	PORTD |= (1 << 3);
	_delay_us(100);

	/* Set up the synchronous USART for XMEGA communications - 8 data bits, even parity, 2 stop bits */
	UBRR1  = ((F_CPU / 2 / XPROG_HARDWARE_SPEED) - 1);
	UCSR1B = (1 << TXEN1);
	UCSR1C = (1 << UMSEL10) | (1 << UPM11) | (1 << USBS1) | (1 << UCSZ11) | (1 << UCSZ10) | (1 << UCPOL1);
#endif
	/* Send two IDLEs of 12 bits each to enable PDI interface (need at least 16 idle bits) */
	XPROGTarget_SendIdle();
	XPROGTarget_SendIdle();
}

/** Enables the target's TPI interface, holding the target in reset until TPI mode is exited. */
void XPROGTarget_EnableTargetTPI(void)
{
	IsSending = false;

#ifdef HELL_WATCH_PORT
	PORTB.DIRSET = 0x01;//Use PB0 for reset
	PORTB.OUTCLR =  0x01;
	_delay_us(100);

	/* Set Tx and XCK as outputs, Rx as input */
	PORTE.DIRSET =	(1 << 1) | (1 << 3);
	PORTE.DIRCLR =	(1 << 2);
	PORTE.PIN1CTRL |= PORT_INVEN_bm;//Data changed at the falling XCK clock edge and sampled at the rising XCK clock edge

	/* Set up the synchronous USART for TPI communications - 8 data bits, even parity, 2 stop bits */
	//uint16_t BaudValue = ((((F_CPU / 16) + (XPROG_HARDWARE_SPEED / 2)) / (XPROG_HARDWARE_SPEED)) - 1)
	USARTE0.BAUDCTRLB = 0x00;
	USARTE0.BAUDCTRLA = 0x00;
	USARTE0.CTRLC	  = USART_CMODE0_bm | USART_PMODE1_bm | USART_SBMODE_bm | USART_CHSIZE0_bm | USART_CHSIZE1_bm;	
	USARTE0.CTRLB	  =  USART_TXEN_bm;	// Enable TX
#else
	/* Set /RESET line low for at least 400ns to enable TPI functionality */
	AUX_LINE_DDR  |=  AUX_LINE_MASK;
	AUX_LINE_PORT &= ~AUX_LINE_MASK;
	_delay_us(100);

	/* Set Tx and XCK as outputs, Rx as input */
	DDRD |=  (1 << 5) | (1 << 3);
	DDRD &= ~(1 << 2);

	/* Set up the synchronous USART for TPI communications - 8 data bits, even parity, 2 stop bits */
	UBRR1  = ((F_CPU / 2 / XPROG_HARDWARE_SPEED) - 1);
	UCSR1B = (1 << TXEN1);
	UCSR1C = (1 << UMSEL10) | (1 << UPM11) | (1 << USBS1) | (1 << UCSZ11) | (1 << UCSZ10) | (1 << UCPOL1);
#endif
	/* Send two IDLEs of 12 bits each to enable TPI interface (need at least 16 idle bits) */
	XPROGTarget_SendIdle();
	XPROGTarget_SendIdle();
}

/** Disables the target's PDI interface, exits programming mode and starts the target's application. */
void XPROGTarget_DisableTargetPDI(void)
{
	/* Switch to Rx mode to ensure that all pending transmissions are complete */
	if (IsSending)
	  XPROGTarget_SetRxMode();
#ifdef HELL_WATCH_PORT
	/* Turn off receiver and transmitter of the USART, clear settings */
	USARTE0.CTRLB = 0;
	USARTE0.CTRLC = 0;

	PORTE.DIRCLR =	(1 << 1) | (1 << 2) | (1 << 3);
	PORTE.OUTCLR =	(1 << 1) | (1 << 2) | (1 << 3);
	PORTE.PIN1CTRL &= ~PORT_INVEN_bm;
#else
	/* Turn off receiver and transmitter of the USART, clear settings */
	UCSR1A  = ((1 << TXC1) | (1 << RXC1));
	UCSR1B  = 0;
	UCSR1C  = 0;

	/* Tristate all pins */
	DDRD  &= ~((1 << 5) | (1 << 3));
	PORTD &= ~((1 << 5) | (1 << 3) | (1 << 2));
#endif
}

/** Disables the target's TPI interface, exits programming mode and starts the target's application. */
void XPROGTarget_DisableTargetTPI(void)
{
	/* Switch to Rx mode to ensure that all pending transmissions are complete */
	if (IsSending)
	  XPROGTarget_SetRxMode();

#ifdef HELL_WATCH_PORT
	/* Turn off receiver and transmitter of the USART, clear settings */
	USARTE0.CTRLB = 0;
	USARTE0.CTRLC = 0;

	/* Set all USART lines as inputs, tristate */
	PORTE.DIRCLR =	(1 << 1) | (1 << 2) | (1 << 3);
	PORTE.OUTCLR =	(1 << 1) | (1 << 2) | (1 << 3);
	PORTE.PIN1CTRL &= ~PORT_INVEN_bm;

	/* Tristate target /RESET line */
	PORTB.DIRCLR =  0x01;//Use PB0 for reset
	PORTB.OUTCLR =	0x01;
#else
	
	UCSR1A |= (1 << TXC1) | (1 << RXC1);
	UCSR1B  = 0;
	UCSR1C  = 0;

	/* Set all USART lines as inputs, tristate */
	DDRD  &= ~((1 << 5) | (1 << 3));
	PORTD &= ~((1 << 5) | (1 << 3) | (1 << 2));

	/* Tristate target /RESET line */
	AUX_LINE_DDR  &= ~AUX_LINE_MASK;
	AUX_LINE_PORT &= ~AUX_LINE_MASK;
#endif
}

/** Sends a byte via the USART.
 *
 *  \param[in] Byte  Byte to send through the USART
 */
void XPROGTarget_SendByte(const uint8_t Byte)
{
	/* Switch to Tx mode if currently in Rx mode */
	if (!(IsSending))
	  XPROGTarget_SetTxMode();

#ifdef HELL_WATCH_PORT
	while ( !(USARTE0.STATUS & USART_DREIF_bm) ) ;
    USARTE0.DATA = Byte;
#else
	/* Wait until there is space in the hardware Tx buffer before writing */
	while (!(UCSR1A & (1 << UDRE1)));
	UCSR1A |= (1 << TXC1);
	UDR1    = Byte;
#endif
}

/** Receives a byte via the hardware USART, blocking until data is received or timeout expired.
 *
 *  \return Received byte from the USART
 */
uint8_t XPROGTarget_ReceiveByte(void)
{
	/* Switch to Rx mode if currently in Tx mode */
	if (IsSending)
	  XPROGTarget_SetRxMode();
#ifdef HELL_WATCH_PORT
	while ( !(USARTE0.STATUS & USART_RXCIF_bm) && TimeoutTicksRemaining) ;
    return USARTE0.DATA;
#else
	/* Wait until a byte has been received before reading */
	while (!(UCSR1A & (1 << RXC1)) && TimeoutTicksRemaining);

	return UDR1;
#endif
}

/** Sends an IDLE via the USART to the attached target, consisting of a full frame of idle bits. */
void XPROGTarget_SendIdle(void)
{
	/* Switch to Tx mode if currently in Rx mode */
	if (!(IsSending))
	  XPROGTarget_SetTxMode();

#ifdef HELL_WATCH_PORT
	/* Need to do nothing for a full frame to send an IDLE */
	for (uint8_t i = 0; i < BITS_IN_USART_FRAME; i++)
	{
		/* Wait for a full cycle of the clock */
		while (PORTE.IN & (1 << 1));
		while (!(PORTE.IN & (1 << 1)));
		while (PORTE.IN & (1 << 1));
	}

#else
	/* Need to do nothing for a full frame to send an IDLE */
	for (uint8_t i = 0; i < BITS_IN_USART_FRAME; i++)
	{
		/* Wait for a full cycle of the clock */
		while (PIND & (1 << 5));
		while (!(PIND & (1 << 5)));
		while (PIND & (1 << 5));
	}
#endif
}

static void XPROGTarget_SetTxMode(void)
{
#ifdef HELL_WATCH_PORT
	/* Wait for a full cycle of the clock */
	while (PORTE.IN & (1 << 1));
	while (!(PORTE.IN & (1 << 1)));
	while (PORTE.IN & (1 << 1));

	PORTE.OUTSET  =  (1 << 3);
	PORTE.DIRSET  =  (1 << 3);

	USARTE0.CTRLB &= ~USART_RXEN_bm;
	USARTE0.CTRLB |=  USART_TXEN_bm;
#else
	/* Wait for a full cycle of the clock */
	while (PIND & (1 << 5));
	while (!(PIND & (1 << 5)));
	while (PIND & (1 << 5));

	PORTD  |=  (1 << 3);
	DDRD   |=  (1 << 3);

	UCSR1B &= ~(1 << RXEN1);
	UCSR1B |=  (1 << TXEN1);
#endif
	IsSending = true;
}

static void XPROGTarget_SetRxMode(void)
{
#ifdef HELL_WATCH_PORT
	while ( !(USARTE0.STATUS & USART_DREIF_bm) ) ;
	USARTE0.DATA;//clear status flag

	USARTE0.CTRLB &= ~USART_TXEN_bm;
	USARTE0.CTRLB |=  USART_RXEN_bm;

	PORTE.DIRCLR  =  (1 << 3);
	PORTE.OUTCLR  =  (1 << 3);
#else
	while (!(UCSR1A & (1 << TXC1)));
	UCSR1A |=  (1 << TXC1);

	UCSR1B &= ~(1 << TXEN1);
	UCSR1B |=  (1 << RXEN1);

	DDRD   &= ~(1 << 3);
	PORTD  &= ~(1 << 3);
#endif
	IsSending = false;
}

#endif

