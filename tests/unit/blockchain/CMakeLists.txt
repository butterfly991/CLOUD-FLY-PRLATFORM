add_executable(blockchain_tests
    blockchain_test.cpp
)

target_link_libraries(blockchain_tests
    PRIVATE
    ledger-lib
    GTest::GTest
    GTest::Main
)

add_test(NAME blockchain_tests COMMAND blockchain_tests) 