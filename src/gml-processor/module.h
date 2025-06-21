/**
 * Заголовок модуля. Содержит объявления функций, которые реализует модуль.
 */
#ifndef GML_WAVE_PROCESSOR_H
#define GML_WAVE_PROCESSOR_H

#include "../common.h"
#include "../util/bitset2d.h"

#define PIXEL_SIZE 8

extern bitset2d v_bg_buffer; // Буфер вертикальных линий,   размер = width * height
extern bitset2d h_bg_buffer; // Буфер горизонтальных линий, размер = width * height
extern bitset2d p_bg_buffer; // Буфер точек,                размер = (width + 1) * (height + 1)

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

#endif