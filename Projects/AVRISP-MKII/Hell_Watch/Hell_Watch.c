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

#define MILLIS_TIMER_EN()		(TCC1.CTRLA = 0x04)

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

	USB.CTRLA = 0x00;
	PR.PRGEN &= ~0x40;//enable clock to usb

	//==============================================
	TCC1.PER = 1999;
	TCC1.INTCTRLA = 0x03; //HI Pri
	MILLIS_TIMER_EN();//DIV 8,  2M
}

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

void sys_power_off(void)
{
	hell_watch_print("Power OFF");

	USB_Disable();

	ms_delay(800);
	oled_power(false);
	ms_delay(100);
	vbat_m_pwr_off();
	OLED_TO_RST();
	ms_delay(100);
	pwr_lock_free();//LDO off
}

inline void hell_watch_hw_init(void)
{
	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
	uart_init();
	clock_init();
	oled_init();
}

inline void hell_watch_poll(void)
{
	if(testbit(PORT_CTRL_IN,PIN_KEY_MAIN)) {
		sys_power_off();
	}
	printf("%02x\r\n", USB.STATUS);
}

void hell_watch_print(char *msg)
{
	draw_string(msg,false,0,0);
	draw_end();
}


ISR(TCC1_OVF_vect)
{
	milliseconds++;
}


