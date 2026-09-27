# Install a pinned Kirigami into the CI Qt SDK so QML scanners and deployment
# tools find it alongside Qt's own modules. No Plasma or Kirigami Addons needed.
cmake_minimum_required(VERSION 3.21)
if(NOT DEFINED QT_PREFIX OR NOT DEFINED WORK_DIR)
    message(FATAL_ERROR "Pass -DQT_PREFIX=<Qt SDK> -DWORK_DIR=<dependency build directory>")
endif()
get_filename_component(QT_PREFIX "${QT_PREFIX}" ABSOLUTE)
get_filename_component(WORK_DIR "${WORK_DIR}" ABSOLUTE)
file(MAKE_DIRECTORY "${WORK_DIR}")

function(run)
    execute_process(COMMAND ${ARGV} COMMAND_ERROR_IS_FATAL ANY)
endfunction()

# Git for Windows ships an MSYS tar ahead of System32 on PATH. That tar treats
# "D:" in a Windows path as a hostname, so extracting templates/kirigami6.tar.bz2
# fails with "Cannot connect to D:". Windows tar must come first. Keep CC/CXX
# (MSVC cl on CI) unchanged.
if(WIN32)
    set(_kbextract_system32 "$ENV{SystemRoot}/System32")
    set(ENV{PATH} "${_kbextract_system32};$ENV{PATH}")
endif()

set(version 6.13.0)
# Honor CC/CXX from the environment. On Windows CI this is cl, so Ninja does
# not probe MinGW's cc.exe and link it against the MSVC Qt libraries.
set(toolchain_args "")
if(DEFINED ENV{CC} AND NOT "$ENV{CC}" STREQUAL "")
    list(APPEND toolchain_args "-DCMAKE_C_COMPILER=$ENV{CC}")
endif()
if(DEFINED ENV{CXX} AND NOT "$ENV{CXX}" STREQUAL "")
    list(APPEND toolchain_args "-DCMAKE_CXX_COMPILER=$ENV{CXX}")
endif()
foreach(dependency IN ITEMS extra-cmake-modules kirigami)
    if(dependency STREQUAL "extra-cmake-modules")
        set(checksum 7006017c00c817ff4c056995146d271791d1487a398d39ea6cac1cd59a8bf402)
    else()
        set(checksum dd5aa1b5b8fbc4eb731227851af7ca9caa1dcaac0dc99421dbd3d0d58d988329)
    endif()
    set(archive "${WORK_DIR}/${dependency}-${version}.tar.xz")
    file(DOWNLOAD
        "https://download.kde.org/stable/frameworks/6.13/${dependency}-${version}.tar.xz"
        "${archive}" EXPECTED_HASH "SHA256=${checksum}" TLS_VERIFY ON)
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${WORK_DIR}")
    run("${CMAKE_COMMAND}" -S "${WORK_DIR}/${dependency}-${version}"
        -B "${WORK_DIR}/${dependency}-build" -G Ninja
        ${toolchain_args}
        "-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_INSTALL_PREFIX=${QT_PREFIX}"
        "-DCMAKE_PREFIX_PATH=${QT_PREFIX}" "-DBUILD_TESTING=OFF"
        "-DBUILD_QCH=OFF" "-DBUILD_DOC=OFF" "-DBUILD_EXAMPLES=OFF"
        "-DKDE_INSTALL_QMLDIR=${QT_PREFIX}/qml"
        "-DKDE_INSTALL_LIBDIR=lib" "-DKDE_INSTALL_USE_QT_SYS_PATHS=OFF")
    run("${CMAKE_COMMAND}" --build "${WORK_DIR}/${dependency}-build" --parallel 4)
    run("${CMAKE_COMMAND}" --install "${WORK_DIR}/${dependency}-build")
endforeach()
