#ifndef GML_UTIL_RENDER_GLYTH_H
#define GML_UTIL_RENDER_GLYTH_H

#include <stdint.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define GLYTH_WIDTH 12
#define GLYTH_HEIGHT 20

#define CHAR_START 0x20
#define CHAR_END   0x80
#define CHARS (CHAR_END - CHAR_START) // Диапазон печатаемых символов ASCII

typedef struct {
	uint32_t width, height;
	uint8_t* buffer;
} glyth_t;

/**
 * Рендерит глиф с использованием шрифта, если он ещё не отрендерен.
 * Кэширует все отрендеренные глифы.
 */
const glyth_t* render_glyth(char ch, FT_Face face);

#endif