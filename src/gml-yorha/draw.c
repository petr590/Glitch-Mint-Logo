/**
 * Файл отвечает за рендеринг изображения в буфер
 */

#include "module.h"
#include "util/util.h"
#include "util/random.h"
#include "util/render_glyph.h"
#include <string.h>
#include <math.h>
#include <assert.h>

#define BACKGROUND     0x353535
#define LINE_COLOR_OFF 0x25201d
#define LINE_COLOR_ON  0x57524e
#define LOGO_COLOR     0x888478
#define TEXT_COLOR     0xf4efb7

#define SIDE 10 // Начальная сторона центрального квадрата

#define TURN_CHANCE  0.4f // Шанс того, что линия повернёт
#define SPLIT_CHANCE 0.2f // Шанс того, что линия разделится на две

#define MAX_TRACKS_SIZE 200 // Максимальное количество треков на экране


// Координаты начала текста на экране
#define RUNNING_STRS_X 50
#define RUNNING_STRS_Y 80

#define FPS_PER_CHAR 1 // За сколько fps выводится 1 символ. Целое число

// Коэффициент для рассчёта предполагаемого времени рендера. Из-за случайного направления линий
// невозможно точно посчитать максимальную длину линии и, соответственно, время окончания рендера.
// Коэффициент рассчитан на практике, для ращрешения 1920x1080. На других разрешениях, в теории,
// может отличаться. Однако, погрешности здесь допустимы.
#define SUPPOSED_RENDER_TIME_MULTIPLIER 1.3


typedef struct {
	int32_t x, y;
} vec2i;

typedef struct {
	int8_t x, y;
} vec2i8;

typedef struct {
	vec2i pos;
	vec2i8 prev_offset;
} track_t;


static const vec2i8 UP    = { .x =  0, .y =  1 };
static const vec2i8 DOWN  = { .x =  0, .y = -1 };
static const vec2i8 LEFT  = { .x =  1, .y =  0 };
static const vec2i8 RIGHT = { .x = -1, .y =  0 };

static track_t tracks[MAX_TRACKS_SIZE];
static uint16_t tracks_size;


static int is_in_bounds(vec2i pos, int32_t w, int32_t h) {
	return  pos.x >= 0 && pos.x < w &&
			pos.y >= 0 && pos.y < h;
}

static int is_out_of_bounds(vec2i pos, int32_t w, int32_t h) {
	return !is_in_bounds(pos, w, h);
}


void init_tracks() {
	for (uint16_t i = 0; i < SIDE; i++) {
		track_t* track_ptr = tracks + i*2;
		
		track_ptr[0].pos.x = i;
		track_ptr[0].pos.y = SIDE;
		track_ptr[0].prev_offset = UP;

		track_ptr[1].pos.x = SIDE;
		track_ptr[1].pos.y = i;
		track_ptr[1].prev_offset = LEFT;

		bitset2d_set_1(&p_bg_buffer, i, SIDE);
		bitset2d_set_1(&p_bg_buffer, SIDE, i);
	}

	tracks_size = SIDE * 2;

	#ifndef NDEBUG // Проверяем, что все координаты разные
	for (uint16_t i = 0; i < tracks_size; i++) {
		for (uint16_t j = i + 1; j < tracks_size; j++) {
			assert( tracks[i].pos.x != tracks[j].pos.x ||
					tracks[i].pos.y != tracks[j].pos.y);
		}
	}
	#endif

	for (uint16_t dy = 0; dy < SIDE; dy++) {
		for (uint16_t dx = 0; dx < SIDE; dx++) {
			bitset2d_set_1(&p_bg_buffer, dx, dy);
		}
	}

	for (uint16_t i = 0; i < SIDE; i++) {
		for (uint16_t j = 0; j < i; j++) {
			bitset2d_set_1(&v_bg_buffer, i, j);
			bitset2d_set_1(&h_bg_buffer, j, i);
		}
	}
}


