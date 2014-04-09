#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/atomic.h>
#include <avr/sleep.h>
#include <stdio.h>

#include <LUFA/Drivers/USB/USB.h>
#include "Hell_Watch.h"
#include "draw.h"
#include "util.h"
#include "oled.h"
#include "uart.h"

static volatile uint16_t milliseconds;

#define MILLIS_TIMER_EN()		()

// Initialise library
void clock_init(void)
{
	//Chang system clock to ext 16M clock

	DFLLRC32M.CTRL = 0;

	CCP = CCP_IOREG_gc; //Security Signature to modify clock 
	CLK.CTRL = CLK_SCLKSEL_RC2M_gc; // Run at 2M

	OSC.CTRL = OSC_RC2MEN_bm;

	OSC.XOSCCTRL = 0xCB;	// Crystal type 0.4-16 MHz XTAL - 16K CLK Start Up time
	OSC.CTRL = OSC_RC2MEN_bm | OSC_XOSCEN_bm;

	while (!(OSC.STATUS & OSC_XOSCRDY_bm));//wait xosc ready

	CCP = CCP_IOREG_gc; //Security Signature to modify clock 
	CLK.CTRL = CLK_SCLKSEL_XOSC_gc; // Use xosc as system clock

	OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 0x03;	//SRC: XOSC, MUL: 3
	OSC.CTRL =  OSC_PLLEN_bm | OSC_XOSCEN_bm;	 // Disable internal 2MHz, enable pll

	while (!(OSC.STATUS & OSC_PLLRDY_bm));


}
/*
uint16_t millis_get(void)
{
	uint16_t ms;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		ms = milliseconds;
	}

	return ms;
}

void ms_delay(uint16_t ms)
{
	uint16_t current, base, delay;

	base = millis_get();

	delay = ms;

	do {
		current = millis_get();

		if(current >= base) {
			if( current - base >= delay) {
				return;
			}
		} else {
			delay = delay - (0xFFFF - base);
			base = 0;
		}
		//sleep_mode();
	} while(1);
}
*/
void sys_power_off(void)
{
	hell_watch_print("Power OFF");

	USB_Disable();

	Delay_MS(800);
	oled_power(false);
	Delay_MS(100);
	vbat_m_pwr_off();
	OLED_TO_RST();
	Delay_MS(100);
	pwr_lock_free();//LDO off
}

inline void deinit_usb(void)
{
	//USBPSDIV[2:0] bits are locked as long as the USB clock source is enabled(USBSEN=1).
	//So clear USBSEN bit first
	CLK.USBCTRL = 0x00;
	CLK.USBCTRL = 0x00;
	
	PR.PRGEN &= ~0x40;//enable clock to usb
	USB.CTRLA = 0x00;
	USB.CTRLB = 0x00;
	USB.ADDR = 0x00;
	USB.EPPTRL = 0x0000;
	USB.INTFLAGSACLR = 0xFF;
	USB.INTCTRLA = 0x00;
	USB.INTCTRLB = 0x00;
	USB.INTFLAGSACLR = 0xFF;
	USB.INTFLAGSBCLR = 0xFF;
}
inline void hell_watch_hw_init(void)
{
	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
	uart_init();
	clock_init();
	oled_init();

	deinit_usb();

	//hell_watch_print("HW Init DONE");
}

inline void hell_watch_poll(void)
{
	if(testbit(PORT_CTRL_IN,PIN_KEY_MAIN)) {
		sys_power_off();
	}
}

void hell_watch_print(char *msg)
{
	draw_string(msg,false,0,0);
	draw_end();
}

