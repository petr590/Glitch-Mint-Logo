#include "module.h"
#include "../util/render_glyph.h"
#include "../util/random.h"
#include "../util/util.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <assert.h>

#define BG_GS_1 0x1A
#define BG_GS_2 0x19

/*
 * Все макросы с _DURATION и _PERIOD объявляют время в тактах,
 * количество тактов в каждой секунде равно FPS
 */

#define BG_GLITCH_DURATION 4
#define BG_GLITCH_PERIOD 10

#define START_GLITCH_DURATON 10
#define MAJOR_GLITCH_DURATON 2
#define MAJOR_GLITCH_PERIOD 15

#define MINOR_GLITCH_DURATON 5
#define MINOR_GLITCH_PERIOD 20
#define MINOR_GLITCH_AMPLITUDE 4
#define MINOR_GLITCH_CHANCE 0.2f

#define TEXT_COLOR 0x87CF3E
#define TEXT_PADDING 10

/** Возвращает рандомное число между from и to включительно или from, если to < from.
 * Для генерации числа использует переданный сид, умноженный на RANDOM_CONSTANT. */
static inline int randrange_seed(int from, int to, int seed) {
	int old_seed = rand();
	srand(seed * RANDOM_CONSTANT);
	int res = randrange(from, to);
	srand(old_seed);
	return res;
}

/** Возвращает `(tick + offset) % period`, где offset - рандомное смещение,
 * которое, однако, зависит только от tick, period и RANDOM_CONSTANT.
 * Таким образом, одинаковые входные данные функции дают одинаковый результат (в одном запуске программы). */
static inline int tick_remainder(int tick, int period) {
	return (tick + randrange_seed(0, period - 1, tick / period)) % period;
}


static float get_major_noise(int tick) {
	if (tick < START_GLITCH_DURATON) {
		return (float)(START_GLITCH_DURATON - tick) * (1.f / START_GLITCH_DURATON);
	}

	return tick_remainder(tick, MAJOR_GLITCH_PERIOD) < MAJOR_GLITCH_DURATON ? 0.5f : 0.0f;
}


static float get_minor_noise(int tick) {
	int rem = tick_remainder(tick, MINOR_GLITCH_PERIOD);
	if (rem >= MINOR_GLITCH_DURATON) return 0;
	return rem <= MINOR_GLITCH_DURATON / 2 ? rem : MINOR_GLITCH_DURATON - 1 - rem;
}


static void draw_bg(int tick, uint32_t width, uint32_t height, uint8_t* frame) {

	if (tick % 4 < 2 && tick_remainder(tick, BG_GLITCH_PERIOD) < BG_GLITCH_DURATION) {
		uint32_t min_h = height / 20;
		uint32_t max_h = height / 5;

		int dark = 0;
		for (uint32_t y = 0; y < height; dark ^= 1) {
			uint32_t line_h = u32min(height - y, randrange(min_h, max_h));
			uint8_t gs = dark ? BG_GS_2 : BG_GS_1;

			memset(frame + y * width * sizeof(color_t), gs, line_h * width * sizeof(color_t));
			memset(bg_buffer + y, gs, line_h * sizeof(color_t));

			y += line_h;
		}

	} else {
		memset(frame, BG_GS_1, width * height * sizeof(color_t));
		memset(bg_buffer, BG_GS_1, height * sizeof(color_t));
	}
}


