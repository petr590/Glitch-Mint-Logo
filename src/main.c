#include "drm.h"
#include "args.h"
#include "modules.h"
#include "read_config.h"
#include "signal_handlers.h"
#include "metric.h"
#include "util.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>


// ------------------------------------------- release --------------------------------------------

static void release_all(void) {
	if (cleanup_before_drm) cleanup_before_drm();
	cleanup_drm();
	if (cleanup) cleanup();
	cleanup_paths();
}


// ------------------------------------------- metrics --------------------------------------------


static metric_t fps_metric       = METRIC_INITIALIZER("FPS", "");
static metric_t draw_time_metric = METRIC_INITIALIZER("draw time", "ms");
static metric_t drm_time_metric  = METRIC_INITIALIZER("drm time", "ms");

static void print_staticstics(void) {
	metric_print(&fps_metric);
	printf("\n");
	metric_print(&draw_time_metric);
	printf("\n");
	metric_print(&drm_time_metric);
}


// ----------------------------------------- start, stop ------------------------------------------

static void render(void) {
	uint32_t connectors[] = { resources->connectors[0] };
	const uint32_t crtc_id = resources->crtcs[0];
	const drmModeModeInfoPtr mode = &connector->modes[0];

	const uint32_t width = mode->hdisplay;
	const uint32_t height = mode->vdisplay;

	const double frame_time = 1 / fps;

	const fb_info* fb_front = fb_info1;
	const fb_info* fb_back = fb_info2;
	
	for (int tick = 0; !stopped; tick++) {
		double draw_start = get_time_in_secs();
		draw(tick, width, height, fb_back->vaddr);
		double draw_end = get_time_in_secs();

		metric_add(&draw_time_metric, (draw_end - draw_start) * 1000);

		const fb_info* tmp = fb_front;
		fb_front = fb_back;
		fb_back = tmp;


		double drm_start = get_time_in_secs();
		drmModeSetCrtc(
				card_file, crtc_id, fb_front->fb_id, 0, 0,
				connectors, sizeof(connectors) / sizeof(connectors[0]), mode
		);
		double drm_end = get_time_in_secs();

		metric_add(&drm_time_metric, (drm_end - drm_start) * 1000);
		metric_add(&fps_metric, 1 / (drm_end - draw_start));

		if (stopped) break;

		usleep(1e6 * fmax(0, draw_start + frame_time - get_time_in_secs()));
	}
}


static void start(void) {
	read_config_file(config_file);
	
	setup();
	init_drm(card_path);

	const drmModeModeInfoPtr mode = &connector->modes[0];
	
	if (fps == 0) {
		fps = (double)(mode->clock * 1000) / (mode->htotal * mode->vtotal);
	}
	
	setup_after_drm(mode->hdisplay, mode->vdisplay);
	
	pid_t deamon_pid = fork();
	if (deamon_pid) {
		write_pid(deamon_pid);
		return;
	}
	
	add_signal_handlers(release_all);
	atexit(release_all);
	atexit(print_staticstics);
	
	render();
}


// ------------------------------------ notify_service_loaded -------------------------------------

static void notify_service_loaded(void) {
	read_config_file(config_file);

	if (access(socket_path, F_OK) != 0) {
		printf("File '%s' does not exists, skipping\n", socket_path);
		return;
	}

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (fd < 0) {
		perror("Failed to open socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_path);
	socklen_t len = offsetof(struct sockaddr_un, sun_path) + strlen(socket_path) + 1;

	int ret = connect(fd, (struct sockaddr*) &addr, len);
	if (ret < 0) {
		close(fd);
		fprintf(stderr, "Failed to connect to '%s': %s\n", addr.sun_path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("Writing to socket: '%s'\n", notify_service_name);
	write(fd, notify_service_name, strlen(notify_service_name) + 1);
	
	close(fd);
}


int main(int argc, const char* argv[]) {
	parse_args(argc, argv);

	switch (action) {
		case START:
			start();
			break;
		
		case STOP:
			kill(read_pid(), SIGTERM);
			break;
		
		case NOTIFY_SERVICE_LOADED:
			notify_service_loaded();
			break;
	}

	return EXIT_SUCCESS;
}
