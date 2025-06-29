#include "drm_fb.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#define BPP 32
#define DEPTH 24

static void print_error_and_exit(const char* msg, int ret) {
	fprintf(stderr, msg, ret);
	exit(EXIT_FAILURE);
}

fb_info* create_fb(int dev_file, uint32_t width, uint32_t height) {
	struct drm_mode_create_dumb create = {
		.width = width,
		.height = height,
		.bpp = BPP,
		.flags = 0,
	};
	
	int ret = drmIoctl(dev_file, DRM_IOCTL_MODE_CREATE_DUMB, &create);
	if (ret) print_error_and_exit("Cannot create dumb: %d\n", ret);


	uint32_t fb_id;
	ret = drmModeAddFB(dev_file, create.width, create.height, DEPTH, create.bpp, create.pitch, create.handle, &fb_id);
	if (ret) print_error_and_exit("Cannot create framebuffer: %d\n", ret);
	

 	struct drm_mode_map_dumb map = {
 		.handle = create.handle
 	};
 	
	ret = drmIoctl(dev_file, DRM_IOCTL_MODE_MAP_DUMB, &map);
	if (ret) print_error_and_exit("Cannot map dump: %d\n", ret);


	color_t* vaddr = (color_t*) mmap(NULL, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, dev_file, map.offset);
	fb_info* res = malloc(sizeof(fb_info));

	res->size = create.size;
	res->handle = create.handle;
	res->fb_id = fb_id;
	res->vaddr = vaddr;
	return res;
}

void release_fb(int dev_file, fb_info* fb) {
	struct drm_mode_destroy_dumb destroy = {
		.handle = fb->handle
	};
	
	drmModeRmFB(dev_file, fb->fb_id);
	munmap(fb->vaddr, fb->size);
	drmIoctl(dev_file, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);

	free(fb);
}