#ifndef GML_UTIL_H
#define GML_UTIL_H

#include <sys/types.h>

double get_time_in_secs(void);

void create_dirs(const char* path);

pid_t read_pid(void);
void write_pid(pid_t pid);

#endif
