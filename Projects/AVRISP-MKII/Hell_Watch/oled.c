// OLED control

#include <avr/io.h>
#include <util/delay.h>
#include "oled.h"
#include "oled_defs.h"
#include "util.h"

#define OLED_CTRL_OFF()	OLED_Write(OLED_OFF)
#define OLED_TO_RST() 		clrbit(PORT_OLED_OUT, PIN_OLED_RST);


#define OLED_ACTIVE() 		clrbit(PORT_OLED_OUT, PIN_OLED_CS)
#define OLED_DEACTIVE()   setbit(PORT_OLED_OUT, PIN_OLED_CS)

#define CMD_MODE()			clrbit(PORT_OLED_OUT,PIN_OLED_DC)
#define DATA_MODE()		setbit(PORT_OLED_OUT,PIN_OLED_DC)

#define spiSelect()		OLED_ACTIVE()
#define spiDeselect()	OLED_DEACTIVE()
#define cmdMode()		CMD_MODE()
#define dataMode()		DATA_MODE()	
#define rstHigh()		
#define rstLow()		

const uint8_t oled_cfg_list[] = {
	//	0x00,		/*set lower column address*/
	//	0x10,		/*set higher column address*/
	0x20,      /*Horizontal addressing mode*/
	//	0x40,		/*set display start line*/
	0xB0,		/*set page address*/
	0x81,0xcf,	/*contract control  128   ??? FIXME*/
	//	0xA0,		/*set segment remap*/
	//	0xA6,		/*normal / reverse*/
	//	0xA8,0x3F,	/*multiplex ratio duty = 1/64*/
	//	0xC0,		/*Com scan direction*/
	//	0xD3,0x00,	/*set display offset*/
	//	0xD5,0x80,	/*set osc division*/
	0xD9,0xf1,	/*set pre-charge period*/
	//	0xDA,0x12,	/*set COM pins*/
	//	0xdb,0x40,	/*set vcomh*/
	0x8d,0x14,	/*set charge pump enable*/
	0xAF,		/*display ON*/
};

static inline bool oled_select(void)
{
	spiSelect();
	return true;
}

static inline bool oled_deselect(void)
{
	spiDeselect();
	return false;
}

#define MODE_CMD	(cmdMode())
#define MODE_DATA	(dataMode())
#define CHIPSELECT(mode) mode; \
	for(bool cs = oled_select(); cs; cs = oled_deselect()) 

#define spi_transfer_nr(cmd)	OLED_Write(cmd)

uint8_t oledBuffer[FRAME_BUFFER_SIZE];

static void resetPosition(void);
static void sendCmd(uint8_t);
static void sendCmd2(uint8_t, uint8_t);

void OLED_Write (uint8_t cmd)
{
	USARTD0.DATA= cmd;
	while(!testbit(USARTD0.STATUS,6));
	setbit(USARTD0.STATUS,6);
}

void oled_init()
{
#if 1  //use init status by bootloader
	for(uint16_t i=0;i<FRAME_BUFFER_SIZE;i++)
	{
		oledBuffer[i] = 0x00; // Clear buffer uint8_t
		spi_transfer_nr(0x00); // Send uint8_t
	}
	oled_flush();
#else
	uint8_t i;

	// Initialize USARTD0 for OLED
	USARTD0.BAUDCTRLA = 0x08;	// BSEL=1 (SPI clock = 8MHz, SSD1306 max is 10MHz)
	USARTD0.CTRLC	  = 0xC0;	// Master SPI mode,
	USARTD0.CTRLB	  = 0x08;	// Enable TX

	OLED_ACTIVE();
	CMD_MODE();
	for(i=0; i<sizeof(oled_cfg_list); i++){
		OLED_Write(oled_cfg_list[i]);
	}
	OLED_DEACTIVE();
#endif
}

static void resetPosition()
{
	CHIPSELECT(MODE_CMD)
	{
		spi_transfer_nr(0x22);
		spi_transfer_nr(0x00);
		spi_transfer_nr(0x07);

		spi_transfer_nr(0x21);
		spi_transfer_nr(0x00);
		spi_transfer_nr(0x7F);
	}
}

static void sendCmd2(uint8_t cmd, uint8_t val)
{
	CHIPSELECT(MODE_CMD)
	{
		spi_transfer_nr(cmd);
		spi_transfer_nr(val);
	}
}

static void sendCmd(uint8_t cmd)
{
	CHIPSELECT(MODE_CMD)
	{
		spi_transfer_nr(cmd);
	}		
}

void oled_flush()
{
	resetPosition();

	CHIPSELECT(MODE_DATA)
	{

		// Here we load a uint8_t from the global variable buffer into a fast local variable.
		// This is done while we're waiting for the SPI transfer to complete.
		// Delay cycles is also used to replace loop_until_bit_is_set().
		// In total, we go from ~3.84ms transfer time of the basic loop to ~2.31ms using local variables and delay cycles. (4MHz SPI, 1KB frame buffer)
		// As an added bonus we clear the frame buffer with the extra clock cycles which means we don't have to wait for memset() to clear it later on.
		// The minimum possible transfer time for 4MHz SPI and 1KB frame buffer is 2.048ms. 12.5% overhead (1 cycle to read SPSR, 1 cycle to set SPDR)... not bad :3

		for(uint16_t i=0;i<FRAME_BUFFER_SIZE;i++)
		{
			uint8_t next = oledBuffer[i]; // Load next uint8_t
			oledBuffer[i] = 0x00; // Clear buffer uint8_t

			spi_transfer_nr(next); // Send uint8_t
		}
	}
}

void oled_power(bool on)
{
	sendCmd(on ? OLED_ON : OLED_OFF);
}

void oled_setBrightness(uint8_t brightness)
{
	sendCmd2(OLED_SETCONTRAST, brightness);
}

void oled_setInvert(bool invert)
{
	sendCmd(invert ? OLED_INVERT : OLED_NONINVERT);
}

void oled_set180(bool rotate)
{
	if(rotate)
	{
		sendCmd(OLED_SEG_REMAP);
		sendCmd(OLED_SCANDIR_REMAP);
	}
	else
	{
		sendCmd(OLED_SEG_NML);
		sendCmd(OLED_SCANDIR_NML);
	}
}

