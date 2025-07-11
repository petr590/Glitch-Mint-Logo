#include "drm.h"
#include "args.h"
#include "modules.h"
#include "read_config.h"
#include "signal_handlers.h"

#include "metric.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>


// ------------------------------------------ release -------------------------------------------

static void release_all(void) {
	if (cleanup_before_drm) cleanup_before_drm();
	cleanup_drm();
	if (cleanup) cleanup();
	cleanup_paths();
}


// ----------------------------------------- metrics ------------------------------------------


static metric_t fps_metric, draw_time_metric, drm_time_metric;

static void print_staticstics(void) {
	metric_print(&fps_metric);
	printf("\n");
	metric_print(&draw_time_metric);
	printf("\n");
	metric_print(&drm_time_metric);
}


// ----------------------------------------- start, stop ------------------------------------------

static void start(void) {
	read_config_file(config_file);
	
	setup();
	init_drm(card_path);
	
	uint32_t connector_id = resources->connectors[0];
	const uint32_t crtc_id = resources->crtcs[0];

	const drmModeModeInfoPtr mode = &connector->modes[0];
	
	if (fps == 0) {
		fps = (double)(mode->clock * 1000) / (mode->htotal * mode->vtotal);
	}
	
	const uint32_t width = mode->hdisplay;
	const uint32_t height = mode->vdisplay;
	
	setup_after_drm(width, height);
	
	pid_t deamon_pid = fork();
	if (deamon_pid) {
		write_pid(deamon_pid);
		return;
	}
	
	add_signal_handlers(release_all);
	atexit(release_all);
	atexit(print_staticstics);
	
	
	const fb_info* fbp1 = fb_info1;
	const fb_info* fbp2 = fb_info2;

	metric_init(&fps_metric, "FPS", "");
	metric_init(&draw_time_metric, "draw time", "ms");
	metric_init(&drm_time_metric, "drm time", "ms");

	const double frame_time = 1 / fps;
	
	for (int tick = 0; !stopped; tick++) {
		double start = get_time_in_secs();

		draw(tick, width, height, fbp1->vaddr);

		double draw_end = get_time_in_secs();

		drmModeSetCrtc(card_file, crtc_id, fbp1->fb_id, 0, 0, &connector_id, 1, &connector->modes[0]);

		double drm_end = get_time_in_secs();

		// Swap buffers
		const fb_info* tmp = fbp1;
		fbp1 = fbp2;
		fbp2 = tmp;

		metric_add(&draw_time_metric, (draw_end - start) * 1000);
		metric_add(&drm_time_metric,  (drm_end - draw_end) * 1000);

		double elapsed = get_time_in_secs() - start;
		metric_add(&fps_metric, 1 / elapsed);

		if (stopped) break;
		
		usleep(1e6 * fmax(0, frame_time - elapsed));
	}
}


static void notify_service_loaded(void) {
	read_config_file(config_file);

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
