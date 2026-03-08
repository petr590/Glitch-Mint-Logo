/**
 * Файл отвечает за загрузку и освобождение ресурсов
 */

#include "module.h"
#include "util/read_png.h"
#include "util/load_font.h"
#include "util/render_glyph.h"
#include "util/system_info.h"
#include <ctype.h>
#include <time.h>

#define UNUSED(v) (void)(v)

static const char *logo_path, *font_path;

color_t text_color;
int RANDOM_CONSTANT;
const char* system_name;

png_structp png_ptr;
png_infop info_ptr, end_info;

FT_Face face;
static FT_Library freeTypeLib;

color_t* bg_buffer;


static char* strcpy_lower_case(const char* str) {
	const size_t len = strlen(str);
	char* new_str = malloc(len + 1);

	for (size_t i = 0; i < len; i++) {
		new_str[i] = tolower(str[i]);
	}

	new_str[len] = '\0';
	return new_str;
}


static const char* concat_logo_path(const char* dir, const char* filename) {
	const size_t dir_len = strlen(dir);
	const int need_slash = (dir_len == 0 || dir[dir_len - 1] != '/') ? 1 : 0;

	const size_t length = dir_len + need_slash + strlen(filename) + 1 /*null terminator*/;
	char* logo_path = malloc(length);

	snprintf(logo_path, length, "%s%s%s", dir, need_slash ? "/" : "", filename);
	return logo_path;
}


static const char* get_logo_filename(const char* system_id) {
	if (system_id == NULL) return NULL;

	if (strcmp(system_id, "ubuntu")       == 0) return "ubuntu.png";
	if (strcmp(system_id, "debian")       == 0) return "debian.png";
	if (strcmp(system_id, "linuxmint")    == 0) return "mint.png";
	if (strcmp(system_id, "arch")         == 0) return "arch.png";
	if (strcmp(system_id, "fedora")       == 0) return "fedora.png";
	if (strcmp(system_id, "centos")       == 0) return "centos.png";
	if (strcmp(system_id, "centoslinux")  == 0) return "centos.png";
	if (strcmp(system_id, "centosstream") == 0) return "centos.png";
	if (strcmp(system_id, "alt")          == 0) return "alt.png";
	if (strcmp(system_id, "altlinux")     == 0) return "alt.png";
	if (strcmp(system_id, "manjaro")      == 0) return "manjaro.png";

	return NULL;
}

static color_t get_text_color(const char* system_id) {
	if (system_id == NULL) return 0xFFFFFF;
	
	if (strcmp(system_id, "ubuntu")    == 0) return 0xf35c23;
	if (strcmp(system_id, "debian")    == 0) return 0xd80150;
	if (strcmp(system_id, "linuxmint") == 0) return 0x87cf3e;
	if (strcmp(system_id, "arch")      == 0) return 0x2298d3;
	if (strcmp(system_id, "fedora")    == 0) return 0x396eb5;
	if (strcmp(system_id, "alt")       == 0) return 0xffca08;
	if (strcmp(system_id, "altlinux")  == 0) return 0xffca08;
	if (strcmp(system_id, "manjaro")   == 0) return 0x34bf5c;

	return 0xFFFFFF;
}


/// Инициализируем system_name и ресурсы, которые зависят от system_id и от конфига
void gml_read_config(config_t* cfgp) {
	const char* system_id;

	get_system_id_and_name(&system_id, &system_name);
	system_id = strcpy_lower_case(system_id);

	text_color = get_text_color(system_id);

	const char* logo_dir = read_config_str(cfgp, "glitch_logo__logo_dir");
	logo_path = concat_logo_path(logo_dir, get_logo_filename(system_id));
	font_path = read_config_str(cfgp, "glitch_logo__font_path");

	free((void*) logo_dir);
	free((void*) system_id);
}

// --------------------------------------------- setup --------------------------------------------

void gml_setup(void) {
	srand(time(NULL));
	RANDOM_CONSTANT = rand();

	fps = FPS;

	if (system_name != NULL && system_name[0] != '\0') {
		freeTypeLib = init_freetype_lib_or_exit();
		face = load_freetype_face_or_exit(freeTypeLib, font_path, GLYPH_HEIGHT);
	}

	read_png(logo_path, &png_ptr, &info_ptr, &end_info);
}

void gml_setup_after_drm(uint16_t width, uint16_t height) {
	UNUSED(width);
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