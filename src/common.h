/**
 * Данные, общие для всех модулей и основного кода
 */

#include <stdint.h>
#include <libconfig.h>

#define FPS 10
#define CONFIG_FILE "/etc/glitch-mint-logo/config"

typedef uint32_t color_t;

/**
 * Читает строку конфига по ключу key и запиывает её в str.
 * Если такой строки нет, выводит сообщение об ошибке и выходит.
 * Строка копируется с помощью strdup для избегания ошибок.
 */
const char* read_config_str(config_t* cfgp, const char* key);