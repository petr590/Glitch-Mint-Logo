#include "module.h"
#include <stdlib.h>

void glspl_draw(int tick, uint32_t width, uint32_t height, color_t* frame) {
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			frame[y * width + x] = rand();
		}
	}
}