static void draw_logo(int tick, uint32_t width, uint32_t height, uint8_t* frame) {

	const uint32_t img_w = u32min(width, png_get_image_width(png_ptr, info_ptr));
	const uint32_t img_h = u32min(height, png_get_image_height(png_ptr, info_ptr));
	const uint32_t start_x = (width - img_w) / 2;
	const uint32_t start_y = (height - img_h) / 2;
	const uint32_t end_x = start_x + img_w;
	const uint32_t end_y = start_y + img_h;

	const float major_noise = get_major_noise(tick);
	const float minor_noise = get_minor_noise(tick);

	const uint32_t max_line_h    = img_h / 4;
	const uint32_t max_off_x     = img_w * major_noise * 0.2f;
	const uint32_t min_off_color = img_w * major_noise * 0.05f;
	const uint32_t max_off_color = img_w * major_noise * 0.1f;

	color_t **img_rows = (color_t**) png_get_rows(png_ptr, info_ptr);

	for (uint32_t sy = start_y; sy < end_y; ) {
		const int32_t off_x = chance(major_noise) ? randrange(-max_off_x, max_off_x) : 0;
		const int32_t off_color = randrange(min_off_color, max_off_color) * 4;

		const uint32_t ey = sy + u32min(end_y - sy, randrange(4, max_line_h));
		const uint32_t sx = i32max(0, start_x - max_off_x);
		const uint32_t ex = u32min(width, end_x + max_off_x);

		const uint32_t minor_y = randrange(sy + MINOR_GLITCH_AMPLITUDE, ey - MINOR_GLITCH_AMPLITUDE);
		const int amplitude = MINOR_GLITCH_AMPLITUDE * minor_noise;
		const int sign = chance(MINOR_GLITCH_CHANCE) ? randchoose(-1, 1) : 0;


		for (uint32_t y = sy; y < ey; y++) {
			for (uint32_t x = sx; x <= ex; x++) {
				int32_t tx = x + off_x + i32max(0, amplitude - abs(y - minor_y)) * sign;

				if (tx >= 0 && tx < width) {
					int32_t u = x - start_x;
					int32_t v = y - start_y;
					assert(v >= 0 && v <= img_h);

					color_t color = u >= 0 && u < img_w ? mix(bg_buffer[y], img_rows[v][u]) : bg_buffer[y];
					uint32_t index = (y * width + tx) * sizeof(color_t);

					frame[index + 2 - off_color] = (uint8_t)(color >> 16); // R
					frame[index + 1]             = (uint8_t)(color >> 8);  // G
					frame[index + 0 + off_color] = (uint8_t)(color);       // B
				}
			}
		}

		sy = ey;
	}
}


static void draw_system_name(int tick, uint32_t width, uint32_t height, color_t* frame) {
	const uint32_t img_height = png_get_image_height(png_ptr, info_ptr);
	const uint32_t namelen = strlen(system_name);
	const uint32_t len = u32min(namelen, tick + 1);
	char text_buf[len];
	memcpy(text_buf, system_name, len);

	if (len < namelen && text_buf[len - 1] != ' ') {
		text_buf[len - 1] = text_buf[0];
	}

	uint32_t str_width = 0;

	for (uint32_t i = 0; i < len; i++) {
		const char ch = text_buf[i];
		if (ch < CHAR_START || ch >= CHAR_END) continue;
		
		str_width += render_glyph(ch, face)->advance_x;
	}

	uint32_t sx = (width - str_width) / 2;
	uint32_t baseline = (height + img_height) / 2 + TEXT_PADDING + GLYPH_HEIGHT;

	const uint32_t minor_y = randrange(0, GLYPH_HEIGHT);
	const int amplitude = MINOR_GLITCH_AMPLITUDE * get_minor_noise(tick);
	const int sign = chance(MINOR_GLITCH_CHANCE) ? randchoose(-1, 1) : 0;

	for (uint32_t i = 0; i < len; i++) {
		const char ch = text_buf[i];
		const glyph_t* glyph = render_glyph(ch, face);

		const uint8_t* buffer  = glyph->buffer;
		const uint32_t glyph_w = glyph->width;
		const uint32_t glyph_h = glyph->height;

		for (uint32_t y = 0; y < glyph_h; y++) {
			for (uint32_t x = 0; x < glyph_w; x++) {
				int32_t tx = x + sx + glyph->left + i32max(0, amplitude - abs(y - minor_y)) * sign;

				if (tx >= 0 && tx < width) {
					uint8_t alpha = buffer[y * glyph_w + x];
					frame[(baseline - glyph->top + y) * width + tx] = mix(bg_buffer[y], alpha << 24 | TEXT_COLOR);
				}
			}
		}

		sx += glyph->advance_x;
	}
}


void gml_draw(int tick, uint32_t width, uint32_t height, color_t* frame) {
	draw_bg(tick, width, height, (uint8_t*) frame);
	draw_logo(tick, width, height, (uint8_t*) frame);

	if (face != NULL) {
		draw_system_name(tick, width, height, frame);
	}
}
