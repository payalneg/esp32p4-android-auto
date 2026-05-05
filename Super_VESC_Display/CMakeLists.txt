# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.12.4)

project(gui_guider)
add_compile_options(-O2)

FILE(GLOB_RECURSE SOURCES ./custom/*.c ./generated/*.c ports/linux/mouse_cursor_icon.c ports/linux/main.c ports/linux/nxp_gg_utils.c)

find_package(PkgConfig)
pkg_check_modules(PKG_WAYLAND wayland-client wayland-cursor wayland-protocols xkbcommon)
pkg_check_modules(PKG_LIBDRM REQUIRED libdrm)

add_executable (gui_guider ${SOURCES})
target_link_libraries (gui_guider PUBLIC lvgl lv_drivers ${PKG_WAYLAND_LIBRARIES} ${PKG_LIBDRM_LIBRARIES})
target_include_directories(gui_guider PRIVATE generated custom generated/guider_customer_fonts generated/guider_fonts generated/images)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/gui_guider DESTINATION bin)

if(EXISTS ${CMAKE_SOURCE_DIR}/lvgl AND EXISTS ${CMAKE_SOURCE_DIR}/ports/linux/lv_drivers)
add_subdirectory(lvgl)
target_include_directories(lvgl PRIVATE ports/linux)
add_subdirectory(ports/linux/lv_drivers ${CMAKE_CURRENT_BINARY_DIR}/lv_drivers)
target_include_directories(gui_guider PRIVATE lvgl/src lvgl/src/font ports/linux/lv_drivers)
endif()

