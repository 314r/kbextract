cmake_minimum_required(VERSION 3.21)

project(kbextract VERSION 0.1.0 LANGUAGES CXX)

set(KBEXTRACT_APP_ID "io.github._314r.kbextract")
set(KBEXTRACT_PACKAGE_VERSION "${PROJECT_VERSION}" CACHE STRING "Version used in package metadata")
if(NOT KBEXTRACT_PACKAGE_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    message(FATAL_ERROR "KBEXTRACT_PACKAGE_VERSION must use MAJOR.MINOR.PATCH format")
endif()
set(KBEXTRACT_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(KBEXTRACT_VERSION_MINOR "${CMAKE_MATCH_2}")
set(KBEXTRACT_VERSION_PATCH "${CMAKE_MATCH_3}")

if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Sql Svg)

qt_standard_project_setup(REQUIRES 6.9)

qt_add_executable(kbextract
    main.cpp
    file_url.h
    clipboard_helper.cpp
    clipboard_helper.h
    kobo_library.cpp
    kobo_library.h
    markdown_highlighter.cpp
    markdown_highlighter.h
    omarchy_theme.cpp
    omarchy_theme.h
    system_appearance.cpp
    system_appearance.h
)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

set(KBEXTRACT_PLATFORM_QML)
if(APPLE)
    list(APPEND KBEXTRACT_PLATFORM_QML MacMain.qml MacCopyButton.qml)
endif()

qt_add_qml_module(kbextract
    URI Kbextract
    VERSION 1.0
    QML_FILES
        AppSession.qml
        AnnotationBody.qml
        AppearanceSettingsPage.qml
        Main.qml
        SettingsWindow.qml
        Theme.qml
        ${KBEXTRACT_PLATFORM_QML}
    RESOURCES
        assets/icons/theme-dark.svg
        assets/icons/theme-light.svg
        assets/icons/theme-omarchy.svg
        assets/icons/theme-system.svg
        assets/icons/kbextract.svg
)

target_link_libraries(kbextract
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Sql
        Qt6::Svg
)
target_compile_definitions(kbextract PRIVATE KBEXTRACT_VERSION="${KBEXTRACT_PACKAGE_VERSION}")

if(APPLE)
    find_program(ICONUTIL_EXECUTABLE iconutil REQUIRED)
    set(KBEXTRACT_MACOS_ICON "${CMAKE_CURRENT_BINARY_DIR}/kbextract.icns")
    set(KBEXTRACT_ICONSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/kbextract.iconset")
    set(KBEXTRACT_ICONSET_FILES
        "${KBEXTRACT_ICONSET_DIR}/icon_16x16.png"
        "${KBEXTRACT_ICONSET_DIR}/icon_16x16@2x.png"
        "${KBEXTRACT_ICONSET_DIR}/icon_32x32.png"
        "${KBEXTRACT_ICONSET_DIR}/icon_32x32@2x.png"
        "${KBEXTRACT_ICONSET_DIR}/icon_128x128.png"
        "${KBEXTRACT_ICONSET_DIR}/icon_128x128@2x.png"
        "${KBEXTRACT_ICONSET_DIR}/icon_256x256.png"
        "${KBEXTRACT_ICONSET_DIR}/icon_256x256@2x.png"
        "${KBEXTRACT_ICONSET_DIR}/icon_512x512.png"
        "${KBEXTRACT_ICONSET_DIR}/icon_512x512@2x.png"
    )
    add_custom_command(
        OUTPUT "${KBEXTRACT_MACOS_ICON}"
        COMMAND "${ICONUTIL_EXECUTABLE}" -c icns "${KBEXTRACT_ICONSET_DIR}" -o "${KBEXTRACT_MACOS_ICON}"
        DEPENDS ${KBEXTRACT_ICONSET_FILES}
        VERBATIM
    )
    set_source_files_properties("${KBEXTRACT_MACOS_ICON}" PROPERTIES
        GENERATED TRUE
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(kbextract PRIVATE "${KBEXTRACT_MACOS_ICON}")
    set_target_properties(kbextract PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "kbextract"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${KBEXTRACT_APP_ID}"
        MACOSX_BUNDLE_ICON_FILE "kbextract.icns"
        MACOSX_BUNDLE_BUNDLE_VERSION "${KBEXTRACT_PACKAGE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${KBEXTRACT_PACKAGE_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 314r"
    )
elseif(WIN32)
    configure_file(
        packaging/windows/kbextract.rc.in
        "${CMAKE_CURRENT_BINARY_DIR}/kbextract.rc"
        @ONLY
    )
    target_sources(kbextract PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/kbextract.rc")
    set_target_properties(kbextract PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS kbextract
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES "${KBEXTRACT_APP_ID}.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    )
    install(FILES "${KBEXTRACT_APP_ID}.metainfo.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
        RENAME "${KBEXTRACT_APP_ID}.appdata.xml"
    )
    install(FILES assets/icons/kbextract.svg
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
        RENAME "${KBEXTRACT_APP_ID}.svg"
    )
    foreach(icon_size 16 32 48 64 128 256 512)
        install(FILES "assets/icons/hicolor/kbextract-${icon_size}.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${icon_size}x${icon_size}/apps"
            RENAME "${KBEXTRACT_APP_ID}.png"
        )
    endforeach()
    install(FILES LICENSE
        DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/kbextract"
        RENAME copyright
    )
endif()

if(APPLE OR WIN32)
    set(deploy_options NO_UNSUPPORTED_PLATFORM_ERROR)
    if(APPLE)
        list(APPEND deploy_options MACOS_BUNDLE_POST_BUILD)
    endif()
    qt_generate_deploy_qml_app_script(
        TARGET kbextract
        OUTPUT_SCRIPT deploy_script
        ${deploy_options}
    )
    install(SCRIPT "${deploy_script}")
endif()

if(BUILD_TESTING)
    find_package(Qt6 6.9 REQUIRED COMPONENTS Test)

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

    qt_add_executable(system_appearance_tests
        tests/system_appearance_test.cpp
        system_appearance.cpp
        system_appearance.h
    )

    target_include_directories(system_appearance_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(system_appearance_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Test)

    add_test(NAME system_appearance_tests COMMAND system_appearance_tests)
    set_tests_properties(system_appearance_tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORMTHEME="
    )

    qt_add_executable(appearance_ui_tests
        tests/appearance_ui_test.cpp
        file_url.h
        clipboard_helper.cpp clipboard_helper.h
        kobo_library.cpp kobo_library.h
        markdown_highlighter.cpp markdown_highlighter.h
        omarchy_theme.cpp omarchy_theme.h
        system_appearance.cpp system_appearance.h
    )
    target_include_directories(appearance_ui_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(appearance_ui_tests PRIVATE
        Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
        Qt6::Sql Qt6::Svg Qt6::Test
    )
    qt_add_resources(appearance_ui_tests "appearance_ui"
        PREFIX "/qt/qml/Kbextract"
        FILES
            Main.qml SettingsWindow.qml AppearanceSettingsPage.qml Theme.qml
            AppSession.qml AnnotationBody.qml
            assets/icons/theme-system.svg assets/icons/theme-light.svg
            assets/icons/theme-dark.svg assets/icons/theme-omarchy.svg
    )
    foreach(scale IN ITEMS 1 1.5 2)
        add_test(NAME appearance_ui_tests_scale_${scale} COMMAND appearance_ui_tests)
        set_tests_properties(appearance_ui_tests_scale_${scale} PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORMTHEME=;QT_QUICK_BACKEND=software;QT_SCALE_FACTOR=${scale}"
        )
    endforeach()

    if(APPLE)
        qt_add_executable(mac_ui_tests
            tests/mac_ui_test.cpp
            file_url.h
            clipboard_helper.cpp clipboard_helper.h
            kobo_library.cpp kobo_library.h
            markdown_highlighter.cpp markdown_highlighter.h
            system_appearance.cpp system_appearance.h
        )
        target_include_directories(mac_ui_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(mac_ui_tests PRIVATE
            Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Sql Qt6::Test
        )
        qt_add_resources(mac_ui_tests "mac_ui"
            PREFIX "/qt/qml/Kbextract"
            FILES MacMain.qml MacCopyButton.qml AppSession.qml AnnotationBody.qml Theme.qml
        )
        add_test(NAME mac_ui_tests COMMAND mac_ui_tests)
        set_tests_properties(mac_ui_tests PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORMTHEME=;QT_QUICK_BACKEND=software"
        )
    endif()

    qt_add_executable(clipboard_helper_tests
        tests/clipboard_helper_test.cpp
        clipboard_helper.cpp
        clipboard_helper.h
    )

    target_include_directories(clipboard_helper_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(clipboard_helper_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Test)

    add_test(NAME clipboard_helper_tests COMMAND clipboard_helper_tests)
    set_tests_properties(clipboard_helper_tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORMTHEME="
    )
endif()

set(CPACK_PACKAGE_NAME "kbextract")
set(CPACK_PACKAGE_VENDOR "314r")
set(CPACK_PACKAGE_CONTACT "contact.314r@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Browse and copy highlights and notes from Kobo eReaders")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/314r/kbextract")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "kbextract")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "kbextract")
    set(CPACK_PACKAGE_FILE_NAME "kbextract-${KBEXTRACT_PACKAGE_VERSION}-macos-${CMAKE_SYSTEM_PROCESSOR}")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_FILE_NAME "kbextract-${KBEXTRACT_PACKAGE_VERSION}-windows-x86_64")
    set(CPACK_PACKAGE_EXECUTABLES "kbextract" "kbextract")
    set(CPACK_CREATE_DESKTOP_LINKS "kbextract")
    set(CPACK_NSIS_DISPLAY_NAME "kbextract")
    set(CPACK_NSIS_PACKAGE_NAME "kbextract")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\kbextract.exe")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/kbextract.ico")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
endif()

if(APPLE OR WIN32)
    include(CPack)
endif()
