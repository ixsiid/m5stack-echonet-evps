#pragma once

#include <M5GFX.h>

// Only use R5G6B5 color depth
class NatureLogo {
public:
	NatureLogo(uint16_t width, uint16_t height);
	void draw(M5GFX & display, bool reverse = true);

private:
	uint16_t cx, cy, r_large, r_short, l_width, l_height, l_x, l_y;
	
	// 0x152332, 0001 0101, 0010 0101, 0101 0010
	const uint16_t basecolor = 0b0001000100000110;
	// 0xe5d7c2, 1110 0101, 1101 0111, 1100 0010
	const uint16_t accentcolor = 0b1110011010111000;
};
