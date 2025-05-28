add_subdirectory(core)
add_subdirectory(blockchain)
add_subdirectory(network)
add_subdirectory(storage)

# Создание тестового исполняемого файла
add_executable(unit_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/core/CoreEngineTest.cpp
)

target_link_libraries(unit_tests
    PRIVATE
    cloud-service
    GTest::GTest
    GTest::Main
)

# Добавление тестов
add_test(NAME unit_tests COMMAND unit_tests)
