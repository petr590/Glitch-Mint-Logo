#include "args.h"
#include "modules.h"
#include "read_config.h"
#include "start.h"
#include "util.h"

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>


#define CHECK_STOPPED_INTERVAL_MCS 100000 // 0.1 sec


static void stop_daemon(void) {
	pid_t pid = read_pid();
	int ret = kill(pid, SIGTERM);

	while (ret && errno == ESRCH) {
		usleep(CHECK_STOPPED_INTERVAL_MCS);
		ret = kill(pid, 0);
	}
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
			start_render();
			break;
		
		case STOP:
			stop_daemon();
			break;
		
		case NOTIFY_SERVICE_LOADED:
			notify_service_loaded();
			break;
	}

	return EXIT_SUCCESS;
}
