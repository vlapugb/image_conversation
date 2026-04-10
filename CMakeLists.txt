cmake_minimum_required(VERSION 3.20)

project(ImageConversation LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE FILTER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/filters/*.c"
)

add_library(filters
    ${FILTER_SOURCES}
)


target_include_directories(filters
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_compile_options(filters
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)