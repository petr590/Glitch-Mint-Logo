#ifndef GML_MODULES_H
#define GML_MODULES_H

#include "common.h"

extern void (*read_config)(config_t*);
extern void (*setup)(void);
extern void (*setup_after_drm)(uint32_t, uint32_t);
extern void (*cleanup_before_drm)(void);
extern void (*cleanup)(void);
extern void (*draw)(int, uint32_t, uint32_t, color_t*);

void load_module(const char* filename);

#endif