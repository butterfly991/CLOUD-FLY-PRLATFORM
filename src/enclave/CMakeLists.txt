add_library(enclave-lib
    enclave_manager.cpp
)

target_include_directories(enclave-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(enclave-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 