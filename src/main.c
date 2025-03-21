#include "draw.h"
#include "drm_fb.h"
#include "read_png.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <xf86drmMode.h>

#define CARD_FILE "/dev/dri/card0"
#define LOGO_FILENAME "logo.png"

#define SECONDS 5
#define FPS 10

int RANDOM_CONSTANT = 0;


int main() {
	int pid = fork();
	
	if (pid != 0) {
		return 0;
	}

	srand(time(NULL));
	RANDOM_CONSTANT = rand();
	
	int dev_file = open(CARD_FILE, O_RDWR | O_CLOEXEC);
	if (dev_file < 0) {
		fprintf(stderr, "Cannot open '%s': %m\n", CARD_FILE);
		return -errno;
	}

	png_structp png_ptr;
	png_infop info_ptr, end_info;

	int res = read_png(LOGO_FILENAME, &png_ptr, &info_ptr, &end_info);

	switch (res) {
	case ENOENT:
		close(dev_file);
		fprintf(stderr, "Error: No such file or directory: '" LOGO_FILENAME "'\n");
		return res;
	
	case ERROR_READ_PNG:
		close(dev_file);
		fprintf(stderr, "Error: Cannot read file '" LOGO_FILENAME "' as png\n");
		return res;
	}
	
	
	drmModeRes *resources = drmModeGetResources(dev_file);
	
	uint32_t crtc_id = resources->crtcs[0];
	uint32_t connector_id = resources->connectors[0];
	
	drmModeConnector *connector = drmModeGetConnector(dev_file, connector_id);

	const uint32_t width = connector->modes[0].hdisplay;
	const uint32_t height = connector->modes[0].vdisplay;
	
	const fb_info fb1 = create_fb(dev_file, width, height);
	const fb_info fb2 = create_fb(dev_file, width, height);
	
	const fb_info
			*fbp1 = &fb1,
			*fbp2 = &fb2;
	
	color_t *bg_buffer = aligned_alloc(sizeof(color_t), height * sizeof(color_t));

	for (int tick = 0; tick < FPS * SECONDS; tick++) {
		clock_t start = clock();

		draw(tick, width, height, (uint8_t*)fbp1->vaddr, bg_buffer, png_ptr, info_ptr);
		drmModeSetCrtc(dev_file, crtc_id, fbp1->fb_id, 0, 0, &connector_id, 1, &connector->modes[0]);

		const fb_info *tmp = fbp1;
		fbp1 = fbp2;
		fbp2 = tmp;

		usleep((1000000 / FPS) - (clock() - start) * 1000000 / CLOCKS_PER_SEC);
	}
	
	free(bg_buffer);

	release_fb(dev_file, &fb1);
	release_fb(dev_file, &fb2);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	
	drmModeFreeConnector(connector);
	drmModeFreeResources(resources);
	
	close(dev_file);
	
	return 0;
}