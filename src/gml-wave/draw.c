#include "module.h"
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#define M_PI_F ((float)M_PI)
#define SMOOTH_COEFFICIENT 10.0f
#define OCTAVE_AMPLITUDE 3

static int imin(int a, int b) {
	return a < b ? a : b;
}

static int imax(int a, int b) {
	return a > b ? a : b;
}

// x = [0; 1]
static float func(int tick, float x) {
	return -sinf((1 - tick / 100.f) * 4 * M_PI_F * x) * log2f(2 - x);
}

static float get_noise(int x) {
	return randbuf2[imax(0, x)];
}

static int get_y(int tick, int width, int height, int x) {
	int cy = height / 2;
	return (int)(cy * (func(tick, (float) x / width) + get_noise(x) + 1));
}


static float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

static void add_octave(uint32_t width, uint32_t height, int step, int amplitude) {
	int count = (width + step - 1) / step + 1;

	for (int i = 0; i < count; i++) {
		randbuf1[i] = (float) rand() * (4.f / RAND_MAX) * amplitude / height;
	}

	for (int i = 0; i < width; i++) {
		int j = i / step;

		float t = (float)(i % step) / step;
		randbuf2[i] += lerp(randbuf1[j], randbuf1[j + 1], t);
	}
}

static void fill_randbufs(uint32_t width, uint32_t height) {
	memset(randbuf2, 0, width * sizeof(float));
	add_octave(width, height, 10, 3);
	add_octave(width, height, 20, 6);
	add_octave(width, height, 40, 12);
}

void gml_draw(int tick, uint32_t width, uint32_t height, color_t* frame) {
	fill_randbufs(width, height);

	memset(frame, 0, width * height * sizeof(color_t));

	int prev_y = get_y(tick, width, height, -1);

	for (int x = 0; x < width; x++) {
		const int curr_y = get_y(tick, width, height, x);
		const int sy = imin(height - 1, imin(curr_y, prev_y));
		const int ey = imin(height - 1, imax(curr_y, prev_y));

		for (int y = sy; y <= ey; y++) {
			frame[y * width + x] = 0xFFFFFF;
		}

		prev_y = curr_y;
	}

	for (int y = 0; y < 10; y++) {
		frame[(y + height / 2) * width + 10] = 0xFF0000;
	}
}