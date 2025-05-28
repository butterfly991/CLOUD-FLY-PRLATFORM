add_library(diagnostics-lib
    diagnostics_manager.cpp
)

target_include_directories(diagnostics-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(diagnostics-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 