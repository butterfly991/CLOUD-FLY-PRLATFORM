add_library(monitoring-lib
    monitoring_system.cpp
)

target_include_directories(monitoring-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(monitoring-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 