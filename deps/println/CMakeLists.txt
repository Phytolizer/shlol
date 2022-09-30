cmake_minimum_required(VERSION 3.23)

project(
  println
  LANGUAGES C
  DESCRIPTION "Minimal printfln() functions for C"
)

add_library(println src/println.c)
target_link_libraries(println PUBLIC hedley::hedley)
target_include_directories(println PUBLIC include)
add_library(println::println ALIAS println)
