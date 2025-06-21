#include "render_glyph.h"

const glyph_t* render_glyph(char ch, FT_Face face) {
	static glyph_t glyths[CHARS] = {
		// Первый глиф - пробел
		{
			.buffer = "", // Пустой буфер, но не NULL
			.width = GLYPH_WIDTH,
			.height = 0,
			.left = 0,
			.top = 0,
			.advance_x = GLYPH_WIDTH,
		}
		// Всё остальное заполняется нулями
	};

	glyph_t* res = &glyths[ch - CHAR_START];
	if (res->buffer) {
		return res;
	}

	const FT_UInt glyph_index = FT_Get_Char_Index(face, ch);
	FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
	FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

	FT_GlyphSlot glyph = face->glyph;

	res->width     = glyph->bitmap.width;
	res->height    = glyph->bitmap.rows;
	res->left      = glyph->bitmap_left;
	res->top       = glyph->bitmap_top;
	res->advance_x = glyph->advance.x / 64;

	size_t size = res->width * res->height;
	res->buffer = malloc(size);
	memcpy(res->buffer, glyph->bitmap.buffer, size);

	return res;
}