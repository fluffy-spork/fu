/** \file
 * Super simple font from Asteroids.
 *
 * http://www.edge-online.com/wp-content/uploads/edgeonline/oldfiles/images/feature_article/2009/05/asteroids2.jpg
 *
 * Downloaded 2018-5-25 from https://github.com/osresearch/vst/blob/master/teensyv/asteroids_font.c
 */

#pragma once

#include <stdint.h>

typedef struct
{
	uint8_t points[8]; // 4 bits x, 4 bits y
} asteroids_char_t;

#define FONT_UP 0xFE
#define FONT_LAST 0xFF

extern const asteroids_char_t asteroids_font[];

/* Implementation */

#define P(x,y)	((((x) & 0xF) << 4) | (((y) & 0xF) << 0))
 
const asteroids_char_t asteroids_font[] = {
	['0' - 0x20] = { P(0,0), P(8,0), P(8,12), P(0,12), P(0,0), P(8,12), FONT_LAST },
	['1' - 0x20] = { P(4,0), P(4,12), P(3,10), FONT_LAST },
	['2' - 0x20] = { P(0,12), P(8,12), P(8,7), P(0,5), P(0,0), P(8,0), FONT_LAST },
	['3' - 0x20] = { P(0,12), P(8,12), P(8,0), P(0,0), FONT_UP, P(0,6), P(8,6), FONT_LAST },
	['4' - 0x20] = { P(0,12), P(0,6), P(8,6), FONT_UP, P(8,12), P(8,0), FONT_LAST },
	['5' - 0x20] = { P(0,0), P(8,0), P(8,6), P(0,7), P(0,12), P(8,12), FONT_LAST },
	['6' - 0x20] = { P(0,12), P(0,0), P(8,0), P(8,5), P(0,7), FONT_LAST },
	['7' - 0x20] = { P(0,12), P(8,12), P(8,6), P(4,0), FONT_LAST },
	['8' - 0x20] = { P(0,0), P(8,0), P(8,12), P(0,12), P(0,0), FONT_UP, P(0,6), P(8,6), },
	['9' - 0x20] = { P(8,0), P(8,12), P(0,12), P(0,7), P(8,5), FONT_LAST },
	[' ' - 0x20] = { FONT_LAST },
	['.' - 0x20] = { P(3,0), P(4,0), FONT_LAST },
	[',' - 0x20] = { P(2,0), P(4,2), FONT_LAST },
	['-' - 0x20] = { P(2,6), P(6,6), FONT_LAST },
	['+' - 0x20] = { P(1,6), P(7,6), FONT_UP, P(4,9), P(4,3), FONT_LAST },
	['!' - 0x20] = { P(4,0), P(3,2), P(5,2), P(4,0), FONT_UP, P(4,4), P(4,12), FONT_LAST },
	['#' - 0x20] = { P(0,4), P(8,4), P(6,2), P(6,10), P(8,8), P(0,8), P(2,10), P(2,2) },
	['^' - 0x20] = { P(2,6), P(4,12), P(6,6), FONT_LAST },
	['=' - 0x20] = { P(1,4), P(7,4), FONT_UP, P(1,8), P(7,8), FONT_LAST },
	['*' - 0x20] = { P(0,0), P(4,12), P(8,0), P(0,8), P(8,8), P(0,0), FONT_LAST },
	['_' - 0x20] = { P(0,0), P(8,0), FONT_LAST },
	['/' - 0x20] = { P(0,0), P(8,12), FONT_LAST },
	['\\' - 0x20] = { P(0,12), P(8,0), FONT_LAST },
	['@' - 0x20] = { P(8,4), P(4,0), P(0,4), P(0,8), P(4,12), P(8,8), P(4,4), P(3,6) },
	['$' - 0x20] = { P(6,2), P(2,6), P(6,10), FONT_UP, P(4,12), P(4,0), FONT_LAST },
	['&' - 0x20] = { P(8,0), P(4,12), P(8,8), P(0,4), P(4,0), P(8,4), FONT_LAST },
	['[' - 0x20] = { P(6,0), P(2,0), P(2,12), P(6,12), FONT_LAST },
	[']' - 0x20] = { P(2,0), P(6,0), P(6,12), P(2,12), FONT_LAST },
	['(' - 0x20] = { P(6,0), P(2,4), P(2,8), P(6,12), FONT_LAST },
	[')' - 0x20] = { P(2,0), P(6,4), P(6,8), P(2,12), FONT_LAST },
	['{' - 0x20] = { P(6,0), P(4,2), P(4,10), P(6,12), FONT_UP, P(2,6), P(4,6), FONT_LAST },
	['}' - 0x20] = { P(4,0), P(6,2), P(6,10), P(4,12), FONT_UP, P(6,6), P(8,6), FONT_LAST },
	['%' - 0x20] = { P(0,0), P(8,12), FONT_UP, P(2,10), P(2,8), FONT_UP, P(6,4), P(6,2) },
	['<' - 0x20] = { P(6,0), P(2,6), P(6,12), FONT_LAST },
	['>' - 0x20] = { P(2,0), P(6,6), P(2,12), FONT_LAST },
	['|' - 0x20] = { P(4,0), P(4,5), FONT_UP, P(4,6), P(4,12), FONT_LAST },
	[':' - 0x20] = { P(4,9), P(4,7), FONT_UP, P(4,5), P(4,3), FONT_LAST },
	[';' - 0x20] = { P(4,9), P(4,7), FONT_UP, P(4,5), P(1,2), FONT_LAST },
	['"' - 0x20] = { P(2,10), P(2,6), FONT_UP, P(6,10), P(6,6), FONT_LAST },
	['\'' - 0x20] = { P(2,6), P(6,10), FONT_LAST },
	['`' - 0x20] = { P(2,10), P(6,6), FONT_LAST },
	['~' - 0x20] = { P(0,4), P(2,8), P(6,4), P(8,8), FONT_LAST },
	['?' - 0x20] = { P(0,8), P(4,12), P(8,8), P(4,4), FONT_UP, P(4,1), P(4,0), FONT_LAST },
	['A' - 0x20] = { P(0,0), P(0,8), P(4,12), P(8,8), P(8,0), FONT_UP, P(0,4), P(8,4) },
	['B' - 0x20] = { P(0,0), P(0,12), P(4,12), P(8,10), P(4,6), P(8,2), P(4,0), P(0,0) },
	['C' - 0x20] = { P(8,0), P(0,0), P(0,12), P(8,12), FONT_LAST },
	['D' - 0x20] = { P(0,0), P(0,12), P(4,12), P(8,8), P(8,4), P(4,0), P(0,0), FONT_LAST },
	['E' - 0x20] = { P(8,0), P(0,0), P(0,12), P(8,12), FONT_UP, P(0,6), P(6,6), FONT_LAST },
	['F' - 0x20] = { P(0,0), P(0,12), P(8,12), FONT_UP, P(0,6), P(6,6), FONT_LAST },
	['G' - 0x20] = { P(6,6), P(8,4), P(8,0), P(0,0), P(0,12), P(8,12), FONT_LAST },
	['H' - 0x20] = { P(0,0), P(0,12), FONT_UP, P(0,6), P(8,6), FONT_UP, P(8,12), P(8,0) },
	['I' - 0x20] = { P(0,0), P(8,0), FONT_UP, P(4,0), P(4,12), FONT_UP, P(0,12), P(8,12) },
	['J' - 0x20] = { P(0,4), P(4,0), P(8,0), P(8,12), FONT_LAST },
	['K' - 0x20] = { P(0,0), P(0,12), FONT_UP, P(8,12), P(0,6), P(6,0), FONT_LAST },
	['L' - 0x20] = { P(8,0), P(0,0), P(0,12), FONT_LAST },
	['M' - 0x20] = { P(0,0), P(0,12), P(4,8), P(8,12), P(8,0), FONT_LAST },
	['N' - 0x20] = { P(0,0), P(0,12), P(8,0), P(8,12), FONT_LAST },
	['O' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,0), P(0,0), FONT_LAST },
	['P' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,6), P(0,5), FONT_LAST },
	['Q' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,4), P(0,0), FONT_UP, P(4,4), P(8,0) },
	['R' - 0x20] = { P(0,0), P(0,12), P(8,12), P(8,6), P(0,5), FONT_UP, P(4,5), P(8,0) },
	['S' - 0x20] = { P(0,2), P(2,0), P(8,0), P(8,5), P(0,7), P(0,12), P(6,12), P(8,10) },
	['T' - 0x20] = { P(0,12), P(8,12), FONT_UP, P(4,12), P(4,0), FONT_LAST },
	['U' - 0x20] = { P(0,12), P(0,2), P(4,0), P(8,2), P(8,12), FONT_LAST },
	['V' - 0x20] = { P(0,12), P(4,0), P(8,12), FONT_LAST },
	['W' - 0x20] = { P(0,12), P(2,0), P(4,4), P(6,0), P(8,12), FONT_LAST },
	['X' - 0x20] = { P(0,0), P(8,12), FONT_UP, P(0,12), P(8,0), FONT_LAST },
	['Y' - 0x20] = { P(0,12), P(4,6), P(8,12), FONT_UP, P(4,6), P(4,0), FONT_LAST },
	['Z' - 0x20] = { P(0,12), P(8,12), P(0,0), P(8,0), FONT_UP, P(2,6), P(6,6), FONT_LAST },
};

