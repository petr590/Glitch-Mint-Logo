#include <stdint.h>
#include <libpng/png.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define GLYTH_HEIGHT 20
#define SPACE_WIDTH 12

typedef uint32_t color_t;
extern int RANDOM_CONSTANT;

void draw(int tick, uint32_t width, uint32_t height, color_t* frame, color_t* bg_buffer,
		const png_struct* png_ptr, const png_info* info_ptr,
		const char* name, FT_Face face);