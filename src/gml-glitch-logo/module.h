/**
 * Заголовок модуля. Содержит объявления функций, которые реализует модуль.
 */
 
#ifndef GML_GLITCH_LOGO_MODULE_H
#define GML_GLITCH_LOGO_MODULE_H

#include "common.h"
#include <libpng/png.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define FPS 8
#define GLYPH_HEIGHT 20

extern color_t text_color;
extern int RANDOM_CONSTANT;
extern const char* system_name;
extern png_structp png_ptr;
extern png_infop info_ptr, end_info;
extern FT_Face face;
extern color_t* bg_buffer; // Буфер размером height

// Следующие функции вызываются в том порядке, в котором объявлены

void gml_read_config(config_t*);

/** Загружает и инициализирует ресурсы модуля */
void gml_setup(void);

/** Загружает и инициализирует ресурсы модуля после загрузки libdrm */
void gml_setup_after_drm(uint16_t width, uint16_t height);


/**
 * Рендерит кадр в переданную память.
 * @param tick - номер такта.
 * @param width - ширина фрейма.
 * @param height - высота фрейма.
 * @param frame - массив размером width * height, куда рендерится кадр.
 * @param supposed_time - предположительное время загрузки, рассчитанное из предыдущих загрузок.
 */
void gml_draw(int tick, uint16_t width, uint16_t height, color_t* frame, double supposed_time);


/** Освобождает ресурсы модуля перед освобождением ресурсов libdrm */
void gml_cleanup_before_drm(void);

/** Освобождает ресурсы модуля */
void gml_cleanup(void);

#endif