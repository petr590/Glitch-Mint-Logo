#include "common.h"
#include <stdlib.h>
#include <string.h>

const char* read_config_str(config_t* cfgp, const char* key) {
	const char* buffer;

	if (config_lookup_string(cfgp, key, &buffer)) {
		return strdup(buffer);
		
	} else {
		fprintf(stderr, "Config file (" CONFIG_FILE ") does not contain '%s' setting\n", key);
		config_destroy(cfgp);
		exit(EXIT_FAILURE);
	}
}