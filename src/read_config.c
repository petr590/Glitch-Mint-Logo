#include "read_config.h"
#include "modules.h"
#include <stdlib.h>
#include <libconfig.h>

const char* module_name;
const char* card_path;
const char* socket_path;

void read_config_file(const char* filename) {
	config_t cfg; 
	config_init(&cfg);

	if (!config_read_file(&cfg, filename)) {
		fprintf(stderr, "Cannot load config: %s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		exit(EXIT_FAILURE);
	}

	module_name = read_config_str(&cfg, "module");
	card_path    = read_config_str(&cfg, "card_path");
	socket_path = read_config_str(&cfg, "socket_path");

	load_module(module_name);
	read_config(&cfg);

	config_destroy(&cfg);
}

void cleanup_paths(void) {
	if (card_path)    { free((void*) card_path);    card_path    = NULL; }
	if (module_name) { free((void*) module_name); module_name = NULL; }
}