/**
 * Файл отвечает за загрузку и освобождение ресурсов
 */

#include "module.h"
#include "../util/read_png.h"
#include "../util/load_font.h"
#include "../util/render_glyph.h"
#include "../util/get_system_name.h"
#include <time.h>

static const char *logo_path, *font_path;

int RANDOM_CONSTANT;
const char* system_name;

png_structp png_ptr;
png_infop info_ptr, end_info;

FT_Face face;
static FT_Library freeTypeLib;

color_t* bg_buffer;

void gml_read_config(config_t* cfgp) {
	logo_path = read_config_str(cfgp, "mint_logo__logo_path");
	font_path = read_config_str(cfgp, "mint_logo__font_path");
}

// --------------------------------------------- setup --------------------------------------------

void gml_setup(void) {
	srand(time(NULL));
	RANDOM_CONSTANT = rand();

	fps = FPS;

	system_name = get_system_name();

	if (system_name[0] != '\0') {
		freeTypeLib = init_freetype_lib_or_exit();
		face = load_freetype_face_or_exit(freeTypeLib, font_path, GLYPH_HEIGHT);
	}

	read_png(logo_path, &png_ptr, &info_ptr, &end_info);
}

void gml_setup_after_drm(uint32_t width, uint32_t height) {
	bg_buffer = aligned_alloc(sizeof(color_t), height * sizeof(color_t));
}

// ------------------------------------------- cleanup --------------------------------------------

void gml_cleanup_before_drm(void) {
	if (bg_buffer) { free(bg_buffer); bg_buffer = NULL; }
}

void gml_cleanup(void) {
	if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	if (face) { FT_Done_Face(face); face = NULL; }
	if (freeTypeLib) { FT_Done_FreeType(freeTypeLib); freeTypeLib = NULL; }

	if (font_path) { free((void*) font_path); font_path = NULL; }
	if (logo_path) { free((void*) logo_path); logo_path = NULL; }
}