#ifndef _HELL_WATCH_H_
#define _HELL_WATCH_H_

#define LOGO_INDEX_AVRISP		0
#define LOGO_INDEX_USB2SERIAL	1
#define LOGO_INDEX_SDREADER		2

#define KEY_NONE				0
#define KEY_BTM_LEFT			1
#define KEY_BTM_RIGHT			2
#define KEY_3WS_DOWN			3
#define KEY_3WS_PUSH			4
#define KEY_3WS_UP				5
#define KEY_TOP_RIGHT			6
#define KEY_TOP_LEFT			7
#define KEY_MAIN				8
#define KEY_WAIT_FLAG			0xFF

void hell_watch_print(char *msg);
void hell_watch_disp_logo(uint8_t index);
void hell_watch_hw_init(void);
void hell_watch_poll(void);
uint8_t get_key_value(void);
#endif