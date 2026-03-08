/**
 * Файл отвечает за рендеринг изображения в буфер
 */

#include "module.h"
#include "3d_math_inline.h"
#include "../util/util.h"
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define UNUSED(v) (void)(v)

#define BACKGROUND_GS 0x1A

static bool is_point_in_polygon(vec2 p, vec2 a, vec2 b, vec2 c) {
    const float d0 = cross2(sub2(b, a), sub2(p, a));
    const float d1 = cross2(sub2(c, b), sub2(p, b));
    const float d2 = cross2(sub2(a, c), sub2(p, c));

    return  (d0 >= 0 && d1 >= 0 && d2 >= 0) ||
			(d0 <= 0 && d1 <= 0 && d2 <= 0);
}


#ifndef NDEBUG
static void asserts(void) {
	const vec2 a = VEC2(0, 100);
	const vec2 b = VEC2(100, 0);
	const vec2 c = VEC2(-100, 0);

	assert(is_point_in_polygon(VEC2(0, 0), a, b, c));
	assert(is_point_in_polygon(VEC2(0, 50), a, b, c));
	assert(is_point_in_polygon(a, a, b, c));
	assert(is_point_in_polygon(b, a, b, c));
	assert(is_point_in_polygon(c, a, b, c));
	assert(is_point_in_polygon(VEC2(50, 50), a, b, c));
	assert(!is_point_in_polygon(VEC2(50, 50.01), a, b, c));
	assert(!is_point_in_polygon(VEC2(0, 100.01), a, b, c));
	assert(!is_point_in_polygon(VEC2(100.01, 0), a, b, c));
	assert(!is_point_in_polygon(VEC2(-100.01, 0), a, b, c));
}
#else
#define asserts(...)
#endif


void gml_draw(int tick, uint16_t width, uint16_t height, color_t* frame, double supposed_time) {
	UNUSED(tick);
	UNUSED(supposed_time);

	if (tick == 0) {
		asserts();
	}

	memset(frame, BACKGROUND_GS, width * height * sizeof(color_t));
	memset(depth_buffer, 0, width * height * sizeof(float));

	mat4 model = MAT4(1);
	model = translate(model, VEC3(300, 0, 0));
	// model = rotate(model, M_PI / 2 * tick / fps, normalize3(VEC3(1, 1, 0)));
	mat4 proj = projection((float)(M_PI / 2), (float) width / height, 1.0f, 100.0f);

	mat4 mat = cross_mat4_mat4(proj, model);

	vec2 low = VEC2(INFINITY, INFINITY);
	vec2 high = VEC2(-INFINITY, -INFINITY);

	static bool printed = false;

	for (size_t i = 0; i < vertex_buffer_len; i++) {
		const vec4 vertex = cross_mat4_vec3(mat, vertex_buffer[i]);
		transformed_vertex_buffer[i] = vertex;
		low = min2(low, VEC2(vertex.x, vertex.y));
		high = max2(high, VEC2(vertex.x, vertex.y));
	}


	const size_t ibuf_len = index_buffer_len / 3 * 3;
	const uint16_t cx = width / 2;
	const uint16_t cy = height / 2;

	const uint32_t start_x = (uint32_t) fclamp(cx + floorf(low.x),  0, width - 1);
	const uint32_t end_x   = (uint32_t) fclamp(cx + ceilf(high.x),  0, width - 1);
	const uint32_t start_y = (uint32_t) fclamp(cy - floorf(high.y), 0, height - 1);
	const uint32_t end_y   = (uint32_t) fclamp(cy - ceilf(low.y),   0, height - 1);

	assert(start_x < width);
	assert(end_x   < width);
	assert(start_y < height);
	assert(end_y   < height);

	// printf("start_x: %d\n", start_x);
	// printf("end_x: %d\n", end_x);
	// printf("start_y: %d\n", start_y);
	// printf("end_y: %d\n", end_y);

	for (uint16_t y = start_y; y < end_y; y++) {
		for (uint16_t x = start_x; x < end_x; x++) {
			const vec2 p = VEC2(x - cx, cy - y);

			for (size_t i = 0; i < ibuf_len; i += 3) {
				assert(index_buffer[i+0] < vertex_buffer_len);
				assert(index_buffer[i+1] < vertex_buffer_len);
				assert(index_buffer[i+2] < vertex_buffer_len);

				const vec4 a = transformed_vertex_buffer[index_buffer[i+0]];
				const vec4 b = transformed_vertex_buffer[index_buffer[i+1]];
				const vec4 c = transformed_vertex_buffer[index_buffer[i+2]];

				assert(a.w != 0);
				assert(b.w != 0);
				assert(c.w != 0);

				vec3 a2 = div31(VEC_TO_VEC3(a), a.w);
				vec3 b2 = div31(VEC_TO_VEC3(b), b.w);
				vec3 c2 = div31(VEC_TO_VEC3(c), c.w);

				if (!printed) {
					print_vec3(a2);
					print_vec3(b2);
					print_vec3(c2);
					printf("==================\n");
				}

				if (is_point_in_polygon(p, VEC_TO_VEC2(a2), VEC_TO_VEC2(b2), VEC_TO_VEC2(c2))) {
					frame[y * width + x] = 0x7F7F7F;
				}
			}

			printed = true;
		}
	}

	printed = true;
}