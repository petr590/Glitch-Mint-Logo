#include "module.h"
#include "../util/render_glyph.h"
#include "../util/random.h"
#include "../util/util.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <assert.h>

#define BACKGROUND_GS 0x1A
#define BACKGROUND (BACKGROUND_GS << 16 | BACKGROUND_GS << 8 | BACKGROUND_GS)
#define FOREGROUND 0x82A4F0
#define OFFSET_Y 8
#define TOGGLE_LINE_CHANCE 0.02f
#define RAND_CHAR_CHANCE 0.25f

static inline char random_char(void) {
	return randrange('\x21', '\x7E');
}

void gml_draw(int tick, uint32_t width, uint32_t height, color_t* frame) {
	memset(frame, BACKGROUND_GS, width * height * sizeof(color_t));

	static int offset_y = 0;

	for (int y = 0; y < text_h; y++) {
		for (int x = 0; x < text_w; x++) {
			char ch = text_buffer[y * text_w + x];

			if (ch != ' ') {
				const glyph_t* glyph = render_glyph(ch, face);

				const int dst_sx = x * CHAR_WIDTH + glyph->left;
				const int dst_sy = y * CHAR_HEIGHT - glyph->top + offset_y;
				const int dst_w = u32min(CHAR_WIDTH, glyph->width);
				const int dst_h = u32min(CHAR_HEIGHT, glyph->height);

				const int dy_start = -i32min(0, dst_sy);

				for (int dy = dy_start; dy < dst_h && dst_sy + dy < height; dy++) {
					for (int dx = 0; dx < dst_w && dst_sx + dx < width; dx++) {
						uint8_t alpha = glyph->buffer[dy * glyph->width + dx];

						if (alpha > 0) {
							frame[(dst_sy + dy) * width + dst_sx + dx] = mix(BACKGROUND, alpha << 24 | FOREGROUND);
						}
					}
				}
			}
		}
	}

	for (int y = 0; y < text_h; y++) {
		for (int x = 0; x < text_w; x++) {
			int index = y * text_w + x;

			if (text_buffer[index] != ' ' && chance(RAND_CHAR_CHANCE)) {
				text_buffer[index] = random_char();
			}
		}
	}

	offset_y += OFFSET_Y;
	
	if (offset_y > CHAR_HEIGHT) {
		offset_y -= CHAR_HEIGHT;

		for (int y = text_h - 1; y > 0; y--) {
			memcpy(
				&text_buffer[y * text_w],
				&text_buffer[(y - 1) * text_w],
				text_w * sizeof(char)
			);
		}

		for (int x = 0; x < text_w; x++) {
			int is_space = text_buffer[x] == ' ';

			if (chance(TOGGLE_LINE_CHANCE)) {
				text_buffer[x] = is_space ? random_char() : ' ';
			} else if (!is_space) {
				text_buffer[x] = random_char();
			}
		}
	}
}