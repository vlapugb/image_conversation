add_executable(sequential_tests
    sequential_tests.c
)

target_link_libraries(sequential_tests
    PRIVATE
        convolution_runtime
        PkgConfig::CMOCKA
)

add_test(NAME sequential_tests COMMAND sequential_tests)

add_executable(sequential_opencv_reference_tests
    sequential_opencv_reference_tests.c
)

target_link_libraries(sequential_opencv_reference_tests
    PRIVATE
        convolution_runtime
        PkgConfig::CMOCKA
)

add_test(NAME sequential_opencv_reference_tests COMMAND sequential_opencv_reference_tests)
