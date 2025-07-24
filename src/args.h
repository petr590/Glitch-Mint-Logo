#ifndef GML_ARGS_H
#define GML_ARGS_H

#include <stdbool.h>

typedef enum {
	START,
	STOP,
	NOTIFY_SERVICE_LOADED,
} action_t;

extern action_t action;
extern const char* config_file;
extern const char* notify_service_name;
extern bool record_video;

void parse_args(int argc, const char* argv[]);

#endif