/**
 * Заголовок модуля. Содержит объявления функций, которые реализует модуль.
 */
#ifndef GML_WAVE_MODULE_H
#define GML_WAVE_MODULE_H

#include "common.h"

extern float *randbuf1, *randbuf2;

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
 */
void gml_draw(int tick, uint16_t width, uint16_t height, color_t* frame);


/** Освобождает ресурсы модуля перед освобождением ресурсов libdrm */
void gml_cleanup_before_drm(void);

/** Освобождает ресурсы модуля */
void gml_cleanup(void);

#endif