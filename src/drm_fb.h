#include "common.h"

typedef struct {
	uint32_t size;
	uint32_t handle;
	uint32_t fb_id;
	color_t* vaddr;
} fb_info;

fb_info* create_fb(int dev_file, uint32_t width, uint32_t height);

void release_fb(int dev_file, fb_info* fb);