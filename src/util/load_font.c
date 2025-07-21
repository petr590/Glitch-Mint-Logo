#include "load_font.h"

FT_Library init_freetype_lib_or_exit(void) {
	FT_Library library;

	FT_Error err = FT_Init_FreeType(&library);
	if (err) {
		fprintf(stderr, "Cannot initialize FreeType\n");
		exit(EXIT_FAILURE);
	}

	return library;
}

FT_Face load_freetype_face_or_exit(FT_Library library, const char* path, FT_UInt pixel_height) {
	FT_Face face;

	FT_Error err = FT_New_Face(library, path, 0, &face);
	if (err) {
		fprintf(stderr, "Cannot load font from file '%s'\n", path);
		exit(EXIT_FAILURE);
	}

	FT_Set_Pixel_Sizes(face, 0, pixel_height);
	return face;
}