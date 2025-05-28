add_library(packet-processor-lib
    packet_processor.cpp
)

target_include_directories(packet-processor-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(packet-processor-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
    ${DPDK_LIBRARIES}
) 