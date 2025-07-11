#ifndef GML_READ_CONFIG_H
#define GML_READ_CONFIG_H

extern const char* module_name;
extern const char* card_path;
extern const char* socket_path;

void read_config_file(const char* filename);
void cleanup_paths(void);

#endif