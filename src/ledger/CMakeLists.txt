add_library(ledger-lib
    distributed_ledger.cpp
)

target_include_directories(ledger-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(ledger-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
    ${OpenSSL_LIBRARIES}
) 