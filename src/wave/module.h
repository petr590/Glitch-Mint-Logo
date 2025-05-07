/**
 * Заголовок модуля. Содержит объявления функций, которые реализует модуль.
 */

#include "../common.h"

extern int RANDOM_CONSTANT;

// Следующие функции вызываются в том порядке, в котором объявлены

void glspl_read_config(config_t*);

/** Загружает и инициализирует ресурсы модуля */
void glspl_setup(void);

/** Загружает и инициализирует ресурсы модуля после загрузки libdrm */
void glspl_setup_after_drm(uint32_t width, uint32_t height);


/**
 * Рендерит кадр в переданную память.
 * @param tick - номер такта.
 * @param width - ширина фрейма.
 * @param height - высота фрейма.
 * @param frame - массив размером width * height, куда рендерится кадр.
 */
void glspl_draw(int tick, uint32_t width, uint32_t height, color_t* frame);


/** Освобождает ресурсы модуля перед освобождением ресурсов libdrm */
void glspl_cleanup_before_drm(void);

/** Освобождает ресурсы модуля */
void glspl_cleanup(void);