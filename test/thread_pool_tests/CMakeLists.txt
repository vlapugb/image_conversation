add_executable(thread_pool_tests
    thread_pool_test.c
)

target_link_libraries(thread_pool_tests
    PRIVATE
        convolution_runtime
        PkgConfig::CMOCKA
)

add_test(NAME thread_pool_tests COMMAND thread_pool_tests)
