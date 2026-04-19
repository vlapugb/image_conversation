cmake_minimum_required(VERSION 3.20)

project(ImageConversation LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

file(GLOB_RECURSE FILTER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/filters/*.c"
)

add_library(filters STATIC ${FILTER_SOURCES})
add_library(filters_core ALIAS filters)

target_include_directories(filters
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(filters
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

if(BUILD_TESTING)
    add_executable(filter_init_builtin_test
        tests/filter_init_builtin_test.c
    )

    target_link_libraries(filter_init_builtin_test
        PRIVATE
            filters
    )

    if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(filter_init_builtin_test
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif()

    add_test(
        NAME filter_init_builtin_test
        COMMAND filter_init_builtin_test
    )
endif()
