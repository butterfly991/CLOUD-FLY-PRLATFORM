enable_testing()

# Поиск GTest
find_package(GTest REQUIRED)

# Добавление поддиректорий с тестами
add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(performance)

# Установка тестов
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/cloud-service/tests
    FILES_MATCHING PATTERN "*.cpp"
) 