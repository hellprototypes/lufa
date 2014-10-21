#ifdef HELL_WATCH_PORT
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include "hell_watch.h"

#include "PROGMEM_data.c"

//Bootloader OLED API define
#define BL_API_OLED_DRAW        (*((void(*)(uint8_t * draw_buffer))(0x08FFE/2)))
#define BL_API_OLED_DISP_CTRL  (*((void(*)(uint8_t))(0x08FF8/2)))

uint8_t oledBuffer[128*64/8];


void oled_draw(void)
{
	BL_API_OLED_DRAW(oledBuffer);
}
void oled_disp_off(void)
{
	BL_API_OLED_DISP_CTRL(0);
}

void hell_watch_line_clr(uint8_t line)//line: 0 ~ 7
{
	memset(&oledBuffer[line*128], 0, 128);
}

void hell_watch_screen_clr(void)
{
	memset(oledBuffer, 1024, 0);
	oled_draw();
}

void hell_watch_print_c(char c, uint16_t addr)
{
	uint16_t offset = (uint16_t)(c - ' ') * 5;
	uint8_t *p = &oledBuffer[addr];

	*p++ = 0;
	for(uint8_t i=0; i<5; i++) {
		*p++= pgm_read_byte(&Font_5x8[offset++]);
	}
}

void hell_watch_print(char *msg)
{
	char c;
	uint16_t addr;

	hell_watch_line_clr(7);
	addr = 128*7;
	c=*msg++;
	while(c  && (addr<1024)) {
		hell_watch_print_c(c, addr);
		c=*msg++;
		addr += 6;
	}
	oled_draw();
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

    for(i=0; i<sizeof(hell_watch_logo_avrisp); i++) {
		oledBuffer[i]= pgm_read_byte(&p[i]);
    }
	oled_draw();
}

void hell_watch_hw_init(void)
{
	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm; //enable all interrupt level

	//Setup usb clock
	OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 0x03;	//SRC: XOSC, MUL: 3
	OSC.CTRL =  OSC_PLLEN_bm | OSC_XOSCEN_bm;	// Disable internal 2MHz, enable pll
	while (!(OSC.STATUS & OSC_PLLRDY_bm));

	PR.PRGEN &= ~0x40; //enable clock to usb, which disable by bootloader
	
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
		hell_watch_screen_clr();
		oled_disp_off();
		_delay_ms(100);
		PORTA.OUTCLR = 1 << 6;//LDO off, Power lock at PA6
	}
}
#endif
