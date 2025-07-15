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
#include <pthread.h>
#include <stdatomic.h>


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

static const fb_info* fb_front;
static const fb_info* fb_back;

static atomic_bool front_ready;
static atomic_bool back_ready;
static pthread_mutex_t signal_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t signal_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t buffer_swap_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool buffer_swapped;

static bool first_thread_stopped;


static bool wait_and_swap_buffers(atomic_bool* restrict self_ready, atomic_bool* restrict other_ready) {
	pthread_mutex_lock(&signal_mutex);
	*self_ready = true;

	if (*other_ready) {
		pthread_cond_signal(&signal_cond);
	} else {
		pthread_cond_wait(&signal_cond, &signal_mutex);
	}

	pthread_mutex_unlock(&signal_mutex);


	pthread_mutex_lock(&buffer_swap_mutex);

	if (!buffer_swapped) {
		printf("swap\n");

		const fb_info* tmp = fb_front;
		fb_front = fb_back;
		fb_back = tmp;

		front_ready = false;
		back_ready = false;

		buffer_swapped = true;

		if (stopped) {
			first_thread_stopped = true;
			pthread_mutex_unlock(&buffer_swap_mutex);
			return true;
		}

	} else {
		buffer_swapped = false;

		if (first_thread_stopped) {
			pthread_mutex_unlock(&buffer_swap_mutex);
			return true;
		}
	}

	pthread_mutex_unlock(&buffer_swap_mutex);
	return false;
}


static void* render_front_buffer(void* ignored) {
	uint32_t connectors[] = { resources->connectors[0] };
	const uint32_t crtc_id = resources->crtcs[0];
	const drmModeModeInfoPtr mode = &connector->modes[0];

	for (;;) {
		double start = get_time_in_secs();
		printf("drm\n");

		drmModeSetCrtc(
				card_file, crtc_id, fb_front->fb_id, 0, 0,
				connectors, sizeof(connectors) / sizeof(connectors[0]), mode
		);
		
		double end = get_time_in_secs();
		metric_add(&drm_time_metric, (end - start) * 1000);

		if (wait_and_swap_buffers(&front_ready, &back_ready)) break;
	}

	return NULL;
}

static void render_back_buffer(void) {
	const uint32_t width = connector->modes[0].hdisplay;
	const uint32_t height = connector->modes[0].vdisplay;

	const double frame_time = 1 / fps;
	
	for (int tick = 0;; tick++) {
		double start = get_time_in_secs();
		printf("draw\n");
		
		draw(tick, width, height, fb_back->vaddr);

		double end = get_time_in_secs();
		metric_add(&draw_time_metric, (end - start) * 1000);
		metric_add(&fps_metric, 1 / (end - start));

		usleep(1e6 * fmax(0, start + frame_time - get_time_in_secs()));

		if (wait_and_swap_buffers(&back_ready, &front_ready)) break;
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
	
	fb_front = fb_info1;
	fb_back = fb_info2;

	pthread_t drm_thread;
	int res = pthread_create(&drm_thread, NULL, render_front_buffer, NULL);
	if (res) {
		fprintf(stderr, "Cannot create thread: %d\n", res);
		exit(EXIT_FAILURE);
	}
	
	render_back_buffer();

	pthread_join(drm_thread, NULL);
}


// ------------------------------------ notify_service_loaded -------------------------------------

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
