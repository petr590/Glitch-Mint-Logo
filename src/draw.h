#include <libpng/png.h>
#include <stdint.h>

typedef uint32_t color_t;
extern int RANDOM_CONSTANT;

void draw(int tick, uint32_t width, uint32_t height, uint8_t *frame, color_t* bg_buffer, const png_struct* png_ptr, const png_info* info_ptr);