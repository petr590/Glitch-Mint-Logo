#include "draw.h"
#include "drm_fb.h"
#include "read_png.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <errno.h>

#include <libconfig.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <assert.h>

#define PID_FILE    "/run/glitch-mint-logo/pid"
#define CONFIG_FILE "/etc/glitch-mint-logo/config"

int RANDOM_CONSTANT = 0;

static const char *logo_path, *font_path, *dev_path;

static FT_Library freeType;
static FT_Face face;
static png_structp png_ptr;
static png_infop info_ptr, end_info;
static int dev_file = -1;
static drmModeRes *resources;
static drmModeConnector *connector;
static drmModeCrtc *saved_crtc;
static fb_info *fb_info1, *fb_info2;
static color_t *bg_buffer;


static void release_all() {
	if (bg_buffer) { free(bg_buffer); bg_buffer = NULL; }

	if (dev_file >= 0 && fb_info1) { release_fb(dev_file, fb_info1); fb_info1 = NULL; }
	if (dev_file >= 0 && fb_info2) { release_fb(dev_file, fb_info2); fb_info2 = NULL; }
	
	if (dev_file >= 0 && connector && saved_crtc) {
		drmModeSetCrtc(dev_file, saved_crtc->crtc_id, saved_crtc->buffer_id, saved_crtc->x, saved_crtc->y, &connector->connector_id, 1, &saved_crtc->mode);
		drmModeFreeCrtc(saved_crtc);
		saved_crtc = NULL;
	}
	
	if (connector) { drmModeFreeConnector(connector); connector = NULL; }
	if (resources) { drmModeFreeResources(resources); resources = NULL; }
	
	if (dev_file >= 0) { close(dev_file); dev_file = -1; }

	if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

	if (face) { FT_Done_Face(face); face = NULL; }
	if (freeType) { FT_Done_FreeType(freeType); freeType = NULL; }
}

static void on_signal(int sig) {
	printf("Exited by signal %d\n", sig);
	signal(sig, SIG_DFL);
	exit((sig == SIGABRT || sig == SIGSEGV) ? EXIT_FAILURE : EXIT_SUCCESS);
}


static void init_font_face() {
	FT_Error err = FT_Init_FreeType(&freeType);
	if (err) {
		fprintf(stderr, "Cannot initialize FreeType\n");
		exit(EXIT_FAILURE);
	}

	err = FT_New_Face(freeType, font_path, 0, &face);
	if (err) {
		fprintf(stderr, "Cannot load font from file '%s'\n", font_path);
		exit(EXIT_FAILURE);
	}

	FT_Set_Pixel_Sizes(face, 0, GLYTH_HEIGHT);
}


static void init_drm() {
	dev_file = open(dev_path, O_RDWR | O_CLOEXEC);
	if (dev_file < 0) {
		fprintf(stderr, "Cannot open '%s': %m\n", dev_path);
		exit(errno);
	}
	
	uint64_t has_dumb;
	if (drmGetCap(dev_file, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
		fprintf(stderr, "Device '%s' does not support dumb buffers\n", dev_path);
		exit(EOPNOTSUPP);
	}
	
	resources = drmModeGetResources(dev_file);
	connector = drmModeGetConnector(dev_file, resources->connectors[0]);
	saved_crtc = drmModeGetCrtc(dev_file, resources->crtcs[0]);
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
		end[0] = '\0';
	}
	
	return name;
}


enum action { START, STOP } action = START;

static void parse_args(int argc, const char* argv[]) {
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--stop") == 0) {
			action = STOP;
		} else {
			fprintf(stderr, "Usage: %s [--stop]\n", argv[0]);
			exit(EINVAL);
		}
	}
}

static void read_config_str(config_t* cfg_ptr, const char* key, const char** str) {
	if (!config_lookup_string(cfg_ptr, key, str)) {
		fprintf(stderr, "Config file (" CONFIG_FILE ") does not contain '%s' setting\n", key);
		config_destroy(cfg_ptr);
		exit(EXIT_FAILURE);
	}
}

static void read_config() {
	config_t cfg; 
	config_init(&cfg);

	if (!config_read_file(&cfg, CONFIG_FILE)) {
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		exit(EXIT_FAILURE);
	}

	read_config_str(&cfg, "logo_path", &logo_path);
	read_config_str(&cfg, "font_path", &font_path);
	read_config_str(&cfg, "dev_path" , &dev_path);
}


static void create_dirs(const char* path) {
	char* buffer = strdup(path);
	size_t pathlen = strlen(buffer);

	for (size_t i = 1; i < pathlen; i++) {
		if (buffer[i] == '/') {
			buffer[i] = '\0';

			struct stat st;
			if (stat(buffer, &st) == -1) {
				mkdir(buffer, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
			}

			buffer[i] = '/';
		}
	}

	free(buffer);
}


static void start() {
	if (fork()) return;

	signal(SIGINT,  on_signal);
	signal(SIGTERM, on_signal);
	signal(SIGSEGV, on_signal);
	signal(SIGABRT, on_signal);
	atexit(release_all);
	
	create_dirs(PID_FILE);
	
	FILE* pid_fp = fopen(PID_FILE, "w");
	if (!pid_fp) {
		fprintf(stderr, "Cannot open '" PID_FILE "'\n");
		exit(EXIT_FAILURE);
	}

	fprintf(pid_fp, "%d", getpid());
	fclose(pid_fp);

	srand(time(NULL));
	RANDOM_CONSTANT = rand();

	const char* name = get_system_name();

	if (name[0] != '\0') {
		init_font_face();
	}

	read_png(logo_path, &png_ptr, &info_ptr, &end_info);
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

	for (int tick = 0;; tick++) {
		clock_t start = clock();

		draw(tick, width, height, fbp1->vaddr, bg_buffer, png_ptr, info_ptr, name, face);
		drmModeSetCrtc(dev_file, crtc_id, fbp1->fb_id, 0, 0, &connector_id, 1, &connector->modes[0]);

		// Swap buffers
		fb_info *tmp = fbp1;
		fbp1 = fbp2;
		fbp2 = tmp;

		usleep((1000000 / FPS) - (clock() - start) * 1000000 / CLOCKS_PER_SEC);
	}
}


void stop() {
	FILE* pid_fp = fopen(PID_FILE, "r");

	if (!pid_fp) {
		fprintf(stderr, "Cannot open '" PID_FILE "'\n");
		exit(EXIT_FAILURE);
	}

	pid_t pid;
	if (fscanf(pid_fp, "%d", &pid) != 1 || pid <= 0) {
		fprintf(stderr, "Cannot stop process: invalid PID");
		exit(EXIT_FAILURE);
	}

	kill(pid, SIGTERM);
}


int main(int argc, const char* argv[]) {
	parse_args(argc, argv);
	read_config();

	switch (action) {
		case START: start(); break;
		case STOP: stop(); break;
	}

	return EXIT_SUCCESS;
}
