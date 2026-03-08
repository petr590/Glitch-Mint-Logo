#!/bin/bash
macros=($(gcc -E -dM - < /dev/null | sed -E 's/^#define (\w+).*/-D\1/g'))

cppcheck\
	--enable=all --disable=style,unusedFunction\
	--std=c11 --quiet -j 2\
	"${macros[@]}"\
	-I/usr/include/\
	-I/usr/include/linux/\
	-I/usr/include/x86_64-linux-gnu/\
	-I/usr/lib/gcc/x86_64-linux-gnu/13/include/\
	-I/usr/include/drm\
	-I/usr/include/freetype2\
	-Isrc/\
	src/*.c src/*/*.c
