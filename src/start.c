#include "start.h"
#include "args.h"
#include "modules.h"
#include "read_config.h"
#include "signal_handlers.h"
#include "metric.h"
#include "drm.h"
#include "util.h"
#include "util/util.h"

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define UNUSED(v) (void)(v)

#define MAX_BOOT_TIMES_COUNT 10

#define FFMPEG_OUT_FILE "/tmp/gml-demo.mp4"
#define FFMPEG_CMD "ffmpeg -hide_banner -f rawvideo -pix_fmt bgra -s %ux%u -r %.2f -i - -c:v libx264 -pix_fmt yuv420p -preset ultrafast -y " FFMPEG_OUT_FILE
#define FFMPEG_CMD_BUFFER_SIZE (sizeof(FFMPEG_CMD) - 4 + 20 /* %u%u */ - 4 + 7 /* %.2f */)

// ------------------------------------------- metrics --------------------------------------------


static metric_t fps_metric         = METRIC_INITIALIZER("FPS", "");
static metric_t draw_time_metric   = METRIC_INITIALIZER("draw time", "ms");
static metric_t drm_time_metric    = METRIC_INITIALIZER("drm time", "ms");
static metric_t ffmpeg_time_metric = METRIC_INITIALIZER("ffmpeg time", "ms");

static void print_staticstics(void) {
	metric_print(&fps_metric);
	printf("\n");
	metric_print(&draw_time_metric);
	printf("\n");
	metric_print(&drm_time_metric);
}

// ------------------------------------------ boot time -------------------------------------------

static int read_boot_times(double* boot_times) {
	FILE* fp = fopen(boot_timings_path, "r");

	if (!fp) {
		if (errno != ENOENT) {
			fprintf(stderr, "Cannot open '%s': skipping\n", boot_timings_path);
		}

		return 0;
	}

	int count = 0;
	while (count < MAX_BOOT_TIMES_COUNT && fscanf(fp, "%lf", &boot_times[count]) == 1) {
		count++;
	}

	fclose(fp);
	return count;
}

static double get_supposed_time(const double* boot_times, int count) {
	if (count == 0) {
		return 0;
	}

	double sum_time = 0;
	for (int i = 0; i < count; i++) {
		sum_time += boot_times[i];
	}

	return count == 0 ? 0 : sum_time / count;
}

static void write_boot_times(const double* boot_times, int count, double new_boot_time) {
	create_dirs(boot_timings_path);

	FILE* fp = fopen(boot_timings_path, "w");

	if (!fp) {
		fprintf(stderr, "Cannot open '%s' (skipping): %s\n", boot_timings_path, strerror(errno));
		return;
	}

	for (int i = count < MAX_BOOT_TIMES_COUNT ? 0 : 1; i < count; i++) {
		fprintf(fp, "%lf\n", boot_times[i]);
	}

	fprintf(fp, "%lf\n", new_boot_time);
	fclose(fp);
}

// -------------------------------------------- render --------------------------------------------

static const fb_info* fb_front1;
static const fb_info* fb_front2;
static const fb_info* fb_back;

static volatile bool front_ready;
static volatile bool back_ready;
static pthread_cond_t signal_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t signal_mutex = PTHREAD_MUTEX_INITIALIZER;

static volatile bool buffer_swapped;
static pthread_mutex_t buffer_swap_mutex = PTHREAD_MUTEX_INITIALIZER;

/// @brief true, когда первый поток увидел переменную stopped. Данная переменная необходима для корректного завершения потоков.
/// Без неё переменная stopped может установиться между критическими секциями buffer_swap_mutex двух потоков, из-за чего первый
// /поток не увидит её, а второй - увидит. На следующей же итерации случится deadlock, так как первый поток будет ждать сигнал
/// от второго, уже завершившегося потока. А с этой переменной потоки всегда будут выходить синхронно.
static volatile bool first_thread_stopped;

static _Thread_local bool is_front_thread;

/// @brief ожидает другой поток. Если другой поток уже готов, то посылает ему сигнал. После этого делает свап буфферов и проверяет флаг stopped.
/// @return true, если нужно завершить текущий поток. Иначе false.
static bool wait_and_swap_buffers(volatile bool* self_ready, const volatile bool* other_ready) {
	pthread_mutex_lock(&signal_mutex);
	*self_ready = true;

	if (*other_ready) {
		pthread_cond_signal(&signal_cond);
		
	} else {
		struct timespec wait_time;
		clock_gettime(CLOCK_REALTIME, &wait_time);
    	wait_time.tv_sec += 1;

		if (pthread_cond_timedwait(&signal_cond, &signal_mutex, &wait_time) == ETIMEDOUT) {
			*self_ready = false;
			fprintf(stderr, "Warning: %s thread skipped by timeout\n", is_front_thread ? "back" : "front");
		}

		// pthread_cond_wait(&signal_cond, &signal_mutex); // Этот способ работает идеально, но зависает при зависании другого потока
	}

	pthread_mutex_unlock(&signal_mutex);


	bool ret = false;

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
			ret = true;
		}

	} else {
		buffer_swapped = false;
		
		if (first_thread_stopped) {
			ret = true;
		}
	}

	pthread_mutex_unlock(&buffer_swap_mutex);
	return ret;
}


