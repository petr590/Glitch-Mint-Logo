#include "bitset2d.h"
#include <string.h>

static size_t get_size(uint32_t width, uint32_t height) {
	return ((size_t) width * height + 7) / 8;
}

static size_t get_index(const bitset2d* bitset, uint32_t x, uint32_t y) {
	return (size_t) y * bitset->width + x;
}


void bitset2d_create(bitset2d* bitset, uint32_t width, uint32_t height) {
	bitset->width = width;
	bitset->height = height;
	bitset->data = (uint8_t*) malloc(get_size(width, height));
}

void bitset2d_destroy(bitset2d* bitset) {
	if (bitset->data) {
		free(bitset->data);
		bitset->width = 0;
		bitset->height = 0;
		bitset->data = NULL;
	}
}

void bitset2d_clear(bitset2d* bitset) {
	memset(bitset->data, 0, get_size(bitset->width, bitset->height));
}


#ifndef NDEBUG

static void check_pos(const char* func, const bitset2d* bitset, uint32_t x, uint32_t y) {
	if (x >= bitset->width || y >= bitset->height) {
		fprintf(stderr, "%s: position (%d, %d) is out of bound for bitset (width=%d, height=%d)\n", func, x, y, bitset->width, bitset->height);
		exit(EXIT_FAILURE);
	}
}

#define CHECK_POS(func, bitset, x, y) check_pos(func, bitset, x, y)

#else
#define CHECK_POS(func, bitset, x, y)
#endif


int bitset2d_get(const bitset2d* bitset, uint32_t x, uint32_t y) {
	CHECK_POS("bitset2d_get", bitset, x, y);
	size_t index = get_index(bitset, x, y);
	return (bitset->data[index >> 3] >> (index & 0x7)) & 0x1;
}

static void bitset2d_set_0_raw(bitset2d* bitset, uint32_t x, uint32_t y) {
	size_t index = get_index(bitset, x, y);
	bitset->data[index >> 3] &= ~(1 << (index & 0x7));
}

static void bitset2d_set_1_raw(bitset2d* bitset, uint32_t x, uint32_t y) {
	size_t index = get_index(bitset, x, y);
	bitset->data[index >> 3] |= (1 << (index & 0x7));
}

void bitset2d_set_0(bitset2d* bitset, uint32_t x, uint32_t y) {
	CHECK_POS("bitset2d_set_0", bitset, x, y);
	bitset2d_set_0_raw(bitset, x, y);
}

void bitset2d_set_1(bitset2d* bitset, uint32_t x, uint32_t y) {
	CHECK_POS("bitset2d_set_1", bitset, x, y);
	bitset2d_set_1_raw(bitset, x, y);
}

void bitset2d_set(bitset2d* bitset, uint32_t x, uint32_t y, int value) {
	CHECK_POS("bitset2d_set", bitset, x, y);
	(value ? bitset2d_set_1_raw(bitset, x, y) : bitset2d_set_0_raw(bitset, x, y));
}