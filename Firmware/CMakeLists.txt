cmake_minimum_required(VERSION 3.13)

# Stuff needed to make RP2040 SDK work
include(pico_sdk_import.cmake)

# Project name
project(matrix C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# More stuff to make RP2040 SDK work
pico_sdk_init()

# Tell Cmake to process other libarary 'CMakeList.txt' files
add_subdirectory(lib)
add_subdirectory(src)

