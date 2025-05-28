add_executable(core_tests
    CoreEngineTest.cpp
)

target_link_libraries(core_tests
    PRIVATE
    core-lib
    GTest::GTest
    GTest::Main
)

add_test(NAME core_tests COMMAND core_tests) 