cmake_minimum_required(VERSION 3.3)
project(GlitchMintLogo VERSION 1.4.1 LANGUAGES C)

include_directories(/usr/include/drm /usr/include/freetype2)

if(BUILD_TYPE MATCHES "Debug")
	add_compile_options(-g)
else()
	add_compile_options(-O3)
endif()

add_compile_options(
	-pedantic
	-Wall
	-Wshadow
	-Wpointer-arith
	-Wcast-qual
	-Wstrict-prototypes
	-Wmissing-prototypes
)

# glitch-mint-logo

add_executable(
	glitch-mint-logo
	src/common.c
	src/main.c
	src/drm_fb.c
)

target_link_libraries(glitch-mint-logo drm config dl)


# libgml-logo.so

add_library(
	gml-logo SHARED
	src/common.c
	src/mint-logo/resources.c
	src/mint-logo/draw.c
	src/util/read_png.c
	src/util/render_glyth.c
)

target_link_libraries(gml-logo png freetype)


# libgml-matrix.so

add_library(
	gml-matrix SHARED
	src/common.c
	src/matrix/resources.c
	src/matrix/draw.c
	src/util/render_glyth.c
)

target_link_libraries(gml-matrix png freetype)


# libgml-wave.so

add_library(
	gml-wave SHARED
	src/common.c
	src/wave/resources.c
	src/wave/draw.c
)

target_link_libraries(gml-wave m)
