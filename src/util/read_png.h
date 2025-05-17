#ifndef GML_UTIL_READ_PNG_H
#define GML_UTIL_READ_PNG_H

#include <libpng/png.h>

void read_png(const char* filename, png_structp* res_png_ptr, png_infop* res_info_ptr, png_infop* res_end_info);

#endif