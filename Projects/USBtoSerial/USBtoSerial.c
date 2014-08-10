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
 *  Main source file for the USBtoSerial project. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 */

#include "USBtoSerial.h"

#include "../HellWatch/hell_watch.h"

/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
static RingBuffer_t USBtoUSART_Buffer;

/** Underlying data buffer for \ref USBtoUSART_Buffer, where the stored bytes are located. */
static uint8_t      USBtoUSART_Buffer_Data[128];

/** Circular buffer to hold data from the serial port before it is sent to the host. */
static RingBuffer_t USARTtoUSB_Buffer;

/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes are located. */
static uint8_t      USARTtoUSB_Buffer_Data[128];

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber         = INTERFACE_ID_CDC_CCI,
				.DataINEndpoint                 =
					{
						.Address                = CDC_TX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.DataOUTEndpoint                =
					{
						.Address                = CDC_RX_EPADDR,
						.Size                   = CDC_TXRX_EPSIZE,
						.Banks                  = 1,
					},
				.NotificationEndpoint           =
					{
						.Address                = CDC_NOTIFICATION_EPADDR,
						.Size                   = CDC_NOTIFICATION_EPSIZE,
						.Banks                  = 1,
					},
			},
	};

#ifdef HELL_WATCH_PORT
uint32_t RX_Count, last_RX_Count;
uint32_t TX_Count, last_TX_Count;
char print_msg[22];
#endif
/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	RingBuffer_InitBuffer(&USBtoUSART_Buffer, USBtoUSART_Buffer_Data, sizeof(USBtoUSART_Buffer_Data));
	RingBuffer_InitBuffer(&USARTtoUSB_Buffer, USARTtoUSB_Buffer_Data, sizeof(USARTtoUSB_Buffer_Data));

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();
#ifdef HELL_WATCH_PORT
	RX_Count = 0;
	TX_Count = 0;
	last_RX_Count = 0;
	last_TX_Count = 0;
	hell_watch_print("Ready...");
	for (;;) {
		hell_watch_poll();
#else
	for (;;) {
#endif

		/* Only try to read in bytes from the CDC interface if the transmit buffer is not full */
		if (!(RingBuffer_IsFull(&USBtoUSART_Buffer)))
		{
			int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);

			/* Store received byte into the USART transmit buffer */
			if (!(ReceivedByte < 0))
			  RingBuffer_Insert(&USBtoUSART_Buffer, ReceivedByte);
		}

		uint16_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
		if (BufferCount)
		{
			Endpoint_SelectEndpoint(VirtualSerial_CDC_Interface.Config.DataINEndpoint.Address);

			/* Check if a packet is already enqueued to the host - if so, we shouldn't try to send more data
			 * until it completes as there is a chance nothing is listening and a lengthy timeout could occur */
			if (Endpoint_IsINReady())
			{
				/* Never send more than one bank size less one byte to the host at a time, so that we don't block
				 * while a Zero Length Packet (ZLP) to terminate the transfer is sent if the host isn't listening */
				uint8_t BytesToSend = MIN(BufferCount, (CDC_TXRX_EPSIZE - 1));

				/* Read bytes from the USART receive buffer into the USB IN endpoint */
				while (BytesToSend--)
				{
					/* Try to send the next byte of data to the host, abort if there is an error without dequeuing */
					if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,
											RingBuffer_Peek(&USARTtoUSB_Buffer)) != ENDPOINT_READYWAIT_NoError)
					{
						break;
					}

					/* Dequeue the already sent byte from the buffer now we have confirmed that no transmission error occurred */
					RingBuffer_Remove(&USARTtoUSB_Buffer);
				}
			}
		}

		/* Load the next byte from the USART transmit buffer into the USART */
		if (!(RingBuffer_IsEmpty(&USBtoUSART_Buffer)))
#ifdef HELL_WATCH_PORT
		{
		  Serial_SendByte(&USARTE0, RingBuffer_Remove(&USBtoUSART_Buffer));
		  TX_Count++;
		} else {
			//Comment below if message print process slow down the rs232 speed
			if(((last_RX_Count != RX_Count) || ((last_TX_Count != TX_Count)))) {
				last_RX_Count = RX_Count;
				last_TX_Count = TX_Count;
				sprintf_P(print_msg, PSTR("RX:%lu TX:%lu"),RX_Count, TX_Count);
				hell_watch_print(print_msg);
			}
		}
#else
		  Serial_SendByte(RingBuffer_Remove(&USBtoUSART_Buffer));
#endif

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#endif
#ifdef HELL_WATCH_PORT
	hell_watch_hw_init();
	hell_watch_disp_logo(1);
#endif
	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

