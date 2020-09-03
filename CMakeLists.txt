cmake_minimum_required(VERSION 3.1.0)

project(movavi_test_task)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(movavi_test_task
    main.cpp
)

target_link_libraries(movavi_test_task Qt5::Widgets)
