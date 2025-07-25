/**
 * Файл отвечает за загрузку и освобождение ресурсов
 */

#include "module.h"
#include "../util/util.h"
#include "../util/read_png.h"
#include "../util/load_font.h"
#include "../util/render_glyph.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#define FPS 30
#define FPS_EPSILON 1


static const char *logo_path, *font_path;

png_structp png_ptr;
png_infop info_ptr, end_info;

FT_Face face;
static FT_Library freeTypeLib;

bitset2d v_bg_buffer, h_bg_buffer, p_bg_buffer;

void gml_read_config(config_t* cfgp) {
	logo_path = read_config_str(cfgp, "yorha__logo_path");
	font_path = read_config_str(cfgp, "yorha__font_path");
}

// --------------------------------------------- setup --------------------------------------------

void gml_setup(void) {
	srand(time(NULL));
	init_socket();
	read_png(logo_path, &png_ptr, &info_ptr, &end_info);

	freeTypeLib = init_freetype_lib_or_exit();
	face = load_freetype_face_or_exit(freeTypeLib, font_path, GLYPH_HEIGHT);
}


void gml_setup_after_drm(uint32_t width, uint32_t height) {
	uint32_t w = u32_div_ceil(u32_div_ceil(width  + LINE_WIDTH / 2, 2), CELL_SIZE);
	uint32_t h = u32_div_ceil(u32_div_ceil(height + LINE_WIDTH / 2, 2), CELL_SIZE);
	assert(w * 2 * CELL_SIZE >= width);
	assert(h * 2 * CELL_SIZE >= height);

	bitset2d_create(&v_bg_buffer, w, h);
	bitset2d_create(&h_bg_buffer, w, h);
	bitset2d_create(&p_bg_buffer, w, h);

	bitset2d_clear(&v_bg_buffer);
	bitset2d_clear(&h_bg_buffer);
	bitset2d_clear(&p_bg_buffer);

	if (fabs(FPS - fps) > FPS_EPSILON) {
		fps = FPS;
	}
}

// ------------------------------------------- cleanup --------------------------------------------

void gml_cleanup_before_drm(void) {
	bitset2d_destroy(&p_bg_buffer);
	bitset2d_destroy(&h_bg_buffer);
	bitset2d_destroy(&v_bg_buffer);
}

void gml_cleanup(void) {
	if (face) { FT_Done_Face(face); face = NULL; }
	if (freeTypeLib) { FT_Done_FreeType(freeTypeLib); freeTypeLib = NULL; }

	if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	cleanup_socket();
}
