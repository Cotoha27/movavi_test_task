cmake_minimum_required(VERSION 3.1.0)

project(movavi_test_task)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(movavi_test_task
    main.cpp

    main_window.h
    main_window.cpp
)

target_link_libraries(movavi_test_task Qt5::Widgets Qt5::Core Qt5::Gui)

get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

add_custom_command(TARGET movavi_test_task POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E
        env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
            --verbose 0
            --no-compiler-runtime
            "$<TARGET_FILE:movavi_test_task>"
    COMMENT "Deploying Qt..."
)