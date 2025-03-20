#include "png_reader.c"
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <assert.h>

#define CARD_FILE "/dev/dri/card0"
#define LOGO_FILENAME "logo.png"
#define BG_GS 0x20
#define BG_RGB (BG_GS << 16 | BG_GS << 8 | BG_GS)

#define FPS 30
#define MAJOR_GLITCH_SEC 1.f
#define MINOR_GLITCH_SEC 0.75f
#define MINOR_GLITCH_PERIOD 3.f
#define SECONDS 10


static inline uint32_t u32min(uint32_t num1, uint32_t num2) {
	return num1 < num2 ? num1 : num2;
}

static inline int32_t i32min(int32_t num1, int32_t num2) {
	return num1 < num2 ? num1 : num2;
}

static inline int32_t i32max(int32_t num1, int32_t num2) {
	return num1 > num2 ? num1 : num2;
}

static inline int randrange(int from, int to) {
	return rand() % (to - from + 1) + from;
}

static inline uint32_t mix(uint8_t grayscale, uint32_t argb) {
	uint8_t alpha = argb >> 24;
	uint8_t r = (grayscale * (0xFF - alpha) + (uint8_t)(argb >> 16) * alpha) / 0xFF;
	uint8_t g = (grayscale * (0xFF - alpha) + (uint8_t)(argb >>  8) * alpha) / 0xFF;
	uint8_t b = (grayscale * (0xFF - alpha) + (uint8_t)(argb      ) * alpha) / 0xFF;
	return r << 16 | g << 8 | b;
}

static inline float get_noise(float sec) {
	if (sec < MAJOR_GLITCH_SEC)
		return (MAJOR_GLITCH_SEC - sec) * (1 / MAJOR_GLITCH_SEC);

	float rem = fmodf(sec, MINOR_GLITCH_PERIOD);

	if (rem < MINOR_GLITCH_SEC) {
		return rem < MINOR_GLITCH_SEC / 2 ? rem : MINOR_GLITCH_SEC - rem;
	}

	return 0;
}




typedef struct {
	uint32_t size;
	uint32_t handle;
	uint32_t fb_id;
	uint32_t *vaddr;
} framebuffer_info;

static framebuffer_info create_fb(int fd, uint32_t width, uint32_t height) {
	struct drm_mode_create_dumb create = {
		.width = width,
		.height = height,
		.bpp = 32,
		.flags = 0,
	};
	
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
	
	uint32_t fb_id;
	drmModeAddFB(fd, create.width, create.height, 24, 32, create.pitch, create.handle, &fb_id);
	
 	struct drm_mode_map_dumb map = {
 		.handle = create.handle
 	};
 	
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
	
	uint32_t *vaddr = (uint32_t*) mmap(NULL, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map.offset);

	framebuffer_info fb_info = {
		.size = create.size,
		.handle = create.handle,
		.fb_id = fb_id,
		.vaddr = vaddr,
	};

	return fb_info;
}

static void release_fb(int fd, const framebuffer_info *fb_info) {
	struct drm_mode_destroy_dumb destroy = {};
	destroy.handle = fb_info->handle;
	
	drmModeRmFB(fd, fb_info->fb_id);
	munmap(fb_info->vaddr, fb_info->size);
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}


