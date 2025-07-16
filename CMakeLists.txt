cmake_minimum_required(VERSION 3.3)
project(GlitchMintLogo VERSION 1.7.0 LANGUAGES C)

include_directories(/usr/include/drm /usr/include/freetype2)

if(BUILD_TYPE MATCHES "Debug")
	add_compile_options(-g)
else()
	add_compile_options(-DNDEBUG -march=native -mfpmath=sse)
endif()

add_compile_options(
	-O3
	-pedantic
	-Wall
	-Wshadow
	-Wpointer-arith
	-Wcast-qual
	-Wstrict-prototypes
	-Wmissing-prototypes
	-Werror=implicit-function-declaration
)

# glitch-mint-logo

add_executable(
	glitch-mint-logo
	src/main.c
	src/drm.c
	src/args.c
	src/util.c
	src/common.c
	src/modules.c
	src/read_config.c
	src/signal_handlers.c
)

target_link_libraries(glitch-mint-logo m config drm dl)


# libgml-logo.so

add_library(
	gml-mint-logo SHARED
	src/common.c
	src/gml-mint-logo/resources.c
	src/gml-mint-logo/draw.c
	src/util/read_png.c
	src/util/render_glyph.c
)

target_link_libraries(gml-mint-logo png freetype)


# libgml-matrix.so

add_library(
	gml-matrix SHARED
	src/common.c
	src/gml-matrix/resources.c
	src/gml-matrix/draw.c
	src/util/render_glyph.c
)

target_link_libraries(gml-matrix png freetype)


# libgml-wave.so

add_library(
	gml-wave SHARED
	src/common.c
	src/gml-wave/resources.c
	src/gml-wave/draw.c
)

target_link_libraries(gml-wave)


# libgml-processor.so

add_library(
	gml-processor SHARED
	src/common.c
	src/gml-processor/resources.c
	src/gml-processor/draw.c
	src/util/read_png.c
	src/util/bitset2d.c
)

target_link_libraries(gml-processor png)


# libgml-pixels.so

add_library(
	gml-pixels SHARED
	src/common.c
	src/gml-pixels/resources.c
	src/gml-pixels/draw.c
	src/util/read_png.c
	src/util/bitset2d.c
)

target_link_libraries(gml-pixels png)


# libgml-yorha.so

add_library(
	gml-yorha SHARED
	src/common.c
	src/gml-yorha/resources.c
	src/gml-yorha/services.c
	src/gml-yorha/draw.c
	src/util/bitset2d.c
	src/util/read_png.c
	src/util/render_glyph.c
)

target_link_libraries(gml-yorha png freetype)
