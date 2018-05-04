cmake_minimum_required(VERSION 2.8.12)

project(ce30_test)

find_package(Qt5Widgets)
find_package(ce30_driver 1.0.3 REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

add_executable(
  ${PROJECT_NAME}
    main.cpp
    main_window.cpp
    main_window.h
    main_window.ui
)

target_link_libraries(
  ${PROJECT_NAME}
    ${CE30_DRIVER_LIBRARIES}
    Qt5::Widgets
)
