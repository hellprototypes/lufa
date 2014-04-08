/*
 * Project: Digital Wristwatch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */

#ifndef DRAW_H_
#define DRAW_H_
#include <stdbool.h>

#define SMALLFONT_WIDTH 5
#define SMALLFONT_HEIGHT 8

typedef struct{
	uint8_t x;
	uint8_t y;
	const uint8_t* bitmap;
	uint8_t width;
	uint8_t height;
	uint8_t foreColour;
	bool invert;
	uint8_t offsetY;
}s_image;

#define newImage(x, y, bitmap, width, height, foreColour, invert, offsetY) \
(s_image){ \
x, \
y, \
bitmap, \
width, \
height, \
foreColour, \
invert, \
offsetY \
}

void draw_string(char*, bool, uint8_t, uint8_t);
void draw_bitmap_s2(s_image*);
//void draw_string_time(char*, bool, uint8_t, uint8_t);
void draw_clearArea(uint8_t, uint8_t, uint8_t);//, uint8_t);
void draw_end(void);

#endif /* DRAW_H_ */