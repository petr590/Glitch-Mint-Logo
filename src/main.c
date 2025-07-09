#include "drm_fb.h"
#include "metric.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>


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
		fprintf(stderr, "Cannot load dynamic library %s: %s\n", module_filename, dlerror());
		exit(EXIT_FAILURE);
	}

	read_config        = load_sym(handle, "gml_read_config");
	setup              = load_sym(handle, "gml_setup");
	setup_after_drm    = load_sym(handle, "gml_setup_after_drm");
	cleanup_before_drm = load_sym(handle, "gml_setup_after_drm");
	cleanup            = load_sym(handle, "gml_cleanup");
	draw               = load_sym(handle, "gml_draw");
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
		fprintf(stderr, "Cannot open '%s': %s\n", dev_path, strerror(errno));
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
	exit((sig == SIGILL || sig == SIGABRT || sig == SIGSEGV || sig == SIGFPE) ? EXIT_FAILURE : EXIT_SUCCESS);
}


// ----------------------------------------- args, config -----------------------------------------

static enum { START, STOP } action = START;
static const char* config_file = CONFIG_FILE;

static void print_usage_and_exit(const char* argv[]) {
	fprintf(stderr, "Usage: %s [--stop] [--config <file>] [--mode boot|reboot|shutdown]\n", argv[0]);
	exit(EINVAL);
}

static void parse_args(int argc, const char* argv[]) {

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--stop") == 0) {
			action = STOP;
			continue;
		}
		
		if (strcmp(argv[i], "--config") == 0) {
			if (++i >= argc)
				print_usage_and_exit(argv);

			config_file = argv[i];
			continue;
		}
		
		if (strcmp(argv[i], "--mode") == 0) {
			if (++i >= argc)
				print_usage_and_exit(argv);
			
			const char* mode = argv[i];

			if (strcmp(mode, "boot")     == 0) { boot_mode = BOOT_MODE_BOOT;     continue; }
			if (strcmp(mode, "reboot")   == 0) { boot_mode = BOOT_MODE_REBOOT;   continue; }
			if (strcmp(mode, "shutdown") == 0) { boot_mode = BOOT_MODE_SHUTDOWN; continue; }
		}

		print_usage_and_exit(argv);
	}
}



static void read_config_file(void) {
	config_t cfg; 
	config_init(&cfg);

	if (!config_read_file(&cfg, config_file)) {
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


static double start_time;
static metric_t fps_metric, draw_time_metric, drm_time_metric;

static void print_staticstics(void) {
	time_t timestamp = time(NULL) - (time_t)((get_time_in_secs() - start_time) * 1000);
	struct tm* start_tm = localtime(&timestamp);
	printf("Started at %s\n", asctime(start_tm));
	
	metric_print(&fps_metric);
	printf("\n");
	metric_print(&draw_time_metric);
	printf("\n");
	metric_print(&drm_time_metric);
}

static void start(void) {
	if (fork()) return;

	signal(SIGINT,  on_signal);
	signal(SIGILL,  on_signal);
	signal(SIGABRT, on_signal);
	signal(SIGFPE,  on_signal);
	signal(SIGSEGV, on_signal);
	signal(SIGTERM, on_signal);
	atexit(release_all);
	atexit(print_staticstics);
	
	write_pid();
	read_config_file();
	setup();
	init_drm();

	uint32_t connector_id = resources->connectors[0];
	const uint32_t crtc_id = resources->crtcs[0];

	const drmModeModeInfoPtr mode = &connector->modes[0];

	if (fps == 0) {
		fps = (double)(mode->clock * 1000) / (mode->htotal * mode->vtotal);
	}
	
	const uint32_t width = mode->hdisplay;
	const uint32_t height = mode->vdisplay;
	
	fb_info1 = create_fb(dev_file, width, height);
	fb_info2 = create_fb(dev_file, width, height);
	
	const fb_info* fbp1 = fb_info1;
	const fb_info* fbp2 = fb_info2;
	
	setup_after_drm(width, height);

	metric_init(&fps_metric, "FPS", "");
	metric_init(&draw_time_metric, "draw time", "ms");
	metric_init(&drm_time_metric, "drm time", "ms");

	const double frame_time = 1 / fps;
	
	start_time = get_time_in_secs();
	
	for (int tick = 0;; tick++) {
		double start = get_time_in_secs();

		draw(tick, width, height, fbp1->vaddr);

		double draw_end = get_time_in_secs();

		drmModeSetCrtc(dev_file, crtc_id, fbp1->fb_id, 0, 0, &connector_id, 1, &connector->modes[0]);

		double drm_end = get_time_in_secs();

		// Swap buffers
		const fb_info* tmp = fbp1;
		fbp1 = fbp2;
		fbp2 = tmp;

		metric_add(&draw_time_metric, (draw_end - start) * 1000);
		metric_add(&drm_time_metric,  (drm_end - draw_end) * 1000);

		double elapsed = get_time_in_secs() - start;
		metric_add(&fps_metric, 1 / elapsed);

		usleep(1e6 * fmax(0, frame_time - elapsed));
	}
}


static void stop(void) {
	kill(read_pid(), SIGTERM);
}

int main(int argc, const char* argv[]) {
	parse_args(argc, argv);

	switch (action) {
		case START: start(); break;
		case STOP: stop(); break;
	}

	return EXIT_SUCCESS;
}