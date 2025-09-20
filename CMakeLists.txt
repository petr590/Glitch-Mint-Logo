cmake_minimum_required(VERSION 3.13)
project(GlitchMintLogo VERSION 1.7.3 LANGUAGES C)

include_directories(/usr/include/drm /usr/include/freetype2 ${CMAKE_SOURCE_DIR}/src)

if(BUILD_TYPE MATCHES "Debug")
	add_compile_options(-g)
else()
	add_compile_options(-DNDEBUG -march=native -mfpmath=sse -flto)
endif()

add_compile_options(
	-O3
	-Wall
	-Wextra
	-Wpedantic

	-Wabsolute-value
	-Wcast-align
	-Wcast-qual
	-Wduplicated-branches
	-Wduplicated-cond
	-Wlogical-op
	-Wredundant-decls
	
	-Wno-conversion
	-Wno-sign-conversion
	-Wno-sign-compare

	-Werror=implicit-function-declaration
	-Werror=return-type
)

# glitch-mint-logo

add_executable(
	glitch-mint-logo

	src/main.c
	src/start.c
	src/drm.c
	src/args.c
	src/util.c
	src/common.c
	src/modules.c
	src/read_config.c
	src/signal_handlers.c

	src/util/bitset2d.c
	src/util/read_png.c
	src/util/load_font.c
	src/util/render_glyph.c
)


target_link_options(glitch-mint-logo PRIVATE -rdynamic)
target_link_libraries(glitch-mint-logo m config drm dl pthread png freetype)


# libgml-logo.so

add_library(
	gml-mint-logo SHARED
	src/gml-mint-logo/resources.c
	src/gml-mint-logo/draw.c
)

target_link_libraries(gml-mint-logo)


# libgml-matrix.so

add_library(
	gml-matrix SHARED
	src/gml-matrix/resources.c
	src/gml-matrix/draw.c
)

target_link_libraries(gml-matrix)


# libgml-wave.so

add_library(
	gml-wave SHARED
	src/gml-wave/resources.c
	src/gml-wave/draw.c
)

target_link_libraries(gml-wave)


# libgml-processor.so

add_library(
	gml-processor SHARED
	src/gml-processor/resources.c
	src/gml-processor/draw.c
)

target_link_libraries(gml-processor)


# libgml-pixels.so

add_library(
	gml-pixels SHARED
	src/gml-pixels/resources.c
	src/gml-pixels/draw.c
)

target_link_libraries(gml-pixels)


# libgml-yorha.so

add_library(
	gml-yorha SHARED
	src/gml-yorha/resources.c
	src/gml-yorha/services.c
	src/gml-yorha/draw.c
)

target_link_libraries(gml-yorha)