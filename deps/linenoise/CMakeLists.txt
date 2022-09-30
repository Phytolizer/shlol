cmake_minimum_required(VERSION 3.23)

project(linenoise LANGUAGES C)

add_library(linenoise repo/linenoise.c repo/stringbuf.c repo/utf8.c)
target_include_directories(linenoise PUBLIC repo)
add_library(linenoise::linenoise ALIAS linenoise)
