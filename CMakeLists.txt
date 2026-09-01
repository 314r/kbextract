cmake_minimum_required(VERSION 3.21)

project(kbextract VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Sql)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(kbextract
    main.cpp
    kobo_library.cpp
    kobo_library.h
    markdown_highlighter.cpp
    markdown_highlighter.h
    omarchy_theme.cpp
    omarchy_theme.h
)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(kbextract
    URI Kbextract
    VERSION 1.0
    QML_FILES
        Main.qml
        Theme.qml
)

target_link_libraries(kbextract
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Sql
)

install(TARGETS kbextract
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES kbextract.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)

    qt_add_executable(kobo_library_tests
        tests/kobo_library_test.cpp
        kobo_library.cpp
        kobo_library.h
    )

    target_include_directories(kobo_library_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(kobo_library_tests PRIVATE Qt6::Core Qt6::Sql Qt6::Test)

    add_test(NAME kobo_library_tests COMMAND kobo_library_tests)

    qt_add_executable(markdown_highlighter_tests
        tests/markdown_highlighter_test.cpp
        markdown_highlighter.cpp
        markdown_highlighter.h
    )

    target_include_directories(markdown_highlighter_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(markdown_highlighter_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Test)

    add_test(NAME markdown_highlighter_tests COMMAND markdown_highlighter_tests)

    qt_add_executable(omarchy_theme_tests
        tests/omarchy_theme_test.cpp
        omarchy_theme.cpp
        omarchy_theme.h
    )

    target_include_directories(omarchy_theme_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(omarchy_theme_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Test)

    add_test(NAME omarchy_theme_tests COMMAND omarchy_theme_tests)
endif()
