/**
 * Заголовок модуля. Содержит объявления функций, которые реализует модуль.
 */
#ifndef GML_MATRIX_MODULE_H
#define GML_MATRIX_MODULE_H

#include "../common.h"
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define GLYPH_WIDTH 12
#define GLYPH_HEIGHT 20
#define TEXT_PADDING 0
#define CHAR_WIDTH (GLYPH_WIDTH + TEXT_PADDING)
#define CHAR_HEIGHT (GLYPH_HEIGHT + TEXT_PADDING)

extern FT_Face face;
extern int text_w, text_h;
extern char* text_buffer; // Буфер размером text_w * text_h

// Следующие функции вызываются в том порядке, в котором объявлены

void gml_read_config(config_t*);

/** Загружает и инициализирует ресурсы модуля */
void gml_setup(void);

/** Загружает и инициализирует ресурсы модуля после загрузки libdrm */
void gml_setup_after_drm(uint32_t width, uint32_t height);


/**
 * Рендерит кадр в переданную память.
 * @param tick - номер такта.
 * @param width - ширина фрейма.
 * @param height - высота фрейма.
 * @param frame - массив размером width * height, куда рендерится кадр.
 */
void gml_draw(int tick, uint32_t width, uint32_t height, color_t* frame);


/** Освобождает ресурсы модуля перед освобождением ресурсов libdrm */
void gml_cleanup_before_drm(void);

/** Освобождает ресурсы модуля */
void gml_cleanup(void);

#endif