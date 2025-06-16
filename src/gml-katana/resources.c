/**
 * Файл отвечает за загрузку и освобождение ресурсов
 */

#include "module.h"
#include "../util/read_png.h"
#include <stdlib.h>
#include <time.h>

#define FPS 45
#define FPS_EPSILON 1

static const char* logo_path;

png_structp png_ptr;
png_infop info_ptr, end_info;

size_t buffer_size;
uint8_t* buffer;


void gml_read_config(config_t* cfgp) {
	logo_path = read_config_str(cfgp, "katana__logo_path");
}

// --------------------------------------------- setup --------------------------------------------

void gml_setup(void) {
	read_png(logo_path, &png_ptr, &info_ptr, &end_info);
}

void gml_setup_after_drm(uint32_t width, uint32_t height) {
	uint32_t w = (width + PIXEL_SIZE - 1) / PIXEL_SIZE;
	uint32_t h = (height + PIXEL_SIZE - 1) / PIXEL_SIZE;
	buffer_size = (w * h + 7) / 8;
	buffer = malloc(buffer_size);

	if (fabs(FPS - fps) > FPS_EPSILON) {
		fps = FPS;
	}
}

// ------------------------------------------- cleanup --------------------------------------------

void gml_cleanup_before_drm(void) {
	if (buffer) { free(buffer); buffer = NULL; }
}

void gml_cleanup(void) {
	if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
}