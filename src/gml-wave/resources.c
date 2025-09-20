#include "module.h"
#include <stdlib.h>
#include <time.h>

#define UNUSED(v) (void)(v)

float *randbuf1, *randbuf2;

void gml_read_config(config_t* cfgp) {
	UNUSED(cfgp);
}

void gml_setup(void) {
	srand(time(NULL));
}

void gml_setup_after_drm(uint16_t width, uint16_t height) {
	UNUSED(height);

	randbuf1 = aligned_alloc(sizeof(float), (width + 1) * sizeof(float));
	randbuf2 = aligned_alloc(sizeof(float), width * sizeof(float));
}

void gml_cleanup_before_drm(void) {
	if (randbuf2) { free(randbuf2); randbuf2 = NULL; }
	if (randbuf1) { free(randbuf1); randbuf1 = NULL; }
}

void gml_cleanup(void) {}