static void add_line(int32_t index, vec2i pos, vec2i8 offset) {
	assert(index >= 0 && index < MAX_TRACKS_SIZE);

	assert(
		((offset.x == 1 || offset.x == -1) && offset.y == 0) ||
		((offset.y == 1 || offset.y == -1) && offset.x == 0)
	);

	vec2i new_pos = {
		.x = pos.x + offset.x,
		.y = pos.y + offset.y,
	};

	tracks[index].pos = new_pos;
	tracks[index].prev_offset = offset;

	bitset2d_set_1(&p_bg_buffer, new_pos.x, new_pos.y);

	if (pos.x != new_pos.x) {
		bitset2d_set_1(&h_bg_buffer, i32min(pos.x, new_pos.x), pos.y);
	} else {
		bitset2d_set_1(&v_bg_buffer, pos.x, i32min(pos.y, new_pos.y));
	}
}


static int32_t find_empty_track(int32_t w, int32_t h) {
	for (uint16_t i = 0; i < tracks_size; i++) {
		if (is_out_of_bounds(tracks[i].pos, w, h)) {
			return i;
		}
	}

	return -1;
}

static void update_bg_buffers(uint16_t width, uint16_t height) {
	const int32_t w = i32_div_ceil(i32_div_ceil(width  + LINE_WIDTH / 2, 2), CELL_SIZE);
	const int32_t h = i32_div_ceil(i32_div_ceil(height + LINE_WIDTH / 2, 2), CELL_SIZE);

	const uint16_t tracks_size_local = tracks_size;

	for (uint16_t i = 0; i < tracks_size_local; i++) {
		vec2i pos = tracks[i].pos;

		if (is_out_of_bounds(pos, w, h)) {
			continue;
		}

		const vec2i8 offset = tracks[i].prev_offset;
		const vec2i next_pos = {
			pos.x + offset.x,
			pos.y + offset.y,
		};

		if (is_in_bounds(next_pos, w, h) && !bitset2d_get(&p_bg_buffer, next_pos.x, next_pos.y) && !chance(TURN_CHANCE)) {
			add_line(i, pos, offset);
			continue;
		}

		vec2i8 offsets[4];
		int offsets_size = 0;

		if (pos.x > 0     && !bitset2d_get(&p_bg_buffer, pos.x - 1, pos.y)) offsets[offsets_size++] = RIGHT;
		if (pos.y > 0     && !bitset2d_get(&p_bg_buffer, pos.x, pos.y - 1)) offsets[offsets_size++] = DOWN;
		if (pos.x + 1 < w && !bitset2d_get(&p_bg_buffer, pos.x + 1, pos.y)) offsets[offsets_size++] = LEFT;
		if (pos.y + 1 < h && !bitset2d_get(&p_bg_buffer, pos.x, pos.y + 1)) offsets[offsets_size++] = UP;

		if (offsets_size >= 1) {
			int j = rand() % offsets_size;
			add_line(i, pos, offsets[j]);

			if (offsets_size >= 2 && chance(SPLIT_CHANCE)) {
				j = (j + 1) % offsets_size;

				int32_t new_pos_index = -1;

				if (tracks_size < MAX_TRACKS_SIZE - 1) {
					new_pos_index = tracks_size++;
				} else {
					new_pos_index = find_empty_track(w, h);
				}

				if (new_pos_index >= 0) {
					add_line(new_pos_index, pos, offsets[j]);
				}
			}

		} else {
			tracks[i].pos.x = -1;
			tracks[i].pos.y = -1;
		}
	}
}


static int32_t draw_char(wchar_t ch, int32_t sx, int32_t baseline, uint16_t width, uint16_t height, color_t* frame) {
	const glyph_t* glyph = render_glyph(ch, face);

	const uint16_t glyph_w = glyph->width;
	const uint16_t glyph_h = glyph->height;
	const int32_t sy = baseline - glyph->top;
	
	for (uint16_t y = 0; y < glyph_h; y++) {
		for (uint16_t x = 0; x < glyph_w; x++) {
			size_t index = (sy + y) * width + sx + x + glyph->left;
			uint8_t alpha = glyph->buffer[y * glyph_w + x];
			
			if (index < width * height) {
				frame[index] = mix(frame[index], ((color_t)alpha << 24) | TEXT_COLOR);
			}
		}
	}

	return glyph->advance_x;
}