#ifdef HELL_WATCH_PORT
/** ISR to manage the reception of data from the serial port, placing received bytes into a circular buffer
 *  for later transmission to the host.
 */
ISR(USARTE0_RXC_vect)
{
	uint8_t ReceivedByte = USARTE0.DATA;
	RX_Count++;

	if (USB_DeviceState == DEVICE_STATE_Configured)
	  RingBuffer_Insert(&USARTtoUSB_Buffer, ReceivedByte);
}

/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
	uint8_t ConfigMask = 0;
	uint32_t parity = 0;
	uint32_t stop_bit = 1, bits = 5;
	uint32_t baud;

	switch (CDCInterfaceInfo->State.LineEncoding.ParityType)
	{
		case CDC_PARITY_Odd:
			parity = 1;
			ConfigMask = USART_PMODE0_bm | USART_PMODE1_bm;
			break;
		case CDC_PARITY_Even:
			parity = 2;
			ConfigMask = USART_PMODE1_bm;
			break;
	}

	if (CDCInterfaceInfo->State.LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits) {
		ConfigMask |= USART_SBMODE_bm;
		stop_bit = 2;
	}

	bits = CDCInterfaceInfo->State.LineEncoding.DataBits;
	switch (bits)
	{
		case 6:
			ConfigMask |= USART_CHSIZE0_bm;
			break;
		case 7:
			ConfigMask |= USART_CHSIZE1_bm;
			break;
		case 8:
			ConfigMask |= USART_CHSIZE0_bm | USART_CHSIZE1_bm;
			break;
	}

	/* Must turn off USART before reconfiguring it, otherwise incorrect operation may occur */
	USARTE0.CTRLA = 0;
	USARTE0.CTRLB = 0;
	USARTE0.CTRLC = 0;

	PORTE.DIRSET =	(1 << 3);//TXD pin Output
	PORTE.DIRCLR =	(1 << 2);//RXD pin Input

	/* Set the new baud rate before configuring the USART */
	baud = CDCInterfaceInfo->State.LineEncoding.BaudRateBPS;
	uint16_t BaudValue = SERIAL_2X_UBBRVAL(baud);
	USARTE0.BAUDCTRLB = (BaudValue >> 8);
	USARTE0.BAUDCTRLA = (BaudValue & 0xFF);
	
	/* Reconfigure the USART in double speed mode for a wider baud rate range at the expense of accuracy */
	USARTE0.CTRLC = ConfigMask;
	USARTE0.CTRLB = USART_CLK2X_bm | USART_RXEN_bm | USART_TXEN_bm;
	USARTE0.CTRLA = USART_RXCINTLVL0_bm;

	sprintf_P(print_msg, PSTR("%lu %lu %c %lu"), baud, bits, (parity == 0) ? 'N' : ((parity == 1) ? 'O' : 'E'), stop_bit);
	hell_watch_print(print_msg);
}

#else
/** ISR to manage the reception of data from the serial port, placing received bytes into a circular buffer
 *  for later transmission to the host.
 */
ISR(USART1_RX_vect, ISR_BLOCK)
{
	uint8_t ReceivedByte = UDR1;

	if (USB_DeviceState == DEVICE_STATE_Configured)
	  RingBuffer_Insert(&USARTtoUSB_Buffer, ReceivedByte);
}

/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
	uint8_t ConfigMask = 0;

	switch (CDCInterfaceInfo->State.LineEncoding.ParityType)
	{
		case CDC_PARITY_Odd:
			ConfigMask = ((1 << UPM11) | (1 << UPM10));
			break;
		case CDC_PARITY_Even:
			ConfigMask = (1 << UPM11);
			break;
	}

	if (CDCInterfaceInfo->State.LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits)
	  ConfigMask |= (1 << USBS1);

	switch (CDCInterfaceInfo->State.LineEncoding.DataBits)
	{
		case 6:
			ConfigMask |= (1 << UCSZ10);
			break;
		case 7:
			ConfigMask |= (1 << UCSZ11);
			break;
		case 8:
			ConfigMask |= ((1 << UCSZ11) | (1 << UCSZ10));
			break;
	}

	/* Must turn off USART before reconfiguring it, otherwise incorrect operation may occur */
	UCSR1B = 0;
	UCSR1A = 0;
	UCSR1C = 0;

	/* Set the new baud rate before configuring the USART */
	UBRR1  = SERIAL_2X_UBBRVAL(CDCInterfaceInfo->State.LineEncoding.BaudRateBPS);

	/* Reconfigure the USART in double speed mode for a wider baud rate range at the expense of accuracy */
	UCSR1C = ConfigMask;
	UCSR1A = (1 << U2X1);
	UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));
}
#endif
