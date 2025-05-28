add_subdirectory(basic)
add_subdirectory(advanced)

# Установка примеров
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/cloud-service
    FILES_MATCHING PATTERN "*.cpp"
) 