static void* render_front(void* data) {
	UNUSED(data);

	is_front_thread = true;

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


static FILE* ffmpeg_pipe;
static pthread_t drm_thread;

static void render_back(double supposed_time) {
	const uint16_t width = connector->modes[0].hdisplay;
	const uint16_t height = connector->modes[0].vdisplay;

	const double frame_time = 1.0 / fps;
	
	for (int tick = 0;; tick++) {
		double draw_start = get_time_in_secs();
		
		draw(tick, width, height, fb_back->vaddr, supposed_time);

		double draw_end = get_time_in_secs();
		metric_add(&draw_time_metric, (draw_end - draw_start) * 1000);

		if (ffmpeg_pipe) {
			double ffmpeg_start = get_time_in_secs();

			fwrite(fb_back->vaddr, 1, width * height * sizeof(color_t), ffmpeg_pipe);
			fflush(ffmpeg_pipe);

			double ffmpeg_end = get_time_in_secs();
			metric_add(&ffmpeg_time_metric, (ffmpeg_end - ffmpeg_start) * 1000);
		}

		metric_add(&fps_metric, 1 / (get_time_in_secs() - draw_start));

		usleep(1e6 * fmax(0, draw_start + frame_time - get_time_in_secs()));

		if (wait_and_swap_buffers(&back_ready, &front_ready)) break;
	}
}


// -------------------------------------------- start ---------------------------------------------

static void release_all(void) {
	fprintf(stderr, "release_all\n");

	if (ffmpeg_pipe) {
		pclose(ffmpeg_pipe);
		ffmpeg_pipe = NULL;
	}

	fprintf(stderr, "A\n");

	if (!is_front_thread && drm_thread) {
		pthread_join(drm_thread, NULL);
		drm_thread = 0;
	}

	fprintf(stderr, "B\n");

	if (cleanup_before_drm) cleanup_before_drm();
	fprintf(stderr, "C\n");
	cleanup_drm();
	fprintf(stderr, "D\n");
	if (cleanup) cleanup();
	fprintf(stderr, "E\n");
	cleanup_paths();

	fprintf(stderr, "release_all end\n");
}

static void release_all_and_exit(void) {
	fprintf(stderr, "release_all_and_exit\n");
	release_all();
	fprintf(stderr, "release_all_and_exit before end\n");
	_exit(1);
	fprintf(stderr, "release_all_and_exit end\n");
}


void start_render(void) {
	read_config_file(config_file);
	
	setup();
	init_drm(card_path);

	const drmModeModeInfoPtr mode = &connector->modes[0];
	
	if (fps == 0) {
		fps = (double)(mode->clock * 1000) / (mode->htotal * mode->vtotal);
	}
	
	const uint16_t width = mode->hdisplay;
	const uint16_t height = mode->vdisplay;
	setup_after_drm(width, height);
	
	const pid_t deamon_pid = fork();
	if (deamon_pid) {
		write_pid(deamon_pid);
		return;
	}
	
	add_error_signal_handler(release_all_and_exit);
	atexit(release_all);
	atexit(print_staticstics);
	
	fb_front1 = fb_info1;
	fb_front2 = fb_info2;
	fb_back = fb_info3;

	if (record_video) {
		const double local_fps = fclamp(fps, 0, 1000);

		char command[FFMPEG_CMD_BUFFER_SIZE];
		sprintf(command, FFMPEG_CMD, width, height, local_fps);
		ffmpeg_pipe = popen(command, "w");

		if (!ffmpeg_pipe) {
			perror("Failed to open pipe to ffmpeg");
			exit(EXIT_FAILURE);
		}
	}

	double boot_times[MAX_BOOT_TIMES_COUNT];
	const int boot_times_count = read_boot_times(boot_times);

	const int ret = pthread_create(&drm_thread, NULL, render_front, NULL);
	if (ret) {
		fprintf(stderr, "Failed to create thread\n");
		exit(EXIT_FAILURE);
	}

	const double supposed_time = get_supposed_time(boot_times, boot_times_count);
	const double start_time = get_time_in_secs();

	render_back(supposed_time);

	const double end_time = get_time_in_secs();
	write_boot_times(boot_times, boot_times_count, end_time - start_time);

	pthread_join(drm_thread, NULL);
	drm_thread = 0;
}
