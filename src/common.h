/**
 * Данные, общие для всех модулей и основного кода.
 * 
 * Чтобы создать модуль, нужно создать динамическую библиотеку,
 * которая реализует следующие функции:
 * 
 * void gml_setup(void);
 * void gml_setup_after_drm(uint32_t width, uint32_t height);
 * void gml_draw(int tick, uint32_t width, uint32_t height, color_t* frame);
 * void gml_cleanup_before_drm(void);
 * void gml_cleanup(void);
 * 
 * В качестве шаблона модуля используйте файл template/module.h
 */

#include <stdint.h>
#include <libconfig.h>

#define CONFIG_FILE "/etc/glitch-mint-logo/config"

typedef uint32_t color_t;

/**
 * Частота обновления экрана, то есть частота вызова функции gml_draw.
 * Можно изменить в функциях gml_setup и gml_setup_after_drm.
 * По умолчанию равна частоте монитора.
 */
extern double fps;

/**
 * Читает строку конфига по ключу key и запиывает её в str.
 * Если такой строки нет, выводит сообщение об ошибке и выходит.
 * Строка копируется с помощью strdup для избегания ошибок.
 */
const char* read_config_str(config_t* cfgp, const char* key);