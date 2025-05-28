add_library(jit-lib
    jit_compiler.cpp
)

target_include_directories(jit-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(jit-lib
    PUBLIC
    core-lib
    ${Boost_LIBRARIES}
) 