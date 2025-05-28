# Добавление поддиректорий
add_subdirectory(core)
add_subdirectory(blockchain)
add_subdirectory(network)
add_subdirectory(storage)
add_subdirectory(container)
add_subdirectory(monitoring)
add_subdirectory(load_balancer)
add_subdirectory(accelerators)
add_subdirectory(cache)
add_subdirectory(sync)
add_subdirectory(packet_processor)
add_subdirectory(diagnostics)
add_subdirectory(tracing)
add_subdirectory(ledger)
add_subdirectory(enclave)
add_subdirectory(integration)
add_subdirectory(jit)

# Создание основной библиотеки
add_library(cloud-service
    core/CoreEngine.cpp
    blockchain/distributed_ledger.cpp
    network/network_manager.cpp
    storage/storage_manager.cpp
    container/container_manager.cpp
    monitoring/monitoring_system.cpp
    load_balancer/load_balancer.cpp
    accelerators/accelerators.cpp
    cache/adaptive_cache.cpp
    sync/advanced_synchronization.cpp
    packet_processor/advanced_packet_processor.cpp
    diagnostics/diagnostics.cpp
    tracing/distributed_tracing.cpp
    enclave/enclave.cpp
    integration/integration.cpp
    jit/jit_compiler.cpp
)

# Установка свойств библиотеки
set_target_properties(cloud-service PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/core/CoreEngine.h"
)

# Линковка с зависимостями
target_link_libraries(cloud-service
    PRIVATE
    core-lib
    blockchain-lib
    network-lib
    storage-lib
    container-lib
    monitoring-lib
    load-balancer-lib
    accelerators-lib
    cache-lib
    sync-lib
    packet-processor-lib
    diagnostics-lib
    tracing-lib
    ledger-lib
    enclave-lib
    integration-lib
    jit-lib
) 