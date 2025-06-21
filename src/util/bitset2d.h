#ifndef GML_UTIL_BITSET2D_H
#define GML_UTIL_BITSET2D_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct _bitset2d {
	uint32_t width, height;
	uint8_t* data;
} bitset2d;

/**
 * Инициализирует битсет, но не очищает нулями данные.
 * Для очистки используйте функцию bitset2d_clear.
 * Для удаления битсета используйте bitset2d_destroy.
 */
void bitset2d_create(bitset2d* bitset, uint32_t width, uint32_t height);
void bitset2d_destroy(bitset2d* bitset);

void bitset2d_clear(bitset2d* bitset);
int bitset2d_get(const bitset2d* bitset, uint32_t x, uint32_t y);
void bitset2d_set_0(bitset2d* bitset, uint32_t x, uint32_t y);
void bitset2d_set_1(bitset2d* bitset, uint32_t x, uint32_t y);
void bitset2d_set(bitset2d* bitset, uint32_t x, uint32_t y, int value);

#endif