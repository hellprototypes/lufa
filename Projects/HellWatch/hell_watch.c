#ifdef HELL_WATCH_PORT
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "hell_watch.h"

#include "PROGMEM_data.c"

#define OLED_ACTIVE() 		(PORTD.OUTCLR = 1 << 0)//clrbit(PORT_OLED_OUT, PIN_OLED_CS)
#define OLED_DEACTIVE()   (PORTD.OUTSET = 1 << 0)//setbit(PORT_OLED_OUT, PIN_OLED_CS)
#define CMD_MODE()			(PORTD.OUTCLR = 1 << 2)//clrbit(PORT_OLED_OUT,PIN_OLED_DC)
#define DATA_MODE()		(PORTD.OUTSET = 1 << 2)//setbit(PORT_OLED_OUT,PIN_OLED_DC)


void hell_watch_oled_cmd (uint8_t cmd)
{
    USARTD0.DATA= cmd;
    while(!(USARTD0.STATUS & (1 << 6)));
    USARTD0.STATUS |= 1 << 6;
}

void hell_watch_set_cursor(uint8_t page, uint8_t column)
{
	if(page > 7) {
		return;
	}
	CMD_MODE();
    hell_watch_oled_cmd(0xB0 | page);//OLED_SET_PAGE
	hell_watch_oled_cmd(0x10 | (column >> 4));//OLED_SET_COL_HI :  Set column at 0
    hell_watch_oled_cmd(0x00 | (column & 0x0F));//OLED_SET_COL_LO
}

void hell_watch_print_c(char c)
{
	uint16_t offset = (uint16_t)(c - ' ') * 5;

	USARTD0.DATA = 0x00;
	for(uint8_t i=0; i<5; i++) {
		while(!(USARTD0.STATUS &  (1<<5)));
		USARTD0.DATA= pgm_read_byte(&Font_5x8[offset++]);
	}
	while(!(USARTD0.STATUS &  (1<<6)));
	USARTD0.STATUS |= 1 <<6;
}

void hell_watch_line_clr(uint8_t line)//line: 0 ~ 7
{
    OLED_ACTIVE();

	hell_watch_set_cursor(line, 0);

	DATA_MODE();
	for(uint8_t i=0; i<128; i++) {
		while(!(USARTD0.STATUS &  (1<<5)));
		USARTD0.DATA= 0x00;
	}
	while(!(USARTD0.STATUS &  (1<<6)));
	USARTD0.STATUS |= 1 <<6;
	OLED_DEACTIVE();
}
void hell_watch_screen_clr(void)
{
	for(uint8_t i=0; i<8; i++) {
		hell_watch_line_clr(i);
	}
}
void hell_watch_print(char *msg)
{
	char c;

	hell_watch_line_clr(7);

    OLED_ACTIVE();
	hell_watch_set_cursor(7, 0);
	DATA_MODE();
	c=*msg++;
	while(c) {
		hell_watch_print_c(c);
		c=*msg++;
	}

	OLED_DEACTIVE();
}

void hell_watch_disp_logo(uint8_t index)
{
    uint16_t i;
	const uint8_t *p;
	if(index == LOGO_INDEX_AVRISP) {
		p = hell_watch_logo_avrisp;
	} else if(index == LOGO_INDEX_USB2SERIAL) {
		p = hell_watch_logo_usb2serial;
	} else if(index == LOGO_INDEX_SDREADER) {
		p = hell_watch_logo_sdreader;
	} else {
		return;
	}
    OLED_ACTIVE();
	hell_watch_set_cursor(0,0);

    DATA_MODE();
   	// Send data to display
    for(i=0; i<sizeof(hell_watch_logo_avrisp); i++) {
        while(!(USARTD0.STATUS &  (1<<5)));
		USARTD0.DATA= pgm_read_byte(&p[i]);
    }
    while(!(USARTD0.STATUS &  (1<<6)));
    USARTD0.STATUS |= 1 <<6;

    OLED_DEACTIVE();
}

void hell_watch_hw_init(void)
{
	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm; //enable all interrupt level
	//Chang system clock to ext 16M clock and setup usb clock

	//Clock init
	DFLLRC32M.CTRL = 0;

	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_RC2M_gc; // Run at 2M

	OSC.CTRL = OSC_RC2MEN_bm;

	OSC.XOSCCTRL = 0xCB;	// Crystal type 0.4-16 MHz XTAL - 16K CLK Start Up time
	OSC.CTRL = OSC_RC2MEN_bm | OSC_XOSCEN_bm;

	while (!(OSC.STATUS & OSC_XOSCRDY_bm));//wait xosc ready

	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_XOSC_gc; // Use xosc as system clock

	OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 0x03;	//SRC: XOSC, MUL: 3
	OSC.CTRL =  OSC_PLLEN_bm | OSC_XOSCEN_bm;	// Disable internal 2MHz, enable pll

	while (!(OSC.STATUS & OSC_PLLRDY_bm));

	//Deinit USB REG, which init by bootloader
	CLK.USBCTRL = 0x00;//Disable USB clock,

	PR.PRGEN &= ~0x40; //enable clock to usb, which disable by bootloader
	USB.CTRLA = 0x00;
	USB.CTRLB = 0x00;
	
	//buttons init
	ADCA.CTRLB        = 0x64;			// High current limit, signed mode, no free run, 8 bit
	ADCA.PRESCALER 	  = 0x07;		// Prescaler 512 (62.5kHZ ADC clock)
	ADCA.CTRLA		  = 0x01;	// Enable ADC

	ADCA.REFCTRL	  = 0x12;	// REF= VCC/1.6 (3.3V/1.6 = 2.0625V)
	ADCA.CH1.MUXCTRL  = 0x08;	// Channel 1 input: ADC1 pin
	ADCA.CH1.CTRL	  = 0x01;	// Single-ended positive input signal
	
	hell_watch_screen_clr();
}

const uint8_t key_value_map[] = {
	250, 231, 210, 186, 158, 126, 86, 0,
};

#define START_ADC()			(ADCA.CH1.CTRL |= 0x80)
#define WAIT_CONVERT()		do{ while(!(ADCA.CH1.INTFLAGS & 0x01)); ADCA.CH1.INTFLAGS = 1;}while(0)

uint8_t get_key_value(void)
{
	uint8_t  adc0, adc1, key;

	if(PORTA.IN & (1<<7)) {
		return KEY_MAIN;
	} else	{
		START_ADC();
		WAIT_CONVERT();
		adc0 = ADCA.CH1.RESL;
		if(adc0 <= 250) {
			_delay_ms(10);
			START_ADC();
			WAIT_CONVERT();
			adc1 = ADCA.CH1.RESL;
			if(adc0 - adc1 < 2) {
				for(key = 0; key < 8; key++) {
					if(adc0 >= key_value_map[key]) {
						return key;
					}
				}
			}
		}
	}

	return KEY_NONE;
}
extern void USB_Disable(void);
void hell_watch_poll(void)
{
	if(PORTA.IN & (1<<7)) {//Main key at PA7
		hell_watch_print("Power OFF");
		USB_Disable();
		_delay_ms(900);
		OLED_ACTIVE();
		CMD_MODE();
		hell_watch_oled_cmd(0xAE);//DISP OFF
		OLED_DEACTIVE();
		_delay_ms(100);
		PORTA.OUTCLR = 1 << 6;//LDO off, Power lock at PA6
	}
}
#endif
