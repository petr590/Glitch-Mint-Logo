cmake_minimum_required(VERSION 3.3)
project(BootLogoTest VERSION 1.1 LANGUAGES C)

include_directories(/usr/include/drm)

add_executable(boot-logo-test
		src/main.c
		src/draw.c
		src/drm_fb.c
		src/read_png.c)

target_link_libraries(boot-logo-test drm png)
