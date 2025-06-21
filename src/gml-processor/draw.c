/**
 * Файл отвечает за рендеринг изображения в буфер
 */

#include "module.h"
#include "../util/random.h"
#include "../util/util.h"
#include <string.h>
#include <math.h>

#include <assert.h>

#define BACKGROUND_GS  0x44
#define LINE_COLOR_OFF 0x25201D
#define LINE_COLOR_ON  0xf1f5d2

#define LINE_WIDTH 2
#define SIDE       20

#define TURN_CHANCE   0.4f
#define SPLLIT_CHANCE 0.2f

#define MAX_TRACKS_SIZE 200

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


static const vec2i8 UP    = { .x = -1, .y =  0 };
static const vec2i8 DOWN  = { .x =  1, .y =  0 };
static const vec2i8 LEFT  = { .x =  0, .y = -1 };
static const vec2i8 RIGHT = { .x =  0, .y =  1 };

static track_t tracks[MAX_TRACKS_SIZE];
static uint32_t tracks_size;


static int is_in_bounds(vec2i pos, uint32_t w, uint32_t h) {
	return  pos.x >= 0 && pos.x < w &&
			pos.y >= 0 && pos.y < h;
}

static int is_out_of_bounds(vec2i pos, uint32_t w, uint32_t h) {
	return !is_in_bounds(pos, w, h);
}


static void init_tracks(int tick, uint32_t w, uint32_t h) {
	const int32_t sx = (w - SIDE + 1) / 2;
	const int32_t sy = (h - SIDE + 1) / 2;

	for (int i = 0; i < SIDE; i++) {
		track_t* track_ptr = tracks + i*4;
		
		track_ptr[0].pos.x = sx + i;
		track_ptr[0].pos.y = sy - 1;
		track_ptr[0].prev_offset = UP;
		
		track_ptr[1].pos.x = sx + i;
		track_ptr[1].pos.y = sy + SIDE;
		track_ptr[1].prev_offset = DOWN;

		track_ptr[2].pos.x = sx - 1;
		track_ptr[2].pos.y = sy + i;
		track_ptr[2].prev_offset = LEFT;

		track_ptr[3].pos.x = sx + SIDE;
		track_ptr[3].pos.y = sy + i;
		track_ptr[3].prev_offset = RIGHT;
	}

	tracks_size = SIDE * 4;

	#ifndef NDEBUG // Проверяем, что все координаты разные
	for (int i = 0; i < tracks_size; i++) {
		for (int j = i + 1; j < tracks_size; j++) {
			assert( tracks[i].pos.x != tracks[j].pos.x ||
					tracks[i].pos.y != tracks[j].pos.y);
		}
	}
	#endif

	for (int dy = 0; dy < SIDE + 2; dy++) {
		for (int dx = 0; dx < SIDE + 2; dx++) {
			bitset2d_set_1(&p_bg_buffer, sx - 1 + dx, sy - 1 + dy);
		}
	}

	bitset2d_set_0(&p_bg_buffer, sx - 1,    sy - 1);
	bitset2d_set_0(&p_bg_buffer, sx + SIDE, sy - 1);
	bitset2d_set_0(&p_bg_buffer, sx - 1,    sy + SIDE);
	bitset2d_set_0(&p_bg_buffer, sx + SIDE, sy + SIDE);


	const uint32_t cx = (w + 1) / 2;
	const uint32_t cy = (h + 1) / 2;

	for (int i = 0; i < SIDE / 2; i++) {
		for (int j = 0; j <= i; j++) {
			bitset2d_set_1(&v_bg_buffer, cx     + i, cy - 1 + j);
			bitset2d_set_1(&v_bg_buffer, cx     + i, cy - 1 - j);
			bitset2d_set_1(&v_bg_buffer, cx - 1 - i, cy - 1 + j);
			bitset2d_set_1(&v_bg_buffer, cx - 1 - i, cy - 1 - j);
			
			bitset2d_set_1(&h_bg_buffer, cx - 1 + j, cy     + i);
			bitset2d_set_1(&h_bg_buffer, cx - 1 - j, cy     + i);
			bitset2d_set_1(&h_bg_buffer, cx - 1 + j, cy - 1 - i);
			bitset2d_set_1(&h_bg_buffer, cx - 1 - j, cy - 1 - i);
		}
	}
}


static void add_line(int index, vec2i pos, vec2i8 offset) {
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
		bitset2d_set_1(&h_bg_buffer, u32min(pos.x, new_pos.x), pos.y);
	} else {
		bitset2d_set_1(&v_bg_buffer, pos.x, u32min(pos.y, new_pos.y));
	}
}


static int find_empty_track(uint32_t w, uint32_t h) {
	for (int i = 0; i < tracks_size; i++) {
		if (is_out_of_bounds(tracks[i].pos, w, h)) {
			return i;
		}
	}

	return -1;
}

static void update_bg_buffers(int tick, uint32_t width, uint32_t height) {
	const uint32_t w = width / PIXEL_SIZE;
	const uint32_t h = height / PIXEL_SIZE;

	if (tick == 0) {
		init_tracks(tick, w, h);
	}

	const uint32_t tracks_size_local = tracks_size;

	for (uint32_t i = 0; i < tracks_size_local; i++) {
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

		if (pos.x > 0     && !bitset2d_get(&p_bg_buffer, pos.x - 1, pos.y)) offsets[offsets_size++] = UP;
		if (pos.y > 0     && !bitset2d_get(&p_bg_buffer, pos.x, pos.y - 1)) offsets[offsets_size++] = LEFT;
		if (pos.x + 1 < w && !bitset2d_get(&p_bg_buffer, pos.x + 1, pos.y)) offsets[offsets_size++] = DOWN;
		if (pos.y + 1 < h && !bitset2d_get(&p_bg_buffer, pos.x, pos.y + 1)) offsets[offsets_size++] = RIGHT;

		if (offsets_size >= 1) {
			int j = rand() % offsets_size;
			add_line(i, pos, offsets[j]);

			if (offsets_size >= 2 && chance(SPLLIT_CHANCE)) {
				j = (j + 1) % offsets_size;

				int new_pos_index = -1;

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


static int bitset2d_get_scaled(const bitset2d* bitset, uint32_t x, uint32_t y) {
	return bitset2d_get(bitset, x / PIXEL_SIZE, y / PIXEL_SIZE);
}

void gml_draw(int tick, uint32_t width, uint32_t height, color_t* frame) {
	update_bg_buffers(tick, width, height);
	memset(frame, BACKGROUND_GS, width * height * sizeof(color_t));

	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			int left_line = x % PIXEL_SIZE < LINE_WIDTH;
			int top_line  = y % PIXEL_SIZE < LINE_WIDTH;

			if (left_line || top_line) {
				if (left_line && bitset2d_get_scaled(&v_bg_buffer, x, y) ||
					top_line && bitset2d_get_scaled(&h_bg_buffer, x, y) ||
					left_line && top_line && bitset2d_get_scaled(&p_bg_buffer, x, y)) {
					
					frame[y * width + x] = LINE_COLOR_ON;
					continue;
				}

				frame[y * width + x] = LINE_COLOR_OFF;
			}
		}
	}
}