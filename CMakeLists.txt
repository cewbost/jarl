cmake_minimum_required(VERSION 3.7.2)
set(CMAKE_CXX_STANDARD 17)

project(jarl)

include_directories(bindings)
file(GLOB SOURCES "src/*.cpp")

add_compile_options(-fno-exceptions)

add_library(jarl STATIC ${SOURCES})
