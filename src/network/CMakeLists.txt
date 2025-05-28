add_library(network-lib
    network_manager.cpp
)

target_include_directories(network-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(network-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${DPDK_LIBRARIES}
) 