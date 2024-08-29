cmake_minimum_required(VERSION 3.29)
project(tga C)

set(CMAKE_C_STANDARD 11)

add_library(tga SHARED tga.c)
