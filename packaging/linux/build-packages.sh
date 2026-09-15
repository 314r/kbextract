#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt 1 || $# -gt 3 ]]; then
    echo "usage: $0 BUILD_DIR [OUTPUT_DIR] [VERSION]" >&2
    exit 2
fi

source_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
build_dir=$(cd "$1" && pwd)
output_dir=${2:-"$build_dir/packages"}
package_version=${3:-"0.1.0"}
staging_root="$build_dir/linux-package"
app_dir="$staging_root/kbextract.AppDir"
filtered_plugin_dir="$staging_root/qt-plugins"

linuxdeploy=${LINUXDEPLOY:-linuxdeploy}
appimagetool=${APPIMAGETOOL:-appimagetool}
cpack_command=${CPACK:-cpack}
qmake_command=${QMAKE:-}

for required_command in "$linuxdeploy" "$appimagetool" "$cpack_command"; do
    if ! command -v "$required_command" >/dev/null 2>&1 && [[ ! -x "$required_command" ]]; then
        echo "required packaging tool not found: $required_command" >&2
        exit 1
    fi
done

if [[ -z "$qmake_command" ]]; then
    if command -v qmake6 >/dev/null 2>&1; then
        qmake_command=$(command -v qmake6)
    elif command -v qmake >/dev/null 2>&1; then
        qmake_command=$(command -v qmake)
    else
        echo "Qt qmake was not found; set QMAKE to the Qt 6.9 qmake executable" >&2
        exit 1
    fi
fi

cmake -E rm -rf "$staging_root"
cmake -E make_directory "$app_dir" "$output_dir" "$filtered_plugin_dir"
cmake --install "$build_dir" --prefix "$app_dir/usr" --strip

export QML_SOURCES_PATHS="$source_dir"
export EXTRA_QT_MODULES="svg"

real_qt_plugin_dir=$("$qmake_command" -query QT_INSTALL_PLUGINS)
plugin_files=(
    platforms/libqxcb.so
    platforms/libqoffscreen.so
    platforms/libqwayland.so
    platforms/libqwayland-egl.so
    platforms/libqwayland-generic.so
    sqldrivers/libqsqlite.so
    iconengines/libqsvgicon.so
    imageformats/libqsvg.so
    platformthemes/libqxdgdesktopportal.so
    platforminputcontexts/libcomposeplatforminputcontextplugin.so
)
for plugin_file in "${plugin_files[@]}"; do
    if [[ -f "$real_qt_plugin_dir/$plugin_file" ]]; then
        cmake -E make_directory "$filtered_plugin_dir/$(dirname "$plugin_file")"
        cmake -E copy_if_different \
            "$real_qt_plugin_dir/$plugin_file" \
            "$filtered_plugin_dir/$plugin_file"
    fi
done

export KBEXTRACT_REAL_QMAKE="$qmake_command"
export KBEXTRACT_QT_PLUGIN_DIR="$filtered_plugin_dir"
export QMAKE="$source_dir/packaging/linux/qmake-wrapper.sh"

available_platform_plugins=""
for platform_plugin in libqoffscreen.so libqwayland.so libqwayland-egl.so libqwayland-generic.so; do
    if [[ -f "$filtered_plugin_dir/platforms/$platform_plugin" ]]; then
        available_platform_plugins+="$platform_plugin;"
    fi
done
if [[ -n "$available_platform_plugins" ]]; then
    export EXTRA_PLATFORM_PLUGINS="$available_platform_plugins"
else
    unset EXTRA_PLATFORM_PLUGINS || true
fi

# linuxdeploy bundles an older strip on some releases that cannot read newer
# ELF relocation sections (for example .relr.dyn). Keep the deployed binaries
# intact instead of failing a correct package build during deferred operations.
export NO_STRIP=1

"$linuxdeploy" \
    --appdir "$app_dir" \
    --executable "$app_dir/usr/bin/kbextract" \
    --desktop-file "$app_dir/usr/share/applications/io.github._314r.kbextract.desktop" \
    --icon-file "$app_dir/usr/share/icons/hicolor/scalable/apps/io.github._314r.kbextract.svg" \
    --plugin qt

ARCH=x86_64 "$appimagetool" \
    "$app_dir" \
    "$output_dir/kbextract-${package_version}-linux-x86_64.AppImage"

"$cpack_command" \
    --config "$source_dir/packaging/linux/CPackAppDir.cmake" \
    -B "$output_dir" \
    -D "CPACK_INSTALLED_DIRECTORIES=$app_dir/usr;/usr" \
    -D "CPACK_PACKAGE_VERSION=$package_version" \
    -D "CPACK_PACKAGE_FILE_NAME=kbextract-${package_version}-linux-x86_64"
