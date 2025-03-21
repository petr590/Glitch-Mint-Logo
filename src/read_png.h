#include <libpng/png.h>

#define ERROR_READ_PNG 35

int read_png(const char* filename, png_structp* res_png_ptr, png_infop* res_info_ptr, png_infop* res_end_info);