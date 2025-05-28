add_library(core-lib
    ParentCore.cpp
    StorageManager.cpp
    NetworkManager.cpp
    LoadBalancer.cpp
    MultiCoreEngine.cpp
    core_optimizer.cpp
    CoreEngine.cpp
    engine.cpp
    blockchain/MultiCoreBlockchain.cpp
)

target_include_directories(core-lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/core
    ${PROJECT_SOURCE_DIR}/include/core/blockchain
    ${PROJECT_SOURCE_DIR}/include/core/drivers
    ${PROJECT_SOURCE_DIR}/include/core/threading
    ${PROJECT_SOURCE_DIR}/include/core/memory
    ${PROJECT_SOURCE_DIR}/include/core/cache
    ${PROJECT_SOURCE_DIR}/include/core/optimization
)

target_link_libraries(core-lib
    PUBLIC
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    Threads::Threads
    crypto
    ssl
    pthread
    rt
    dl
)

# Add compiler flags
target_compile_options(core-lib
    PRIVATE
    -Wall
    -Wextra
    -O3
    -march=native
    -mtune=native
    -fPIC
)

# Enable SIMD optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(core-lib
        PRIVATE
        -mavx2
        -mfma
        -msse4.2
    )
endif() 