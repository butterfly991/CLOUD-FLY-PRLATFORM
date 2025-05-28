add_library(sync-lib
    sync_manager.cpp
)

target_include_directories(sync-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(sync-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 