cmake_minimum_required(VERSION 3.20)

project(ImageConversation LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
include(FetchContent)
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)

option(IMAGE_CONVERSATION_ENABLE_OPENCV
       "Fetch the legacy OpenCV C API target"
       OFF)

file(GLOB_RECURSE FILTER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/filters/*.c"
)

file(GLOB_RECURSE CONVOLUTION_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/convolution/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/parallel_convolution/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/parallel_convolution/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sequentially_convolution/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/image_helpers/*.c"
)

set(BUILD_TESTS OFF CACHE BOOL "")
set(BUILD_PERF_TESTS OFF CACHE BOOL "")
set(BUILD_EXAMPLES OFF CACHE BOOL "")
set(BUILD_DOCS OFF CACHE BOOL "")
set(BUILD_opencv_apps OFF CACHE BOOL "")

set(BUILD_opencv_core ON CACHE BOOL "")
set(BUILD_opencv_imgproc ON CACHE BOOL "")
set(BUILD_opencv_highgui ON CACHE BOOL "")

set(BUILD_opencv_ts OFF CACHE BOOL "")
set(BUILD_opencv_calib3d OFF CACHE BOOL "")
set(BUILD_opencv_contrib OFF CACHE BOOL "")
set(BUILD_opencv_features2d OFF CACHE BOOL "")
set(BUILD_opencv_flann OFF CACHE BOOL "")
set(BUILD_opencv_gpu OFF CACHE BOOL "")
set(BUILD_opencv_legacy OFF CACHE BOOL "")
set(BUILD_opencv_ml OFF CACHE BOOL "")
set(BUILD_opencv_nonfree OFF CACHE BOOL "")
set(BUILD_opencv_objdetect OFF CACHE BOOL "")
set(BUILD_opencv_ocl OFF CACHE BOOL "")
set(BUILD_opencv_photo OFF CACHE BOOL "")
set(BUILD_opencv_stitching OFF CACHE BOOL "")
set(BUILD_opencv_superres OFF CACHE BOOL "")
set(BUILD_opencv_video OFF CACHE BOOL "")
set(BUILD_opencv_videostab OFF CACHE BOOL "")
set(BUILD_opencv_world OFF CACHE BOOL "")

set(WITH_QT OFF CACHE BOOL "")
set(WITH_GTK OFF CACHE BOOL "")
set(WITH_OPENGL OFF CACHE BOOL "")
set(WITH_FFMPEG OFF CACHE BOOL "")
set(WITH_GSTREAMER OFF CACHE BOOL "")
set(WITH_V4L OFF CACHE BOOL "")
set(WITH_1394 OFF CACHE BOOL "")
set(WITH_CUDA OFF CACHE BOOL "")
set(WITH_OPENCL OFF CACHE BOOL "")
set(WITH_TBB OFF CACHE BOOL "")
set(WITH_IPP OFF CACHE BOOL "")
set(WITH_TIFF OFF CACHE BOOL "")
set(WITH_JASPER OFF CACHE BOOL "")
set(WITH_OPENEXR OFF CACHE BOOL "")

FetchContent_Declare(
    opencv
    GIT_REPOSITORY https://github.com/opencv/opencv
    GIT_TAG 2.4.13.6
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(opencv)

add_library(opencv_legacy_c_api INTERFACE)
target_include_directories(opencv_legacy_c_api
    INTERFACE
        ${opencv_SOURCE_DIR}/modules/core/include
        ${opencv_SOURCE_DIR}/modules/imgproc/include
        ${opencv_SOURCE_DIR}/modules/highgui/include
)
target_link_libraries(opencv_legacy_c_api
    INTERFACE
        opencv_core
        opencv_imgproc
        opencv_highgui
)

add_library(filters_core STATIC
    ${FILTER_SOURCES}
)
add_library(filters ALIAS filters_core)

target_include_directories(filters_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(convolution_runtime STATIC
    ${CONVOLUTION_RUNTIME_SOURCES}
)
target_include_directories(convolution_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
)
target_link_libraries(convolution_runtime
    PUBLIC
        filters_core
        opencv_legacy_c_api
        Threads::Threads
        OpenMP::OpenMP_C
)

add_executable(app
    src/app.c
    src/cli_args.c
)
target_link_libraries(app
    PRIVATE
        convolution_runtime
)

if(BUILD_TESTING)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CMOCKA REQUIRED IMPORTED_TARGET cmocka)

    add_subdirectory(test/sequential_tests)
    add_subdirectory(test/thread_pool_tests)
endif()
