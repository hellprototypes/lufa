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
 *  Main source file for the Mouse demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "Mouse.h"

#include "../HellWatch/hell_watch.h"

#include "i2c.h"
#include "i2c_dev.h"

/** Buffer to hold the previously generated Mouse HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevMouseHIDReportBuffer[sizeof(USB_MouseReport_Data_t)];

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Mouse_HID_Interface =
	{
		.Config =
			{
				.InterfaceNumber              = INTERFACE_ID_Mouse,
				.ReportINEndpoint             =
					{
						.Address              = MOUSE_EPADDR,
						.Size                 = MOUSE_EPSIZE,
						.Banks                = 1,
					},
				.PrevReportINBuffer           = PrevMouseHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevMouseHIDReportBuffer),
			},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

#ifdef HELL_WATCH_PORT
	hell_watch_print("Ready...");
	for (;;) {
		hell_watch_poll();
#else
	for (;;) {
#endif
		HID_Device_USBTask(&Mouse_HID_Interface);
		USB_USBTask();
	}
}

#ifdef HELL_WATCH_PORT
int16_t last_x, last_y;
void sensor_init(void)
{
	uint8_t cmd[4];
	i2c_init();
	last_x = 0;
	last_y = 0;

	cmd[0] = 0x00;//exit sleep mode
	i2c_dev_write(0xD0, 0x6B, cmd, 1);
	cmd[0] = 0x04;//SMPLRT_DIV
	cmd[1] = 0x04;//CONFIG
	cmd[2] = 0x00;//GYRO_CONFIG
	cmd[3] = 0x00;//ACCEL_CONFIG
	i2c_dev_write(0xD0, 0x19, cmd, 4);
}

bool sensor_get_data(int16_t *x, int16_t *y)
{
	uint8_t read[4];
	bool ret = i2c_dev_read(0xD0, 0x45, read, 4);
	if(ret) {
		*x = read[1] | (read[0] << 8);
		*y = read[3] | (read[2] << 8);
	} else {
		*x = 0;
		*y = 0;
	}
	return ret;
}
#endif

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
#ifdef HELL_WATCH_PORT
	hell_watch_hw_init();
	hell_watch_disp_logo(0xFF);
	sensor_init();
#else
#if (ARCH == ARCH_AVR8)
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
#elif (ARCH == ARCH_XMEGA)
	/* Start the PLL to multiply the 2MHz RC oscillator to 32MHz and switch the CPU core to run from it */
	XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
	XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);

	/* Start the 32MHz internal RC oscillator and start the DFLL to increase it to 48MHz using the USB SOF as a reference */
	XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
	XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
#endif
#endif

	/* Hardware Initialization */
	Joystick_Init();
	LEDs_Init();
	Buttons_Init();
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

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Mouse_HID_Interface);

	USB_Device_EnableSOFEvents();

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Mouse_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Mouse_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of uint8_ts written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	USB_MouseReport_Data_t* MouseReport = (USB_MouseReport_Data_t*)ReportData;

	uint8_t key = get_key_value();
	int16_t x, y;
	sensor_get_data(&x, &y);
	x = x >> 7;
	y = y >> 7;

	if((x>2) || (x<-2)) {
		MouseReport->X = x/abs(x);
	}
	if((y>2) || (y<-2)) {
		MouseReport->Y = y/abs(y);
	}

	if((last_x != x) || (last_y != y)) {
		char msg[24];
		sprintf(msg, "x:%d y:%d", x, y);
		hell_watch_print(msg);
	}
	last_x = x;
	last_y = y;

	switch(key) {
	case KEY_TOP_LEFT:
		MouseReport->X = -1;
		break;
	case KEY_TOP_RIGHT:
		MouseReport->X =  1;
		break;
	case KEY_BTM_LEFT:
		MouseReport->Button |= (1 << 0);
		break;
	case KEY_BTM_RIGHT:
		MouseReport->Button |= (1 << 1);
		break;
	case KEY_3WS_DOWN:
		MouseReport->Y = -1;
		break;
	case KEY_3WS_PUSH:
		break;
	case KEY_3WS_UP:
		MouseReport->Y =  1;
		break;
	default:
		break;
	}

	*ReportSize = sizeof(USB_MouseReport_Data_t);
	return true;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in uint8_ts of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	// Unused (but mandatory for the HID class driver) in this demo, since there are no Host->Device reports
}

