cmake_minimum_required(VERSION 3.20)

project(kgrapher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(QT_MIN_VERSION "6.0.0")

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Widgets
)

add_executable(kgrapher
    main.cpp
    mainwindow.cpp
    expressionparser.cpp
    graphwidget.cpp
    graphwidget3d.cpp
)

set_target_properties(kgrapher PROPERTIES
    AUTOMOC ON
)

target_link_libraries(kgrapher
    Qt6::Core
    Qt6::Widgets
)

install(TARGETS kgrapher
    RUNTIME DESTINATION bin
)

# Desktop entry for application menu (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/kgrapher.desktop")
    install(FILES kgrapher.desktop DESTINATION share/applications)
endif()
