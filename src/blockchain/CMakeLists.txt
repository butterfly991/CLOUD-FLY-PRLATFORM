add_library(blockchain-lib
    distributed_ledger.cpp
)

target_include_directories(blockchain-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(blockchain-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
) 