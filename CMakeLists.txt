cmake_minimum_required(VERSION 3.7.2)
set(CMAKE_CXX_STANDARD 17)

project(jarl)

include_directories("src/bindings")
file(GLOB jarl_sources "src/jarl/*.cpp")
file(GLOB bin_sources "src/bin/*.cpp")

add_compile_options(-fno-exceptions)

add_library(jarl STATIC ${jarl_sources})
add_executable(jarlbin ${bin_sources})
target_link_libraries(jarlbin jarl)
