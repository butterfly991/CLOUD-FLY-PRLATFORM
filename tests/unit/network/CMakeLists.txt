add_executable(network_tests
    network_test.cpp
)

target_link_libraries(network_tests
    PRIVATE
    network-lib
    GTest::GTest
    GTest::Main
)

add_test(NAME network_tests COMMAND network_tests) 