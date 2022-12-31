#include "NatureLogo.hpp"

NatureLogo::NatureLogo(uint16_t width, uint16_t height) {
	cx = width / 2;
	cy = height / 2;

	uint16_t s = (width > height ? height : width) / 2;

	r_large = s / 2;
	r_short = s / 2 - s / 10;

	l_width = s * 7 / 4;
	l_height = s / 32 + 1;
	l_x = (width - l_width) / 2 + 1;
	l_y = s * 8 / 7;
}

void NatureLogo::draw(M5GFX & display, bool reverse) {
	const uint16_t fore = reverse ? basecolor : 0xffff;
	const uint16_t back = reverse ? accentcolor : basecolor;

	display.fillScreen(back);
	display.fillCircle(cx, cy, r_large, fore);
	display.fillCircle(cx, cy, r_short, back);
	display.fillRect(l_x, l_y, l_width, l_height, fore);
}

