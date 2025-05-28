add_library(integration-lib
    integration_manager.cpp
)

target_include_directories(integration-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(integration-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 