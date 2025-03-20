cmake_minimum_required(VERSION 3.3)
project(BootLogoTest VERSION 1.0 LANGUAGES C)

include_directories(/usr/include/drm)

add_executable(boot-logo-test src/main.c)
target_link_libraries(boot-logo-test drm png m)
