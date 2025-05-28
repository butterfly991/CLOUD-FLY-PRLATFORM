add_library(accelerators-lib
    accelerator_manager.cpp
)

target_include_directories(accelerators-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(accelerators-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 