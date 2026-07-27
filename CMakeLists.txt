cmake_minimum_required(VERSION 3.16)
project(crossword LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Поддержка как Qt5, так и Qt6
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(crossword
    main.cpp
    mainwindow.cpp
    mainwindow.h
    generator.cpp
    generator.h
    cellwidget.cpp
    cellwidget.h
    words.cpp
    words.h
)

target_link_libraries(crossword PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if (MSVC)
    target_compile_options(crossword PRIVATE /utf-8)
endif()

# Копируем словарь рядом с исполняемым файлом
add_custom_command(TARGET crossword POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/words.txt"
        "$<TARGET_FILE_DIR:crossword>/words.txt"
)
