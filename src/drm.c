#include "drm.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#define BPP 32
#define DEPTH 24

int card_file = -1;
drmModeRes* resources;
drmModeConnector* connector;
fb_info* fb_info1;
fb_info* fb_info2;

static drmModeCrtc* saved_crtc;


static void print_error_and_exit(const char* msg, int ret) {
	fprintf(stderr, msg, ret);
	exit(EXIT_FAILURE);
}

static fb_info* create_fb(uint32_t width, uint32_t height) {
	struct drm_mode_create_dumb create = {
		.width = width,
		.height = height,
		.bpp = BPP,
		.flags = 0,
	};
	
	int ret = drmIoctl(card_file, DRM_IOCTL_MODE_CREATE_DUMB, &create);
	if (ret) print_error_and_exit("Cannot create dumb: %d\n", ret);


	uint32_t fb_id;
	ret = drmModeAddFB(card_file, create.width, create.height, DEPTH, create.bpp, create.pitch, create.handle, &fb_id);
	if (ret) print_error_and_exit("Cannot create framebuffer: %d\n", ret);
	

 	struct drm_mode_map_dumb map = {
 		.handle = create.handle
 	};
 	
	ret = drmIoctl(card_file, DRM_IOCTL_MODE_MAP_DUMB, &map);
	if (ret) print_error_and_exit("Cannot map dump: %d\n", ret);


	color_t* vaddr = (color_t*) mmap(NULL, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, card_file, map.offset);
	fb_info* res = malloc(sizeof(fb_info));

	res->size = create.size;
	res->handle = create.handle;
	res->fb_id = fb_id;
	res->vaddr = vaddr;
	return res;
}

static void release_fb(fb_info* fb) {
	struct drm_mode_destroy_dumb destroy = {
		.handle = fb->handle
	};
	
	drmModeRmFB(card_file, fb->fb_id);
	munmap(fb->vaddr, fb->size);
	drmIoctl(card_file, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);

	free(fb);
}


void init_drm(const char* card_path) {
	card_file = open(card_path, O_RDWR | O_CLOEXEC);
	if (card_file < 0) {
		fprintf(stderr, "Cannot open '%s': %s\n", card_path, strerror(errno));
		exit(errno);
	}
	
	uint64_t has_dumb;
	if (drmGetCap(card_file, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
		fprintf(stderr, "Device '%s' does not support dumb buffers\n", card_path);
		exit(EOPNOTSUPP);
	}
	
	resources = drmModeGetResources(card_file);
	connector = drmModeGetConnector(card_file, resources->connectors[0]);
	saved_crtc = drmModeGetCrtc(card_file, resources->crtcs[0]);

	const drmModeModeInfoPtr mode = &connector->modes[0];
	
	fb_info1 = create_fb(mode->hdisplay, mode->vdisplay);
	fb_info2 = create_fb(mode->hdisplay, mode->vdisplay);
}

void cleanup_drm(void) {
	if (card_file >= 0 && fb_info1) { release_fb(fb_info1); fb_info1 = NULL; }
	if (card_file >= 0 && fb_info2) { release_fb(fb_info2); fb_info2 = NULL; }
	
	if (card_file >= 0 && connector && saved_crtc) {
		drmModeSetCrtc(card_file, saved_crtc->crtc_id, saved_crtc->buffer_id, saved_crtc->x, saved_crtc->y, &connector->connector_id, 1, &saved_crtc->mode);
		drmModeFreeCrtc(saved_crtc);
		saved_crtc = NULL;
	}
	
	if (connector) { drmModeFreeConnector(connector); connector = NULL; }
	if (resources) { drmModeFreeResources(resources); resources = NULL; }
	
	if (card_file >= 0) { close(card_file); card_file = -1; }
}