/**
 * Файл отвечает за загрузку и освобождение ресурсов
 */

#include "module.h"
#include "../util/render_glyth.h"
#include <time.h>

#define FPS 60
#define FPS_EPSILON 1

static const char* font_path;

FT_Face face;
int text_w, text_h;
char* text_buffer;

void gml_read_config(config_t* cfgp) {
	font_path = read_config_str(cfgp, "matrix__font_path");
}

// --------------------------------------------- setup --------------------------------------------

static FT_Library freeType;

static void init_font_face(void) {
	FT_Error err = FT_Init_FreeType(&freeType);
	if (err) {
		fprintf(stderr, "Cannot initialize FreeType\n");
		exit(EXIT_FAILURE);
	}

	err = FT_New_Face(freeType, font_path, 0, &face);
	if (err) {
		fprintf(stderr, "Cannot load font from file '%s'\n", font_path);
		exit(EXIT_FAILURE);
	}

	FT_Set_Pixel_Sizes(face, 0, GLYTH_HEIGHT);
}


void gml_setup(void) {
	srand(time(NULL));
	init_font_face();
}

void gml_setup_after_drm(uint32_t width, uint32_t height) {
	text_w = (width + CHAR_WIDTH - 1) / CHAR_WIDTH;
	text_h = (height + CHAR_HEIGHT - 1) / CHAR_HEIGHT;

	const size_t size = text_w * text_h * sizeof(char);
	text_buffer = malloc(size);
	memset(text_buffer, ' ', size);

	if (abs(FPS - fps) > FPS_EPSILON) {
		fps = FPS;
	}
}

// ------------------------------------------- cleanup --------------------------------------------

void gml_cleanup_before_drm(void) {
	if (text_buffer) { free(text_buffer); text_buffer = NULL; }
}

void gml_cleanup(void) {
	if (face) { FT_Done_Face(face); face = NULL; }
	if (freeType) { FT_Done_FreeType(freeType); freeType = NULL; }

	if (font_path) { free((void*) font_path); font_path = NULL; }
}