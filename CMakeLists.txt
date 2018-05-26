cmake_minimum_required(VERSION 3.7.2)
set(CMAKE_CXX_STANDARD 17)

project(jarl)

include_directories(bindings)
file(GLOB SOURCES "src/*.cpp")

add_compile_options(-fno-exceptions)

# add_definitions(-DPRINT_LEXEMES)
# add_definitions(-DPRINT_AST)
# add_definitions(-DNO_GENERATE)
# add_definitions(-DPRINT_CODE)
# add_definitions(-DNO_EXECUTE)
# add_definitions(-DPRINT_STACK)
# add_definitions(-DPRINT_OP)
# add_definitions(-DPRINT_ERROR_JUMPS)

add_library(jarl STATIC ${SOURCES})
