add_executable(basic_example main.cpp)

target_link_libraries(basic_example
    PRIVATE
    cloud-service
)

# Установка примера
install(
    TARGETS basic_example
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
) 