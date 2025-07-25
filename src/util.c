#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/stat.h>

#define PID_FILE "/run/glitch-mint-logo/pid"

double get_time_in_secs(void) {
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time.tv_sec + time.tv_nsec * 1e-9;
}

void create_dirs(const char* path) {
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


pid_t read_pid(void) {
	FILE* fp = fopen(PID_FILE, "r");

	if (!fp) {
		fprintf(stderr, "Cannot open '" PID_FILE "'\n");
		exit(EXIT_FAILURE);
	}

	pid_t pid;
	if (fscanf(fp, "%d", &pid) != 1 || pid <= 0) {
		fprintf(stderr, "Invalid PID\n");
		exit(EXIT_FAILURE);
	}

	fclose(fp);

	return pid;
}


void write_pid(pid_t pid) {
	create_dirs(PID_FILE);
	
	FILE* fp = fopen(PID_FILE, "w");
	if (!fp) {
		fprintf(stderr, "Cannot open '" PID_FILE "'\n");
		exit(EXIT_FAILURE);
	}

	fprintf(fp, "%d", pid);
	fclose(fp);
}
