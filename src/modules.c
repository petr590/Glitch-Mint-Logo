#include "modules.h"
#include <stdlib.h>
#include <dlfcn.h>

void (*read_config)(config_t*);
void (*setup)(void);
void (*setup_after_drm)(uint16_t, uint16_t);
void (*cleanup_before_drm)(void);
void (*cleanup)(void);
void (*draw)(int, uint16_t, uint16_t, color_t*);

static void* load_sym(const char* filename, void* handle, const char* id) {
	void* sym = dlsym(handle, id);

	char* error = dlerror();
	if (error)  {
		fprintf(stderr, "Cannot load symbol %s from '%s': %s\n", id, filename, error);
		exit(EXIT_FAILURE);
	}

	return sym;
}

void load_module(const char* filename) {
	void* handle = dlopen(filename, RTLD_LAZY | RTLD_GLOBAL);

	if (!handle) {
		fprintf(stderr, "Cannot load dynamic library %s: %s\n", filename, dlerror());
		exit(EXIT_FAILURE);
	}

	read_config        = load_sym(filename, handle, "gml_read_config");
	setup              = load_sym(filename, handle, "gml_setup");
	setup_after_drm    = load_sym(filename, handle, "gml_setup_after_drm");
	cleanup_before_drm = load_sym(filename, handle, "gml_setup_after_drm");
	cleanup            = load_sym(filename, handle, "gml_cleanup");
	draw               = load_sym(filename, handle, "gml_draw");
}