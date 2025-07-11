#include "args.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

action_t action = START;
const char* config_file = CONFIG_FILE;
const char* notify_service_name = NULL;

static void print_usage_and_exit(const char* argv[]) {
	fprintf(stderr, "Usage: %s [--config <file>] [--stop | --service-loaded <service>] [--mode boot|reboot|shutdown]\n", argv[0]);
	exit(EINVAL);
}

static void set_action(action_t new_action, const char* argv[]) {
	if (action != START && action != new_action) {
		print_usage_and_exit(argv);
	}

	action = new_action;
}

void parse_args(int argc, const char* argv[]) {

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--stop") == 0) {
			set_action(STOP, argv);
			continue;
		}
		
		if (strcmp(argv[i], "--config") == 0) {
			if (++i >= argc)
				print_usage_and_exit(argv);

			config_file = argv[i];
			continue;
		}

		if (strcmp(argv[i], "--service-loaded") == 0) {
			if (++i >= argc)
				print_usage_and_exit(argv);
			
			set_action(NOTIFY_SERVICE_LOADED, argv);
			notify_service_name = argv[i];
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