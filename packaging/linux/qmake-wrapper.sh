#!/usr/bin/env bash

set -euo pipefail

: "${KBEXTRACT_REAL_QMAKE:?KBEXTRACT_REAL_QMAKE is required}"
: "${KBEXTRACT_QT_PLUGIN_DIR:?KBEXTRACT_QT_PLUGIN_DIR is required}"

if [[ ${1:-} == "-query" && $# -eq 1 ]]; then
    "$KBEXTRACT_REAL_QMAKE" -query \
        | sed "s|^QT_INSTALL_PLUGINS:.*|QT_INSTALL_PLUGINS:$KBEXTRACT_QT_PLUGIN_DIR|"
elif [[ ${1:-} == "-query" && ${2:-} == "QT_INSTALL_PLUGINS" ]]; then
    printf '%s\n' "$KBEXTRACT_QT_PLUGIN_DIR"
else
    exec "$KBEXTRACT_REAL_QMAKE" "$@"
fi
