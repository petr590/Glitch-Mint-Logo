#include <stdint.h>
#include <libpng/png.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#define FPS 10
#define GLYTH_HEIGHT 20
#define SPACE_WIDTH 12

typedef uint32_t color_t;
extern int RANDOM_CONSTANT;

/**
 * Рендерит кадр в переданную память.
 * @param tick - номер такта.
 * @param width - ширина фрейма.
 * @param height - высота фрейма.
 * @param frame - массив размером width * height, куда рендерится кадр.
 * @param bg_buffer - массив длиной height, используется при рендере. Может быть не инициализирован.
 * @param png_ptr - указатель на изображение png.
 * @param info_ptr - указатель на информацию об изображении png.
 * @param name - строка с названием системы.
 * @param face - шрифт для рендера названия. Может быть NULL.
 */
void draw(int tick, uint32_t width, uint32_t height, color_t* frame, color_t* bg_buffer,
		const png_struct* png_ptr, const png_info* info_ptr,
		const char* name, FT_Face face);