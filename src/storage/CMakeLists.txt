add_library(storage-lib
    storage_manager.cpp
)

target_include_directories(storage-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(storage-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
    ${NUMA_LIBRARIES}
) 