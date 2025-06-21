/**
 * Файл отвечает за загрузку и освобождение ресурсов
 */

#include "module.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define FPS 45
#define FPS_EPSILON 1

bitset2d v_bg_buffer;
bitset2d h_bg_buffer;
bitset2d p_bg_buffer;


void gml_read_config(config_t* cfgp) {}

// --------------------------------------------- setup --------------------------------------------

void gml_setup(void) {
	srand(time(NULL));
}

void gml_setup_after_drm(uint32_t width, uint32_t height) {
	uint32_t w = (width + PIXEL_SIZE - 1) / PIXEL_SIZE;
	uint32_t h = (height + PIXEL_SIZE - 1) / PIXEL_SIZE;
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

void gml_cleanup(void) {}