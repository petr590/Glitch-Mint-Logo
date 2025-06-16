#include "read_png.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define PNG_HEADER_LEN 8

static void print_error_and_exit(FILE* fp, const char* filename) {
	fclose(fp);
	fprintf(stderr, "Cannot read file '%s' as png\n", filename);
	exit(EXIT_FAILURE);
}

void read_png(const char* filename, png_structp* res_png_ptr, png_infop* res_info_ptr, png_infop* res_end_info) {
	FILE* fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "Cannot open file '%s': %s\n", filename, strerror(errno));
		exit(errno);
	}

	png_byte header[PNG_HEADER_LEN];
	if (fread(header, 1, PNG_HEADER_LEN, fp) < PNG_HEADER_LEN ||
		png_sig_cmp(header, 0, PNG_HEADER_LEN)) {
		print_error_and_exit(fp, filename);
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		print_error_and_exit(fp, filename);
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		print_error_and_exit(fp, filename);
	}

	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		print_error_and_exit(fp, filename);
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		print_error_and_exit(fp, filename);
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, PNG_HEADER_LEN);


	png_set_palette_to_rgb(png_ptr);
	png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xFF, PNG_FILLER_BEFORE);

	png_read_png(png_ptr, info_ptr,
			PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_STRIP_16 |
			PNG_TRANSFORM_GRAY_TO_RGB | PNG_TRANSFORM_BGR,
			NULL);

	fclose(fp);

	*res_png_ptr = png_ptr;
	*res_info_ptr = info_ptr;
	*res_end_info = end_info;
}