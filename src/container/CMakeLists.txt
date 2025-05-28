add_library(container-lib
    container_manager.cpp
)

target_include_directories(container-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(container-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 