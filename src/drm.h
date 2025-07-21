#ifndef GML_DRM_H
#define GML_DRM_H

#include "common.h"
#include <xf86drm.h>
#include <xf86drmMode.h>

typedef struct {
	uint32_t size;
	uint32_t handle;
	uint32_t fb_id;
	color_t* vaddr;
} fb_info;


extern int card_file;
extern drmModeRes* resources;
extern drmModeConnector* connector;
extern fb_info* fb_info1;
extern fb_info* fb_info2;
extern fb_info* fb_info3;

void init_drm(const char* card_path);
void cleanup_drm(void);

#endif