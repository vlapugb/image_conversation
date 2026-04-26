add_executable(sequential_tests
    sequential_tests.c
)

target_link_libraries(sequential_tests
    PRIVATE
        convolution_runtime
)

add_test(NAME sequential_tests COMMAND sequential_tests)
