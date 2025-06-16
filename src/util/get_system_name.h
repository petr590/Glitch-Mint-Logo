#ifndef GML_UTIL_GET_SYSTEM_NAME_H
#define GML_UTIL_GET_SYSTEM_NAME_H

#include <stdio.h>
#include <string.h>

#define COMMAND "lsb_release -sd"

static const char* get_system_name(void) {
	FILE* fp = popen(COMMAND, "r");

	if (!fp) {
		perror("Cannot execute `" COMMAND "`");
		return "";
	}

	static char name[64];
	const char* res = fgets(name, sizeof(name), fp);
	pclose(fp);

	if (!res) {
		perror("`" COMMAND "` returns empty string");
		return "";
	}

	char* end = strchr(name, '\n');
	if (end) {
		end[0] = '\0';
	}
	
	return name;
}

#undef COMMAND

#endif