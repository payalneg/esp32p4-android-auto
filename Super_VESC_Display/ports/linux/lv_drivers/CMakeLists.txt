cmake_minimum_required(VERSION 3.12.4)

project(lv_drivers HOMEPAGE_URL https://github.com/lvgl/lv_drivers/)

# Option to build as shared library (as opposed to static), default: OFF
option(BUILD_SHARED_LIBS "Build shared as library (as opposed to static)" OFF)

# Option to define LV_LVGL_H_INCLUDE_SIMPLE, default: ON
option(LV_LVGL_H_INCLUDE_SIMPLE
       "Use #include \"lvgl.h\" instead of #include \"../../lvgl.h\"" ON)

# Option to define LV_CONF_INCLUDE_SIMPLE, default: ON
option(LV_CONF_INCLUDE_SIMPLE
       "Use #include \"lv_conf.h\" instead of #include \"../../lv_conf.h\"" ON)

file(GLOB_RECURSE SOURCES ./*.c)

if (BUILD_SHARED_LIBS)
  add_library(lv_drivers SHARED ${SOURCES})
else()
  add_library(lv_drivers STATIC ${SOURCES})
endif()

add_library(lvgl_drivers ALIAS lv_drivers)
add_library(lvgl::drivers ALIAS lv_drivers)

target_compile_definitions(
  lv_drivers PUBLIC $<$<BOOL:${LV_LVGL_H_INCLUDE_SIMPLE}>:LV_LVGL_H_INCLUDE_SIMPLE>
              $<$<BOOL:${LV_CONF_INCLUDE_SIMPLE}>:LV_CONF_INCLUDE_SIMPLE>
              LV_WAYLAND_TIMER_HANDLER)

target_include_directories(lv_drivers SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(PkgConfig)
pkg_check_modules(PKG_WAYLAND wayland-client wayland-cursor wayland-protocols xkbcommon)
pkg_check_modules(PKG_LIBDRM REQUIRED libdrm)
target_include_directories(lv_drivers PUBLIC ${PKG_LIBDRM_INCLUDE_DIRS})
target_link_libraries(lv_drivers PUBLIC lvgl ${PKG_WAYLAND_LIBRARIES} ${PKG_LIBDRM_LIBRARIES})

if("${LIB_INSTALL_DIR}" STREQUAL "")
  set(LIB_INSTALL_DIR "lib")
endif()

if("${INC_INSTALL_DIR}" STREQUAL "")
  set(INC_INSTALL_DIR "include/lvgl/lv_drivers")
endif()

install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/"
  DESTINATION "${CMAKE_INSTALL_PREFIX}/${INC_INSTALL_DIR}/"
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN ".git*" EXCLUDE
  PATTERN "CMakeFiles" EXCLUDE
  PATTERN "docs" EXCLUDE
  PATTERN "lib" EXCLUDE)

file(GLOB LV_DRIVERS_PUBLIC_HEADERS "${CMAKE_SOURCE_DIR}/lv_drv_conf.h")

set_target_properties(
  lv_drivers
  PROPERTIES OUTPUT_NAME lv_drivers
             ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
             LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
             RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
             PUBLIC_HEADER "${LV_DRIVERS_PUBLIC_HEADERS}")

install(
  TARGETS lv_drivers
  ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
  LIBRARY DESTINATION "${LIB_INSTALL_DIR}"
  RUNTIME DESTINATION "${LIB_INSTALL_DIR}"
  PUBLIC_HEADER DESTINATION "${INC_INSTALL_DIR}")
