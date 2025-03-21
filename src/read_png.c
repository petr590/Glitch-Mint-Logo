#include "read_png.h"
#include <errno.h>

int read_png(const char* filename, png_structp* res_png_ptr, png_infop* res_info_ptr, png_infop* res_end_info) {
	FILE* fp = fopen(filename, "rb");
	if (!fp) return ENOENT;

	char header[8];
	fread(header, 1, 8, fp);

	if (png_sig_cmp(header, 0, 8)) {
		fclose(fp);
		return ERROR_READ_PNG;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		return ERROR_READ_PNG;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fp);
		return ERROR_READ_PNG;
	}

	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return ERROR_READ_PNG;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(fp);
		return ERROR_READ_PNG;
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_BGR, NULL);
	fclose(fp);

	*res_png_ptr = png_ptr;
	*res_info_ptr = info_ptr;
	*res_end_info = end_info;
	return 0;
}