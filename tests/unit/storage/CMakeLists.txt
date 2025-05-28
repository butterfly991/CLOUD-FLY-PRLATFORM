add_executable(storage_tests
    storage_test.cpp
)

target_link_libraries(storage_tests
    PRIVATE
    storage-lib
    GTest::GTest
    GTest::Main
)

add_test(NAME storage_tests COMMAND storage_tests) 