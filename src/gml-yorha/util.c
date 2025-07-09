#include <stdio.h>

static int __sdbus_perror_and_exit_if_ret(int ret, const char* file, int linenum, const char* func) {
	if (ret < 0) {
		fprintf(stderr, "SD-Bus error: %s:%d: %s returned %s\n", file, linenum, func, strerror(-ret));
		exit(-ret);
	}

	return ret;
}

#define SDBUS_EXIT_IF_ERROR(val) __sdbus_perror_and_exit_if_ret(val, __FILE__, __LINE__, #val)