#ifndef GML_UTIL_RENDER_GLYPH_H
#define GML_UTIL_RENDER_GLYPH_H

#include <stdint.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define CHAR_START 0x20
#define CHAR_END   0x80
#define CHARS (CHAR_END - CHAR_START) // Диапазон печатаемых символов ASCII

typedef struct {
	uint8_t* buffer;
	uint16_t width, height;
	int32_t left, top, advance_x;
} glyph_t;

/**
 * Рендерит глиф с использованием шрифта, если он ещё не отрендерен.
 * Кэширует все отрендеренные глифы.
 */
const glyph_t* render_glyph(wchar_t code, FT_Face face);

#endif