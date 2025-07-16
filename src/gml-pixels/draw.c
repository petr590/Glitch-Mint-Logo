/**
 * Файл отвечает за рендеринг изображения в буфер
 */

#include "module.h"
#include "../util/random.h"
#include "../util/util.h"

#ifndef __USE_MISC
#define __USE_MISC // Для того, чтобы VS Code увидел константы M_PI и др.
#endif
#include <math.h>
#include <stdbool.h>
#include <string.h>

#include <assert.h>

#define BACKGROUND_GS 0x33
#define BACKGROUND (BACKGROUND_GS << 16 | BACKGROUND_GS << 8 | BACKGROUND_GS)
#define FOREGROUND 0xCCCCCC
#define SHADOW     0x020202
#define SHADOW_WIDTH 2

#define RAD(deg) ((deg) * (float)(M_PI / 180))
#define ANGLE_PER_TICK RAD(0.5f)
#define FADE_ANGLE RAD(20)

/** @return Псевдорандомный хэш от координат x, y с хорошим распределением */
static uint32_t hash(uint32_t x, uint32_t y) {
	uint32_t hash = x * 374761393 + y * 668265263;
	hash = (hash ^ (hash >> 13)) * 1274126177;
	hash = hash ^ (hash >> 16);
	return hash;
}

/** @return true с шансом chance */
static bool hash_chance(uint32_t hash, float chance) {
	return (float) hash / 0xFFFFFFFFu < chance;
}


static float brightness(color_t color) {
	return  (0.299f / 255) * ((color >> 16) & 0xFF) +
			(0.587f / 255) * ((color >>  8) & 0xFF) +
			(0.114f / 255) * ((color      ) & 0xFF);
}


static bool is_set(
	uint32_t x, uint32_t y, uint32_t width, uint32_t height,
	float max_angle, float min_tg, float max_tg
) {
	float tg = (float)y / (2*width - x);

	if (tg > min_tg && tg < max_tg) {
		float diff_angle = max_angle - atanf(tg);

		if (hash_chance(hash(x, y), 1 - diff_angle / FADE_ANGLE)) {
			return true;
		}
	}

	uint32_t img_w = png_get_image_width(png_ptr, info_ptr);
	uint32_t img_h = png_get_image_height(png_ptr, info_ptr);

	if (tg < max_tg &&
		abs(width - 2*x) < img_w &&
		abs(height - 2*y) < img_h) {
		
		color_t** img_rows = (color_t**) png_get_rows(png_ptr, info_ptr);
		uint32_t u = x - (width - img_w) / 2;
		uint32_t v = y - (height - img_h) / 2;

		assert(u < img_w);
		assert(v < img_h);

		return brightness(mix(BACKGROUND, img_rows[v][u])) >= 0.5f;
	}

	return false;
}


static void fill_buffer(int tick, uint32_t width, uint32_t height) {
	const float max_angle = fminf(M_PI_2 + FADE_ANGLE, tick * ANGLE_PER_TICK);
	const float min_angle = fmaxf(0, max_angle - FADE_ANGLE);

	const float min_tg = min_angle >= M_PI_2 ? INFINITY : tanf(min_angle);
	const float max_tg = max_angle >= M_PI_2 ? INFINITY : tanf(max_angle);

	bitset2d_clear(&buffer);

	const uint32_t w = width / PIXEL_SIZE;
	const uint32_t h = height / PIXEL_SIZE;

	for (uint32_t y = 0; y < h; y++) {
		for (uint32_t x = 0; x < w; x++) {

			if (is_set(x, y, w, h, max_angle, min_tg, max_tg)) {
				bitset2d_set_1(&buffer, x, y);
			}
		}
	}
}

static int buffer_get_scaled(uint32_t x, uint32_t y, uint32_t width) {
	return bitset2d_get(&buffer, x / PIXEL_SIZE, y / PIXEL_SIZE);
}

void gml_draw(int tick, uint32_t width, uint32_t height, color_t* frame) {
	fill_buffer(tick, width, height);

	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {

			if (buffer_get_scaled(x, y, width)) {
				frame[y * width + x] = FOREGROUND;
				continue;
			}

			bool left_shadow = x >= SHADOW_WIDTH && buffer_get_scaled(x - SHADOW_WIDTH, y, width);
			bool top_shadow  = y >= SHADOW_WIDTH && buffer_get_scaled(x, y - SHADOW_WIDTH, width);

			if (left_shadow || top_shadow) {
				frame[y * width + x] = SHADOW;
				continue;
			}

			frame[y * width + x] = BACKGROUND;
		}
	}
}