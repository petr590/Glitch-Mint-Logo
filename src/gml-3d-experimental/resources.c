/**
 * Файл отвечает за загрузку и освобождение ресурсов
 */

#include "module.h"
#include <stdlib.h>

#define UNUSED(v) (void)(v)

vec3* vertex_buffer;
vec4* transformed_vertex_buffer;
index_t* index_buffer;

size_t vertex_buffer_len;
size_t index_buffer_len;

float* depth_buffer;

void gml_read_config(config_t* cfgp) {
	UNUSED(cfgp);
}

// --------------------------------------------- setup --------------------------------------------

static vec3 vertex_data[] = {
	// {{ -100, -100, -1   }},
	// {{ -100, -100, -101 }},
	// {{ -100,  100, -1   }},
	// {{ -100,  100, -101 }},
	// {{  100, -100, -1   }},
	// {{  100, -100, -101 }},
	// {{  100,  100, -1   }},
	// {{  100,  100, -101 }},

	{{ 0, 100, -1 }},
	{{ 100, 0, -1 }},
	{{ -100, 0, -1 }},

	{{ 0, 100, -50 }},
	{{ 0, 0, -100 }},
	{{ 0, 0, -1 }},
};

static index_t index_data[] = {
	// 0,1,4,
	// 1,4,5,
	// 2,3,6,
	// 3,6,7,
	// 0,1,3,
	// 0,2,3,
	// 4,5,7,
	// 4,6,7,
	// 0,2,6,
	// 0,4,6,
	// 1,3,7,
	// 1,5,7,
	
	// 0,1,2,
	3,4,5,
};

void gml_setup(void) {
	vertex_buffer = vertex_data;
	index_buffer = index_data;

	vertex_buffer_len = sizeof(vertex_data) / sizeof(vec3);
	index_buffer_len = sizeof(index_data) / sizeof(index_t);

	transformed_vertex_buffer = malloc(vertex_buffer_len * sizeof(vec4));
}

void gml_setup_after_drm(uint16_t width, uint16_t height) {
	depth_buffer = malloc(width * height * sizeof(float));
}

// ------------------------------------------- cleanup --------------------------------------------

void gml_cleanup_before_drm(void) {
	if (depth_buffer) { free((void*) depth_buffer); depth_buffer = NULL; }
}

void gml_cleanup(void) {
	if (transformed_vertex_buffer) { free((void*) transformed_vertex_buffer); transformed_vertex_buffer = NULL; }
}