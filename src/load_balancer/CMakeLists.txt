add_library(load-balancer-lib
    load_balancer.cpp
)

target_include_directories(load-balancer-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(load-balancer-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 