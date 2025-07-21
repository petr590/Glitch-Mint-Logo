#include "start.h"
#include "args.h"
#include "modules.h"
#include "read_config.h"
#include "signal_handlers.h"
#include "metric.h"
#include "drm.h"
#include "util.h"

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>


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

// -------------------------------------------- render --------------------------------------------

static const fb_info* fb_front1;
static const fb_info* fb_front2;
static const fb_info* fb_back;

static volatile bool front_ready;
static volatile bool back_ready;

static pthread_cond_t signal_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t signal_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t buffer_swap_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile bool buffer_swapped;
static volatile bool first_thread_stopped;

static bool wait_and_swap_buffers(volatile bool* self_ready, const volatile bool* other_ready) {
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

		const fb_info* tmp = fb_front1;
		fb_front1 = fb_front2;
		fb_front2 = fb_back;
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


static void* render_front(void* ignored) {
	uint32_t connectors[] = { resources->connectors[0] };
	const uint32_t crtc_id = resources->crtcs[0];
	const drmModeModeInfoPtr mode = &connector->modes[0];

	for (;;) {
		double start = get_time_in_secs();

		drmModeSetCrtc(
				card_file, crtc_id, fb_front2->fb_id, 0, 0,
				connectors, sizeof(connectors) / sizeof(connectors[0]), mode
		);
		
		double end = get_time_in_secs();
		metric_add(&drm_time_metric, (end - start) * 1000);

		if (wait_and_swap_buffers(&front_ready, &back_ready)) break;
	}

	return NULL;
}


static void render_back(void) {
	const uint32_t width = connector->modes[0].hdisplay;
	const uint32_t height = connector->modes[0].vdisplay;

	const double frame_time = 1 / fps;
	
	for (int tick = 0;; tick++) {
		double start = get_time_in_secs();
		
		draw(tick, width, height, fb_back->vaddr);

		double end = get_time_in_secs();
		metric_add(&draw_time_metric, (end - start) * 1000);
		metric_add(&fps_metric, 1 / (end - start));

		usleep(1e6 * fmax(0, start + frame_time - get_time_in_secs()));

		if (wait_and_swap_buffers(&back_ready, &front_ready)) break;
	}
}


// -------------------------------------------- start ---------------------------------------------


static void release_all(void) {
	if (cleanup_before_drm) cleanup_before_drm();
	cleanup_drm();
	if (cleanup) cleanup();
	cleanup_paths();
}


void start_render(void) {
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
	
	fb_front1 = fb_info1;
	fb_front2 = fb_info2;
	fb_back = fb_info3;

	pthread_t drm_thread;
	int ret = pthread_create(&drm_thread, NULL, render_front, NULL);
	if (ret) {
		fprintf(stderr, "Failed to create thread\n");
		exit(EXIT_FAILURE);
	}

	render_back();

	pthread_join(drm_thread, NULL);
}