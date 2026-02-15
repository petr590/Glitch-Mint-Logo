#ifndef GML_UTIL_UTIL_H
#define GML_UTIL_UTIL_H

#include "common.h"

static inline uint16_t u16min(uint16_t num1, uint16_t num2) {
	return num1 < num2 ? num1 : num2;
}

static inline int32_t i32min(int32_t num1, int32_t num2) {
	return num1 < num2 ? num1 : num2;
}

static inline int32_t i32max(int32_t num1, int32_t num2) {
	return num1 > num2 ? num1 : num2;
}

static inline uint32_t u32min(uint32_t num1, uint32_t num2) {
	return num1 < num2 ? num1 : num2;
}

static inline double fclamp(double val, double min, double max) {
	return  val < min ? min :
			val > max ? max :
			val;
}

/** Накладывает второй цвет на первый, учитывая его прозрачность. */
static inline color_t mix(color_t rgb, color_t argb) {
	uint8_t alpha = (uint8_t)(argb >> 24);
	uint8_t r = (uint8_t)(((uint8_t)(rgb >> 16) * (0xFF - alpha) + (uint8_t)(argb >> 16) * alpha) / 0xFF);
	uint8_t g = (uint8_t)(((uint8_t)(rgb >>  8) * (0xFF - alpha) + (uint8_t)(argb >>  8) * alpha) / 0xFF);
	uint8_t b = (uint8_t)(((uint8_t)(rgb      ) * (0xFF - alpha) + (uint8_t)(argb      ) * alpha) / 0xFF);
	return (color_t)(r << 16 | g << 8 | b);
}

/** Целочисленное деление с округлением вверх */
static inline uint32_t u32_div_ceil(uint32_t a, uint32_t b) {
	return (a + b - 1) / b;
}

/** Целочисленное деление с округлением вверх */
static inline int32_t i32_div_ceil(int32_t a, int32_t b) {
	return (a + b - 1) / b;
}

#endif