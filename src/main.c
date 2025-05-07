#include "drm_fb.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <assert.h>

#define PID_FILE "/run/glitch-mint-logo/pid"


// ---------------------------------------- dynamic module ----------------------------------------

static const char *module_filename;
static void (*read_config)(config_t*);
static void (*setup)(void);
static void (*setup_after_drm)(uint32_t, uint32_t);
static void (*cleanup_before_drm)(void);
static void (*cleanup)(void);
static void (*draw)(int, uint32_t, uint32_t, color_t*);

static void* load_sym(void* handle, const char* id) {
	void* sym = dlsym(handle, id);

	char* error;
	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "Cannot load symbol %s from '%s': %s\n", id, module_filename, error);
		exit(EXIT_FAILURE);
	}

	return sym;
}

static void load_module(void) {
	void* handle = dlopen(module_filename, RTLD_LAZY);

	if (!handle) {
		fprintf(stderr, "Cannot load dynamic library: %s\n", module_filename, dlerror());
		exit(EXIT_FAILURE);
	}

	read_config        = load_sym(handle, "glspl_read_config");
	setup              = load_sym(handle, "glspl_setup");
	setup_after_drm    = load_sym(handle, "glspl_setup_after_drm");
	cleanup_before_drm = load_sym(handle, "glspl_setup_after_drm");
	cleanup            = load_sym(handle, "glspl_cleanup");
	draw               = load_sym(handle, "glspl_draw");
}


// ------------------------------------------ resources -------------------------------------------

static const char *dev_path;

static int dev_file = -1;
static drmModeRes *resources;
static drmModeConnector *connector;
static drmModeCrtc *saved_crtc;
static fb_info *fb_info1, *fb_info2;


static void init_drm(void) {
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


static void release_all(void) {
	if (cleanup_before_drm) cleanup_before_drm();

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

	if (cleanup) cleanup();

	if (dev_path)        { free((void*) dev_path);        dev_path = NULL; }
	if (module_filename) { free((void*) module_filename); module_filename = NULL; }
}

static void on_signal(int sig) {
	printf("Exited by signal %d\n", sig);
	signal(sig, SIG_DFL);
	exit((sig == SIGABRT || sig == SIGSEGV || sig == SIGFPE) ? EXIT_FAILURE : EXIT_SUCCESS);
}


// ----------------------------------------- args, config -----------------------------------------

static enum { START, STOP } action = START;

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

static void read_config_file(void) {
	config_t cfg; 
	config_init(&cfg);

	if (!config_read_file(&cfg, CONFIG_FILE)) {
		fprintf(stderr, "Cannot load config: %s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		exit(EXIT_FAILURE);
	}

	dev_path = read_config_str(&cfg, "dev_path");
	module_filename = read_config_str(&cfg, "module");

	load_module();
	read_config(&cfg);

	config_destroy(&cfg);
}


// ----------------------------------------- start, stop ------------------------------------------

static void start(void) {
	if (fork()) return;

	signal(SIGINT,  on_signal);
	signal(SIGABRT, on_signal);
	signal(SIGFPE,  on_signal);
	signal(SIGSEGV, on_signal);
	signal(SIGTERM, on_signal);
	atexit(release_all);
	
	create_dirs(PID_FILE);
	
	FILE* pid_fp = fopen(PID_FILE, "w");
	if (!pid_fp) {
		fprintf(stderr, "Cannot open '" PID_FILE "'\n");
		exit(EXIT_FAILURE);
	}

	fprintf(pid_fp, "%d", getpid());
	fclose(pid_fp);

	read_config_file();
	setup();
	init_drm();

	uint32_t connector_id = resources->connectors[0];
	const uint32_t crtc_id = resources->crtcs[0];
	const uint32_t width = connector->modes[0].hdisplay;
	const uint32_t height = connector->modes[0].vdisplay;
	
	fb_info1 = create_fb(dev_file, width, height);
	fb_info2 = create_fb(dev_file, width, height);
	
	fb_info *fbp1 = fb_info1,
			*fbp2 = fb_info2;
	
	setup_after_drm(width, height);
	
	for (int tick = 0;; tick++) {
		clock_t start = clock();

		draw(tick, width, height, fbp1->vaddr);
		drmModeSetCrtc(dev_file, crtc_id, fbp1->fb_id, 0, 0, &connector_id, 1, &connector->modes[0]);

		// Swap buffers
		fb_info *tmp = fbp1;
		fbp1 = fbp2;
		fbp2 = tmp;

		usleep((1000000 / FPS) - (clock() - start) * 1000000 / CLOCKS_PER_SEC);
	}
}


static void stop(void) {
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

	switch (action) {
		case START: start(); break;
		case STOP: stop(); break;
	}

	return EXIT_SUCCESS;
}
