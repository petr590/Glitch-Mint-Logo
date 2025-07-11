#include "render_glyph.h"
#include "../uthash/uthash.h"

typedef struct {
	uint32_t code;
	glyph_t* glyph;
	UT_hash_handle hh;
} glyph_cache_t;

static glyph_cache_t* cache = NULL;

static void add_glyph(uint32_t code, glyph_t* glyth) {
    glyph_cache_t* entry = malloc(sizeof(glyph_cache_t));
    entry->code = code;
    entry->glyph = glyth;

    HASH_ADD_INT(cache, code, entry);
}

static glyph_cache_t* find_glyph(uint32_t code) {
    glyph_cache_t* entry;
    HASH_FIND_INT(cache, &code, entry);
    return entry;
}

const glyph_t* render_glyph(uint32_t code, FT_Face face) {
	glyph_cache_t* entry = find_glyph(code);
	if (entry) return entry->glyph;

	const FT_UInt glyph_index = FT_Get_Char_Index(face, code);
	FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
	FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

	FT_GlyphSlot ft_glyph = face->glyph;

	glyph_t* glyth = malloc(sizeof(glyph_t));
	glyth->width     = ft_glyph->bitmap.width;
	glyth->height    = ft_glyph->bitmap.rows;
	glyth->left      = ft_glyph->bitmap_left;
	glyth->top       = ft_glyph->bitmap_top;
	glyth->advance_x = ft_glyph->advance.x / 64;

	size_t size = glyth->width * glyth->height;
	glyth->buffer = malloc(size);
	memcpy(glyth->buffer, ft_glyph->bitmap.buffer, size);

	add_glyph(code, glyth);
	return glyth;
}