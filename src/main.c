#include "draw.h"
#include "drm_fb.h"
#include "read_png.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <xf86drmMode.h>
#include <assert.h>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define CARD_FILE "/dev/dri/card0"
#define LOGO_FILENAME "resources/logo.png"
#define FONT_FILENAME "resources/UbuntuMono-Bold.ttf"

#define SECONDS 5
#define FPS 10

int RANDOM_CONSTANT = 0;


static FT_Library freeType;
static FT_Face face;
static png_structp png_ptr;
static png_infop info_ptr, end_info;
static int dev_file = -1;
static drmModeRes *resources;
static drmModeConnector *connector;
static fb_info *fb_info1, *fb_info2;
static color_t *bg_buffer;


static void release_all() {
	free(bg_buffer);

	if (fb_info1) { release_fb(dev_file, fb_info1); fb_info1 = NULL; }
	if (fb_info2) { release_fb(dev_file, fb_info2); fb_info2 = NULL; }
	
	if (connector) { drmModeFreeConnector(connector); connector = NULL; }
	if (resources) { drmModeFreeResources(resources); }
	
	if (dev_file >= 0) { close(dev_file); dev_file = -1; }

	if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	if (face) { FT_Done_Face(face); face = NULL; }
	if (freeType) { FT_Done_FreeType(freeType); freeType = NULL; }
}

static void on_signal(int signum) {
	release_all();
	printf("Exited by signal %d\n", signum);
}


static void init_font_face() {
	FT_Error err = FT_Init_FreeType(&freeType);
	if (err) {
		fprintf(stderr, "Cannot initialize FreeType\n");
		exit(EXIT_FAILURE);
	}

	err = FT_New_Face(freeType, FONT_FILENAME, 0, &face);
	if (err) {
		fprintf(stderr, "Cannot load font from file '" FONT_FILENAME "'\n");
		exit(EXIT_FAILURE);
	}

	FT_Set_Pixel_Sizes(face, 0, GLYTH_HEIGHT);
}


static void init_drm() {
	dev_file = open(CARD_FILE, O_RDWR | O_CLOEXEC);
	if (dev_file < 0) {
		fprintf(stderr, "Cannot open '" CARD_FILE "': %m\n");
		exit(errno);
	}
	
	resources = drmModeGetResources(dev_file);
	connector = drmModeGetConnector(dev_file, resources->connectors[0]);
}

static const char* get_system_name() {
	FILE* fp = popen("lsb_release -sd", "r");

	if (!fp) {
		fprintf(stderr, "Cannot execute `lsb_release -sd`: %m\n");
		return "";
	}

	static char name[64];
	fgets(name, sizeof(name), fp);
	pclose(fp);

	char* end = strchr(name, '\n');
	if (end) {
		assert(end[0] == '\n');
		end[0] = '\0';
	}
	
	return name;
}


int main() {
	int pid = fork();
	
	if (pid != 0) {
		return EXIT_SUCCESS;
	}

	signal(SIGINT,  on_signal);
	signal(SIGSEGV, on_signal);
	signal(SIGABRT, on_signal);
	// atexit(release_all);

	srand(time(NULL));
	RANDOM_CONSTANT = rand();

	const char* name = get_system_name();

	if (name[0] != '\0') {
		init_font_face();
	}

	read_png(LOGO_FILENAME, &png_ptr, &info_ptr, &end_info);
	init_drm();

	uint32_t connector_id = resources->connectors[0];
	const uint32_t crtc_id = resources->crtcs[0];
	const uint32_t width = connector->modes[0].hdisplay;
	const uint32_t height = connector->modes[0].vdisplay;
	
	fb_info1 = create_fb(dev_file, width, height);
	fb_info2 = create_fb(dev_file, width, height);
	
	fb_info *fbp1 = fb_info1,
			*fbp2 = fb_info2;
	
	bg_buffer = aligned_alloc(sizeof(color_t), height * sizeof(color_t));

	for (int tick = 0; tick < FPS * SECONDS; tick++) {
		clock_t start = clock();

		draw(tick, width, height, fbp1->vaddr, bg_buffer, png_ptr, info_ptr, name, face);
		drmModeSetCrtc(dev_file, crtc_id, fbp1->fb_id, 0, 0, &connector_id, 1, &connector->modes[0]);

		// Swap buffers
		fb_info *tmp = fbp1;
		fbp1 = fbp2;
		fbp2 = tmp;

		usleep((1000000 / FPS) - (clock() - start) * 1000000 / CLOCKS_PER_SEC);
	}
	
	release_all();
	return EXIT_SUCCESS;
}