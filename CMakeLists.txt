cmake_minimum_required(VERSION 3.15)

project(ArgMap VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# Vendored FLTK (and its bundled zlib/libpng/libjpeg) is the default so a full
# build works without installed development packages. This is what the Windows
# scripts use. The plain Makefile can still use a system FLTK via fltk-config.
# -----------------------------------------------------------------------------
set(OPTION_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared FLTK libraries" FORCE)
set(FLTK_BUILD_TEST OFF CACHE BOOL "Build FLTK test programs" FORCE)
set(FLTK_BUILD_EXAMPLES OFF CACHE BOOL "Build FLTK examples" FORCE)
set(OPTION_USE_SYSTEM_ZLIB OFF CACHE BOOL "Use bundled zlib" FORCE)
set(OPTION_USE_SYSTEM_LIBPNG OFF CACHE BOOL "Use bundled libpng" FORCE)
set(OPTION_USE_SYSTEM_LIBJPEG OFF CACHE BOOL "Use bundled libjpeg" FORCE)

add_subdirectory(third_party/fltk-1.3.11 EXCLUDE_FROM_ALL)

# FLTK builds its bundled zlib with fltk_z_-prefixed symbols; its zlib.h defines
# the matching macros. Use that header so our direct zlib calls bind to fltk_z.
set(ARGMAP_ZLIB_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fltk-1.3.11/zlib)

set(UI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gui)
set(UI_FL ${UI_SOURCE_DIR}/MapWindow.fl)
set(UI_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(UI_HEADER ${UI_GEN_DIR}/MapWindow.H)
set(UI_CPP ${UI_GEN_DIR}/MapWindow.cpp)

add_custom_command(
  OUTPUT ${UI_HEADER} ${UI_CPP}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${UI_GEN_DIR}
  COMMAND ${CMAKE_COMMAND} -E chdir ${UI_GEN_DIR}
          $<TARGET_FILE:fluid> -c -o MapWindow.cpp -h MapWindow.H ${UI_FL}
  DEPENDS ${UI_FL} fluid
  COMMENT "Generating FLUID UI sources from gui/MapWindow.fl"
  VERBATIM
)

add_custom_target(argmap_ui DEPENDS ${UI_HEADER} ${UI_CPP})

set(ARGMAP_SOURCES
  main.cpp
  domain/Types.cpp
  domain/MapService.cpp
  data/Json.cpp
  data/JsonMapRepository.cpp
  data/PngExporter.cpp
  app/AppController.cpp
  gui/AppWindow.cpp
  gui/ArgumentCanvas.cpp
  gui/Dialogs.cpp
  gui/MapImageRenderer.cpp
  gui/MapWindow.cpp
  gui/Renderer.cpp
  gui/WindowView.cpp
  ${UI_CPP}
)

add_executable(argmap ${ARGMAP_SOURCES})
add_dependencies(argmap argmap_ui)

target_include_directories(argmap BEFORE PRIVATE
  ${ARGMAP_ZLIB_INCLUDE}
)
target_include_directories(argmap PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${UI_GEN_DIR}
)

# fltk_z is linked explicitly because PngExporter calls zlib directly.
target_link_libraries(argmap PRIVATE fltk fltk_png fltk_jpeg fltk_z)

enable_testing()

add_executable(domain_tests
  tests/domain_tests.cpp
  domain/Types.cpp
  domain/MapService.cpp
  data/Json.cpp
  data/JsonMapRepository.cpp
  data/PngExporter.cpp
)

target_include_directories(domain_tests BEFORE PRIVATE ${ARGMAP_ZLIB_INCLUDE})
target_include_directories(domain_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(domain_tests PRIVATE fltk_z)

add_test(NAME domain_tests COMMAND domain_tests)
