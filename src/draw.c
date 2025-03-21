#include "draw.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define BG_GS_1 0x1A
#define BG_GS_2 0x19

#define BG_GLITCH_DURATION 4
#define BG_GLITCH_PERIOD 6
#define MAJOR_GLITCH_DURATON 10
#define MINOR_GLITCH_DURATON 2
#define MINOR_GLITCH_PERIOD 15

static inline uint32_t u32min(uint32_t num1, uint32_t num2) {
	return num1 < num2 ? num1 : num2;
}

static inline int32_t i32min(int32_t num1, int32_t num2) {
	return num1 < num2 ? num1 : num2;
}

static inline int32_t i32max(int32_t num1, int32_t num2) {
	return num1 > num2 ? num1 : num2;
}

static inline int randrange(int from, int to) {
	return rand() % (to - from + 1) + from;
}

static inline color_t mix(color_t rgb, color_t argb) {
	uint8_t alpha = argb >> 24;
	uint8_t r = ((uint8_t)(rgb >> 16) * (0xFF - alpha) + (uint8_t)(argb >> 16) * alpha) / 0xFF;
	uint8_t g = ((uint8_t)(rgb >>  8) * (0xFF - alpha) + (uint8_t)(argb >>  8) * alpha) / 0xFF;
	uint8_t b = ((uint8_t)(rgb      ) * (0xFF - alpha) + (uint8_t)(argb      ) * alpha) / 0xFF;
	return r << 16 | g << 8 | b;
}


static float get_noise(int tick) {
	if (tick < MAJOR_GLITCH_DURATON) {
		return (float)(MAJOR_GLITCH_DURATON - tick) * (1.f / MAJOR_GLITCH_DURATON);
	}

	int seed = rand();
	srand(tick / MINOR_GLITCH_PERIOD * RANDOM_CONSTANT);
	int offset = rand() % MINOR_GLITCH_PERIOD;
	srand(seed);

	return ((tick + offset) % MINOR_GLITCH_PERIOD < MINOR_GLITCH_DURATON) ? 0.8f : 0.0f;
}


static void draw_bg(int tick, uint32_t width, uint32_t height, uint8_t* frame, color_t* bg_buffer) {
	int seed = rand();
	srand(tick / BG_GLITCH_PERIOD * RANDOM_CONSTANT);
	int offset = rand() % BG_GLITCH_PERIOD;
	srand(seed);

	if (tick % 4 < 2 && (tick + offset) % BG_GLITCH_PERIOD < BG_GLITCH_DURATION) {
		uint32_t min_h = height / 20;
		uint32_t max_h = height / 5;

		int dark = 0;
		for (uint32_t y = 0; y < height; dark ^= 1) {
			uint32_t line_h = u32min(height - y, randrange(min_h, max_h));
			uint8_t gs = dark ? BG_GS_2 : BG_GS_1;

			memset(frame + y * width * sizeof(color_t), gs, line_h * width * sizeof(color_t));
			memset((void*)bg_buffer + y * sizeof(color_t), gs, line_h * sizeof(color_t));

			y += line_h;
		}

	} else {
		memset(frame, BG_GS_1, width * height * sizeof(color_t));
		memset(bg_buffer, BG_GS_1, height * sizeof(color_t));
	}
}


static void draw_logo(
		int tick, uint32_t width, uint32_t height, uint8_t* frame, const color_t* bg_buffer,
		const png_struct* png_ptr, const png_info* info_ptr
) {
	const uint32_t img_w = u32min(width, png_get_image_width(png_ptr, info_ptr));
	const uint32_t img_h = u32min(height, png_get_image_height(png_ptr, info_ptr));
	const uint32_t start_x = (width - img_w) / 2;
	const uint32_t start_y = (height - img_h) / 2;
	const uint32_t end_x = start_x + img_w;
	const uint32_t end_y = start_y + img_h;

	const float noise = get_noise(tick);

	const uint32_t max_line_h    = img_h / 4;
	const uint32_t max_off_x     = img_w * noise * 0.2f;
	const uint32_t min_off_color = img_w * noise * 0.05f;
	const uint32_t max_off_color = img_w * noise * 0.1f;

	uint32_t **img_rows = (uint32_t**) png_get_rows(png_ptr, info_ptr);

	for (uint32_t sy = start_y; sy < end_y; ) {
		const int32_t off_x = ((float) rand() / RAND_MAX) >= noise ? 0 : randrange(-max_off_x, max_off_x);
		const int32_t off_color = randrange(min_off_color, max_off_color) * 4;

		const uint32_t line_h = u32min(end_y - sy, randrange(4, max_line_h));

		const int dx = off_x > 0 ? -1 : 1;
		const uint32_t sx = off_x > 0 ? i32min(width, end_x + max_off_x) : i32max(0, start_x - max_off_x);
		const uint32_t ex = (off_x > 0 ? i32max(0, start_x - max_off_x) : i32min(width, end_x + max_off_x)) + dx;

		for (uint32_t y = sy; y < sy + line_h; y++) {
			for (uint32_t x = sx; x != ex; x += dx) {
				int32_t tx = x + off_x;

				if (tx >= 0 && tx < width) {
					int32_t u = x - start_x;
					int32_t v = y - start_y;
					assert(v >= 0 && v <= img_h);

					color_t color = u >= 0 && u < img_w ? mix(bg_buffer[y], img_rows[v][u]) : bg_buffer[y];
					uint32_t index = (y * width + tx) * 4;

					frame[index + 2 - off_color] = (uint8_t)(color >> 16); // R
					frame[index + 1]             = (uint8_t)(color >> 8);  // G
					frame[index + 0 + off_color] = (uint8_t)(color);       // B
				}
			}
		}

		sy += line_h;
	}
}


void draw(int tick, uint32_t width, uint32_t height, uint8_t *frame, color_t* bg_buffer, const png_struct* png_ptr, const png_info* info_ptr) {
	draw_bg(tick, width, height, frame, bg_buffer);
	draw_logo(tick, width, height, frame, bg_buffer, png_ptr, info_ptr);
}