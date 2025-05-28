add_library(cache-lib
    cache_manager.cpp
)

target_include_directories(cache-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(cache-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 