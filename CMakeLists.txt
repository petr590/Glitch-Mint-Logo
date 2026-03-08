cmake_minimum_required(VERSION 3.13)
project(GlitchMintLogo VERSION 1.7.3 LANGUAGES C)

include_directories(/usr/include/drm /usr/include/freetype2 ${CMAKE_SOURCE_DIR}/src)

if(BUILD_TYPE MATCHES "Debug")
	message("Debug build")
	add_compile_options(-g)
else()
	message("Release build")
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
	src/util/system_info.c
)


target_link_options(glitch-mint-logo PRIVATE -rdynamic)
target_link_libraries(glitch-mint-logo m config drm dl pthread png freetype)


# libgml-glitch-logo.so

add_library(
	gml-glitch-logo SHARED
	src/gml-glitch-logo/resources.c
	src/gml-glitch-logo/draw.c
)

target_link_libraries(gml-glitch-logo)


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


# libgml-pixel-logo.so

add_library(
	gml-pixel-logo SHARED
	src/gml-pixel-logo/resources.c
	src/gml-pixel-logo/draw.c
)

target_link_libraries(gml-pixel-logo)


# libgml-yorha.so

add_library(
	gml-yorha SHARED
	src/gml-yorha/resources.c
	src/gml-yorha/services.c
	src/gml-yorha/draw.c
)

# libgml-3d.so

add_library(
	gml-3d-experimental SHARED
	src/gml-3d-experimental/resources.c
	src/gml-3d-experimental/draw.c
)

target_link_libraries(gml-3d-experimental)