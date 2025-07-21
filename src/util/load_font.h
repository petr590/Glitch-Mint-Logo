#ifndef GML_UTIL_LOAD_FONT_H
#define GML_UTIL_LOAD_FONT_H

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

FT_Library init_freetype_lib_or_exit(void);
FT_Face load_freetype_face_or_exit(FT_Library library, const char* path, FT_UInt pixel_height);

#endif