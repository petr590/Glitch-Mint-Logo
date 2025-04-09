cmake_minimum_required(VERSION 3.3)
project(GlitchMintLogo VERSION 1.2 LANGUAGES C)

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

add_executable(glitch-mint-logo
		src/main.c
		src/draw.c
		src/drm_fb.c
		src/read_png.c)

target_link_libraries(glitch-mint-logo drm png freetype config)