static void draw_running_str(uint16_t index, int tick, uint16_t width, uint16_t height, color_t* frame) {
	const wchar_t* str = running_strings[index].str;
	const int len = running_strings[index].printed;

	const int32_t baseline = RUNNING_STRS_Y + (index + 1) * STRING_HEIGHT;
	int32_t sx = RUNNING_STRS_X;

	for (int i = 0; i < len; i++) {
		sx += draw_char(str[i], sx, baseline, width, height, frame);
	}

	if (str[len] == '\0') {
		return;
	}

	draw_char(str[0], sx, baseline, width, height, frame);

	if (tick % FPS_PER_CHAR == 0) {
		running_strings[index].printed += 1;
	}
}


static int get_scaled(const bitset2d* bitset, int32_t x, int32_t y) {
	return bitset2d_get(bitset, x / CELL_SIZE, y / CELL_SIZE);
}

static void draw_bg_and_logo(uint16_t width, uint16_t height, color_t* frame) {
	const int32_t img_w = (int32_t) png_get_image_width(png_ptr, info_ptr);
	const int32_t img_h = (int32_t) png_get_image_height(png_ptr, info_ptr);
	const color_t** img_data = (const color_t**) png_get_rows(png_ptr, info_ptr);

	const int32_t sx = (width  - img_w) / 2;
	const int32_t sy = (height - img_h) / 2;
	const int32_t ex = (width  + img_w) / 2;
	const int32_t ey = (height + img_h) / 2;

	const int32_t cx = (width + 1) / 2;
	const int32_t cy = (height + 1) / 2;

	for (int32_t y = 0; y < height; y++) {
		for (int32_t x = 0; x < width; x++) {

			color_t* res = &frame[y * width + x];

			int32_t sym_x = (x < cx ? cx - x - 1 : x - cx) + LINE_WIDTH / 2;
			int32_t sym_y = (y < cy ? cy - y - 1 : y - cy) + LINE_WIDTH / 2;

			int left_line = sym_x % CELL_SIZE < LINE_WIDTH;
			int top_line  = sym_y % CELL_SIZE < LINE_WIDTH;

			if (left_line || top_line) {
				if ((left_line             && get_scaled(&v_bg_buffer, sym_x, sym_y)) ||
					(top_line              && get_scaled(&h_bg_buffer, sym_x, sym_y)) ||
					(left_line && top_line && get_scaled(&p_bg_buffer, sym_x, sym_y))) {
					
					*res = LINE_COLOR_ON;
				} else {
					*res = LINE_COLOR_OFF;
				}

			} else {
				*res = BACKGROUND;
			}

			if (x >= sx && x < ex && y >= sy && y < ey) {
				*res = mix(*res, (img_data[y - sy][x - sx] & 0xFF000000) | LOGO_COLOR);
			}
		}
	}
}


static double get_target_speed(uint16_t width, uint16_t height, double supposed_time) {
	if (supposed_time <= 0.0 || !isfinite(supposed_time))
		return 1.0;

	const int32_t distance_to_corner = i32_div_ceil(width, 2 * CELL_SIZE) + i32_div_ceil(height, 2 * CELL_SIZE) - SIDE / 2;
	return fclamp(SUPPOSED_RENDER_TIME_MULTIPLIER * distance_to_corner / supposed_time, 1.0, 2.0);
}


void gml_draw(int tick, uint16_t width, uint16_t height, color_t* frame, double supposed_time) {
	static int updates = 0;

	update_bg_buffers(width, height);
	updates += 1;

	const double speed = (double) updates / (tick + 1);
	const double target_speed = get_target_speed(width, height, supposed_time);

	if (speed < target_speed) {
		update_bg_buffers(width, height);
		updates += 1;
	}

	draw_bg_and_logo(width, height, frame);

	read_from_socket();

	for (uint16_t i = 0; i < running_strings_len; i++) {
		draw_running_str(i, tick, width, height, frame);
	}
}
