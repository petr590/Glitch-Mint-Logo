/**
 * Файл отвечает за загрузку и освобождение ресурсов
 */

#include "module.h"
#include "read_png.h"
#include <time.h>

static const char *logo_path, *font_path;

int RANDOM_CONSTANT;
const char* system_name;
png_structp png_ptr;
png_infop info_ptr, end_info;
FT_Face face;
color_t* bg_buffer;

void glspl_read_config(config_t* cfgp) {
	logo_path = read_config_str(cfgp, "logo_path");
	font_path = read_config_str(cfgp, "font_path");
	printf("%s\n", font_path);
}

// --------------------------------------------- setup --------------------------------------------

static const char* get_system_name(void) {
	FILE* fp = popen("lsb_release -sd", "r");

	if (!fp) {
		perror("Cannot execute `lsb_release -sd`");
		return "";
	}

	static char name[64];
	fgets(name, sizeof(name), fp);
	pclose(fp);

	char* end = strchr(name, '\n');
	if (end) {
		end[0] = '\0';
	}
	
	return name;
}


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


void glspl_setup(void) {
	srand(time(NULL));
	RANDOM_CONSTANT = rand();

	system_name = get_system_name();

	if (system_name[0] != '\0') {
		init_font_face();
	}

	read_png(logo_path, &png_ptr, &info_ptr, &end_info);
}

void glspl_setup_after_drm(uint32_t width, uint32_t height) {
	bg_buffer = aligned_alloc(sizeof(color_t), height * sizeof(color_t));
}

// ------------------------------------------- cleanup --------------------------------------------

void glspl_cleanup_before_drm(void) {
	if (bg_buffer) { free(bg_buffer); bg_buffer = NULL; }
}

void glspl_cleanup(void) {
	if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	if (face) { FT_Done_Face(face); face = NULL; }
	if (freeType) { FT_Done_FreeType(freeType); freeType = NULL; }

	if (font_path) { free((void*) font_path); font_path = NULL; }
	if (logo_path) { free((void*) logo_path); logo_path = NULL; }
}