static void fill_fb(float noise, uint32_t width, uint32_t height, uint32_t *vaddr, png_const_structp png_ptr, png_infop info_ptr) {
	memset(vaddr, BG_GS, width * height * sizeof(uint32_t));

	const uint32_t img_w = u32min(width, png_get_image_width(png_ptr, info_ptr));
	const uint32_t img_h = u32min(height, png_get_image_height(png_ptr, info_ptr));
	const uint32_t start_x = (width - img_w) / 2;
	const uint32_t start_y = (height - img_h) / 2;
	const uint32_t end_x = start_x + img_w;
	const uint32_t end_y = start_y + img_h;

	const uint32_t max_line_h    = img_h / 4;
	const uint32_t max_off_x     = img_w * noise * 0.2f;
	const uint32_t min_off_color = img_w * noise * 0.05f;
	const uint32_t max_off_color = img_w * noise * 0.1f;

	uint32_t **img_rows = (uint32_t**) png_get_rows(png_ptr, info_ptr);
	uint8_t* vaddr_bytes = (uint8_t*) vaddr;

	for (uint32_t sy = start_y; sy < end_y; ) {
		const int32_t off_x = ((float) rand() / RAND_MAX) >= noise ? 0 : randrange(-max_off_x, max_off_x);
		const int32_t off_color = randrange(min_off_color, max_off_color) * 4;

		const uint32_t line_h = u32min(end_y - sy, randrange(4, max_line_h));

		const int dx = off_x > 0 ? -1 : 1;
		const uint32_t sx = off_x > 0 ? i32min(width, end_x + max_off_x) : i32max(0, start_x - max_off_x);
		const uint32_t ex = (off_x > 0 ? i32max(0, start_x - max_off_x) : i32min(width, end_x + max_off_x)) + dx;

		for (uint32_t y = sy; y < sy + line_h; y++) {
			for (uint32_t x = sx; x != ex; x += dx) {
				int32_t tx = x + off_x;

				if (tx >= 0 && tx < width) {
					int32_t u = x - start_x;
					int32_t v = y - start_y;
					assert(v >= 0 && v <= img_h);

					uint32_t color = u >= 0 && u <= img_w ? mix(BG_GS, img_rows[v][u]) : BG_RGB;
					uint32_t index = (y * width + tx) * 4;

					vaddr_bytes[index + 2 - off_color] = color >> 16; // R
					vaddr_bytes[index + 1]             = color >> 8;  // G
					vaddr_bytes[index + 0 + off_color] = color;       // B
				}
			}
		}

		sy += line_h;
	}
}


int main() {
	int pid = fork();
	
	if (pid != 0) {
		return 0;
	}

	srand(time(NULL));
	
	int fd = open(CARD_FILE, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "Cannot open '%s': %m\n", CARD_FILE);
		return -errno;
	}

	png_structp png_ptr;
	png_infop info_ptr, end_info;

	int res = read_png(LOGO_FILENAME, &png_ptr, &info_ptr, &end_info);

	switch (res) {
	case ENOENT:
		close(fd);
		fprintf(stderr, "Error: No such file or directory: '" LOGO_FILENAME "'\n");
		return res;
	
	case ERROR_READ_PNG:
		close(fd);
		fprintf(stderr, "Error: Cannot read file '" LOGO_FILENAME "' as png\n");
		return res;
	}
	
	
	drmModeRes *resources = drmModeGetResources(fd);
	
	uint32_t crtc_id = resources->crtcs[0];
	uint32_t connector_id = resources->connectors[0];
	
	drmModeConnector *connector = drmModeGetConnector(fd, connector_id);

	const uint32_t width = connector->modes[0].hdisplay;
	const uint32_t height = connector->modes[0].vdisplay;
	
	const framebuffer_info fb_info1 = create_fb(fd, width, height);
	const framebuffer_info fb_info2 = create_fb(fd, width, height);
	
	const framebuffer_info
			*fbp1 = &fb_info1,
			*fbp2 = &fb_info2;

	for (int frame = 0; frame < FPS * SECONDS; frame++) {
		clock_t start = clock();

		fill_fb(get_noise((float) frame / FPS), width, height, fbp1->vaddr, png_ptr, info_ptr);

		drmModeSetCrtc(fd, crtc_id, fbp1->fb_id, 0, 0, &connector_id, 1, &connector->modes[0]);

		const framebuffer_info *tmp = fbp1;
		fbp1 = fbp2;
		fbp2 = tmp;

		usleep((1000000 / FPS) - (clock() - start) * 1000000 / CLOCKS_PER_SEC);
	}
	
	release_fb(fd, &fb_info1);
	release_fb(fd, &fb_info2);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	
	drmModeFreeConnector(connector);
	drmModeFreeResources(resources);
	
	close(fd);
	
	return 0;
}