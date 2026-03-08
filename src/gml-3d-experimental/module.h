/**
 * Заголовок модуля. Содержит объявления функций, которые реализует модуль.
 */

#include "common.h"
#include "3d_math.h"

#ifndef GML_3D_EXPERIMENTAL_MODULE_H
#define GML_3D_EXPERIMENTAL_MODULE_H


typedef uint16_t index_t;

extern vec3* vertex_buffer;
extern vec4* transformed_vertex_buffer;
extern index_t* index_buffer;

extern size_t vertex_buffer_len;
extern size_t index_buffer_len;

extern float* depth_buffer;

// Следующие функции вызываются в том порядке, в котором объявлены

/** Читает конфиг из файла */
void gml_read_config(config_t*);

/** Загружает и инициализирует ресурсы модуля */
void gml_setup(void);

/**
 * Загружает и инициализирует ресурсы модуля после загрузки libdrm.
 * Используйте эту функцию, если какие-то ресурсы требуют ширину и высоту
 * экрана для инициализации.
 * @param width - ширина экрана.
 * @param height - высота экрана.
 */
void gml_setup_after_drm(uint16_t width, uint16_t height);


/**
 * Рендерит кадр в переданную память. Примечание: не стоит полагаться на то,
 * что осталось в памяти после предыдущего кадра, так как каждый кадр происходит
 * смена буферов. Изначально память не инициализирована.
 * @param tick - номер такта.
 * @param width - ширина фрейма.
 * @param height - высота фрейма.
 * @param frame - массив размером width * height, куда рендерится кадр.
 * @param supposed_time - предположительное время загрузки, рассчитанное из предыдущих загрузок.
 */
void gml_draw(int tick, uint16_t width, uint16_t height, color_t* frame, double supposed_time);


/**
 * Освобождает ресурсы модуля перед освобождением ресурсов libdrm.
 * Используйте эту функцию для освобождения ресурсов, выделенных в gml_setup_after_drm.
 */
void gml_cleanup_before_drm(void);

/**
 * Освобождает ресурсы модуля.
 * Используйте эту функцию для освобождения ресурсов, выделенных в gml_setup.
 */
void gml_cleanup(void);

#endif