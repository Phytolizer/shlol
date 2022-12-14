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
add_subdirectory(deps/sum)

add_executable(
  shlol src/main.c src/lexer.c src/parser.c src/ast.c src/executor.c
)
target_compile_features(shlol PRIVATE c_std_17)
target_link_libraries(
  shlol PRIVATE str::str println::println linenoise::linenoise buf::buf
                hedley::hedley sum::sum
)

if(SHLOL_SANITIZE)
  target_compile_options(
    shlol PRIVATE -fsanitize=address -fno-omit-frame-pointer
  )
  target_link_options(shlol PRIVATE -fsanitize=address)
endif()
