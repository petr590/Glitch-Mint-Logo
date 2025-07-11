/**
 * Заголовок модуля. Содержит объявления функций, которые реализует модуль.
 */
#ifndef GML_YORHA_MODULE_H
#define GML_YORHA_MODULE_H

#include "../common.h"
#include "../util/bitset2d.h"
#include <libpng/png.h>
#include <systemd/sd-bus.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define CELL_SIZE     8
#define GLYPH_HEIGHT  25
#define STRING_HEIGHT 50

#define MAX_RUNNING_STRINGS 10

extern png_structp png_ptr;
extern png_infop info_ptr, end_info;
extern FT_Face face;

extern bitset2d v_bg_buffer; // Буфер вертикальных линий,   размер = width * height
extern bitset2d h_bg_buffer; // Буфер горизонтальных линий, размер = width * height
extern bitset2d p_bg_buffer; // Буфер точек,                размер = width * height


typedef struct running_str {
    const wchar_t* str;
    int printed;
} running_str_t;

extern running_str_t running_strings[MAX_RUNNING_STRINGS];
extern size_t running_strings_len;

// Следующие функции вызываются в том порядке, в котором объявлены

void gml_read_config(config_t*);

/** Загружает и инициализирует ресурсы модуля */
void gml_setup(void);

/**
 * Загружает и инициализирует ресурсы модуля после загрузки libdrm.
 * @param width - ширина экрана.
 * @param height - высота экрана.
 */
void gml_setup_after_drm(uint32_t width, uint32_t height);


/**
 * Рендерит кадр в переданную память.
 * @param tick - номер такта.
 * @param width - ширина фрейма.
 * @param height - высота фрейма.
 * @param frame - массив размером width * height, куда рендерится кадр.
 */
void gml_draw(int tick, uint32_t width, uint32_t height, color_t* frame);


/** Освобождает ресурсы модуля перед освобождением ресурсов libdrm. */
void gml_cleanup_before_drm(void);

/** Освобождает ресурсы модуля. */
void gml_cleanup(void);


void init_socket(void);
void cleanup_socket(void);

void read_from_socket(void);

#endif