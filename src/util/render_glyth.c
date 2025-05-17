#include "render_glyth.h"

const glyth_t* render_glyth(char ch, FT_Face face) {
	static glyth_t glyths[CHARS] = {
		// Первый глиф - пробел
		{
			.width = GLYTH_WIDTH,
			.height = 0,
			.buffer = "", // Пустой буфер, но не NULL
		}
		// Всё остальное заполняется нулями
	};

	glyth_t* glyth = &glyths[ch - CHAR_START];
	if (glyth->buffer) {
		return glyth;
	}

	const FT_UInt glyph_index = FT_Get_Char_Index(face, ch);
	FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
	FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

	FT_Bitmap* bitmap = &face->glyph->bitmap;

	glyth->width = bitmap->width;
	glyth->height = bitmap->rows;
	size_t size = glyth->width * glyth->height;

	glyth->buffer = malloc(size);
	memcpy(glyth->buffer, bitmap->buffer, size);

	return glyth;
}