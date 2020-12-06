cmake_minimum_required(VERSION 3.17)
project(simplefs C)

set(CMAKE_C_STANDARD 99)

add_executable(simplefs main.c simplefs.h simplefs.c)