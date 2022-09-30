cmake_minimum_required(VERSION 3.23)

project(
  shlol
  LANGUAGES C
  VERSION 0.1.0
  DESCRIPTION "A simple shell"
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(deps/hedley)

add_subdirectory(deps/str)
add_subdirectory(deps/println)
add_subdirectory(deps/linenoise)
add_subdirectory(deps/buf)

add_executable(shlol src/main.c)
target_compile_features(shlol PRIVATE c_std_17)
target_link_libraries(
  shlol PRIVATE str::str println::println linenoise::linenoise buf::buf
                hedley::hedley
)
