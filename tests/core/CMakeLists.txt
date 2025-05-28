add_executable(core_tests
    core_tests.cpp
    parent_core_tests.cpp
    storage_manager_tests.cpp
    network_manager_tests.cpp
    load_balancer_tests.cpp
    blockchain_tests.cpp
    multi_core_tests.cpp
)

target_include_directories(core_tests
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/core
    ${PROJECT_SOURCE_DIR}/include/core/blockchain
    ${PROJECT_SOURCE_DIR}/include/core/drivers
    ${PROJECT_SOURCE_DIR}/include/core/threading
    ${PROJECT_SOURCE_DIR}/include/core/memory
    ${PROJECT_SOURCE_DIR}/include/core/cache
    ${PROJECT_SOURCE_DIR}/include/core/optimization
    ${PROJECT_SOURCE_DIR}/googletest/googletest/include
)

target_link_libraries(core_tests
    PRIVATE
    core-lib
    gtest
    gtest_main
    pthread
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

add_test(NAME core_tests COMMAND core_tests) 