#include "module.h"
#include "util/util.h"
#include <stdlib.h>
#include <memory.h>

#ifndef __USE_MISC
#define __USE_MISC // Для того, чтобы VS Code увидел константы M_PI и др.
#endif
#include <math.h>

#define M_PI_F ((float)M_PI)
#define SMOOTH_COEFFICIENT 10.0f
#define OCTAVE_AMPLITUDE 3

// x = [0; 1]
static float func(int tick, float x) {
	return -sinf((1 - tick / 100.f) * 4 * M_PI_F * x) * log2f(2 - x);
}

static float get_noise(int32_t x) {
	return randbuf2[i32max(0, x)];
}

static int32_t get_y(int tick, uint16_t width, uint16_t height, int32_t x) {
	int32_t cy = height / 2;
	return cy * (func(tick, (float) x / width) + get_noise(x) + 1);
}


static float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

static void add_octave(uint16_t width, uint16_t height, uint16_t step, uint16_t amplitude) {
	int32_t count = (width + step - 1) / step + 1;

	for (int32_t i = 0; i < count; i++) {
		randbuf1[i] = (float) rand() * (4.f / RAND_MAX) * amplitude / height;
	}

	for (uint16_t i = 0; i < width; i++) {
		uint16_t j = i / step;

		float t = (float)(i % step) / step;
		randbuf2[i] += lerp(randbuf1[j], randbuf1[j + 1], t);
	}
}

static void fill_randbufs(uint16_t width, uint16_t height) {
	memset(randbuf2, 0, width * sizeof(float));
	add_octave(width, height, 10, 3);
	add_octave(width, height, 20, 6);
	add_octave(width, height, 40, 12);
}

void gml_draw(int tick, uint16_t width, uint16_t height, color_t* frame) {
	fill_randbufs(width, height);

	memset(frame, 0, width * height * sizeof(color_t));

	int32_t prev_y = get_y(tick, width, height, -1);

	for (uint16_t x = 0; x < width; x++) {
		const int32_t curr_y = get_y(tick, width, height, x);
		const int32_t sy = i32min(height - 1, i32min(curr_y, prev_y));
		const int32_t ey = i32min(height - 1, i32max(curr_y, prev_y));

		for (int32_t y = sy; y <= ey; y++) {
			frame[y * width + x] = 0xFFFFFF;
		}

		prev_y = curr_y;
	}
}