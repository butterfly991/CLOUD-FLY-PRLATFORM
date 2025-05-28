add_library(tracing-lib
    tracing_manager.cpp
)

target_include_directories(tracing-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(tracing-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 