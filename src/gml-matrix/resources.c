/**
 * Файл отвечает за загрузку и освобождение ресурсов
 */

#include "module.h"
#include "util/render_glyph.h"
#include "util/load_font.h"
#include <time.h>
#include <math.h>

#define FPS 60
#define FPS_EPSILON 1

static const char* font_path;

FT_Face face;
int32_t text_w, text_h;
char* text_buffer;

void gml_read_config(config_t* cfgp) {
	font_path = read_config_str(cfgp, "matrix__font_path");
}

// --------------------------------------------- setup --------------------------------------------

static FT_Library freeTypeLib;

void gml_setup(void) {
	srand(time(NULL));
	
	freeTypeLib = init_freetype_lib_or_exit();
	face = load_freetype_face_or_exit(freeTypeLib, font_path, GLYPH_HEIGHT);
}

void gml_setup_after_drm(uint16_t width, uint16_t height) {
	text_w = (width + CHAR_WIDTH - 1) / CHAR_WIDTH;
	text_h = (height + CHAR_HEIGHT - 1) / CHAR_HEIGHT;

	const size_t size = text_w * text_h * sizeof(char);
	text_buffer = malloc(size);
	memset(text_buffer, ' ', size);

	if (fabs(FPS - fps) > FPS_EPSILON) {
		fps = FPS;
	}
}

// ------------------------------------------- cleanup --------------------------------------------

void gml_cleanup_before_drm(void) {
	if (text_buffer) { free(text_buffer); text_buffer = NULL; }
}

void gml_cleanup(void) {
	if (face) { FT_Done_Face(face); face = NULL; }
	if (freeTypeLib) { FT_Done_FreeType(freeTypeLib); freeTypeLib = NULL; }

	if (font_path) { free((void*) font_path); font_path = NULL; }
}