cmake_minimum_required(VERSION 3.15)
project(CloudService VERSION 1.0.0 LANGUAGES CXX)

# Установка стандарта C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Опции сборки
option(BUILD_TESTS "Build test suite" ON)
option(USE_GPU "Enable GPU acceleration" ON)
option(USE_FPGA "Enable FPGA acceleration" ON)
option(USE_DPDK "Enable DPDK networking" ON)
option(USE_NUMA "Enable NUMA support" ON)

# Поиск необходимых пакетов
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(CUDA QUIET)
find_package(DPDK QUIET)
find_package(NUMA QUIET)

# Добавление поддиректорий
add_subdirectory(src)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Установка путей включения
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${CUDA_INCLUDE_DIRS}
    ${DPDK_INCLUDE_DIRS}
)

# Линковка библиотек
link_libraries(
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    Threads::Threads
    ${CUDA_LIBRARIES}
    ${DPDK_LIBRARIES}
    ${NUMA_LIBRARIES}
)

# Установка флагов компиляции
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(USE_NUMA)
        add_compile_definitions(USE_NUMA)
    endif()
    if(USE_GPU)
        add_compile_definitions(USE_GPU)
    endif()
    if(USE_FPGA)
        add_compile_definitions(USE_FPGA)
    endif()
    if(USE_DPDK)
        add_compile_definitions(USE_DPDK)
    endif()
endif()

# Установка путей установки
set(CMAKE_INSTALL_PREFIX /usr/local)
set(CMAKE_INSTALL_INCLUDEDIR include/cloud-service)
set(CMAKE_INSTALL_LIBDIR lib/cloud-service)
set(CMAKE_INSTALL_BINDIR bin)

# Установка файлов
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")
install(TARGETS cloud-service
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}) 