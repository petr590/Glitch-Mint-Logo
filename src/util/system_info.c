#include "system_info.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define COMMAND "lsb_release -sid"
#define BUFFER_SIZE 64


static bool read_line_or_return(char* buffer, FILE* fp, const char* error_message) {
	const char* res = fgets(buffer, BUFFER_SIZE, fp);

	if (!res) {
		pclose(fp);
		perror(error_message);
	}

	return res;
}

void get_system_id_and_name(const char** id, const char** name) {
	*id = NULL;
	*name = NULL;

	FILE* fp = popen(COMMAND, "r");

	if (!fp) {
		perror("Cannot execute `" COMMAND "`");
		return;
	}

	static char id_buffer[BUFFER_SIZE];
	static char name_buffer[BUFFER_SIZE];

	if (!read_line_or_return(id_buffer,   fp, "`" COMMAND "` does not return id"  )) return;
	if (!read_line_or_return(name_buffer, fp, "`" COMMAND "` does not return name")) return;

	char* end = strchr(id_buffer, '\n');
	if (end) {
		end[0] = '\0';
	}

	end = strchr(name_buffer, '\n');
	if (end) {
		end[0] = '\0';
	}

	*id   = id_buffer;
	*name = name_buffer;
}