pragma Singleton

import QtQuick

QtObject {
    property string mode: "omarchy"
    property bool omarchyDark: true
    property var omarchyPalette: ({
        "background": "#030203",
        "dark_background": "#080708",
        "darker_background": "#050505",
        "lighter_background": "#232223",
        "foreground": "#b5bfc4",
        "dark_foreground": "#747b7f",
        "light_foreground": "#a9b2b6",
        "bright_foreground": "#b3bbbf",
        "muted": "#5c5c5c",
        "accent": "#b85d4d",
        "selection": "#2d2b2d",
        "red": "#b85d4d",
        "green": "#687d60",
        "yellow": "#877364",
        "cyan": "#b5bfc4"
    })
    property var omarchyFontSizes: ({
        "caption": 10,
        "body": 12,
        "heading": 16
    })

    readonly property bool omarchyMode: mode === "omarchy"
    readonly property bool darkMode: mode === "dark" || (omarchyMode && omarchyDark)
    readonly property color canvas: omarchyMode ? omarchyPalette.background : (darkMode ? "#141414" : "#f2f2f3")
    readonly property color canvasGlass: omarchyMode ? omarchyPalette.darker_background : (darkMode ? "#1a1a1a" : "#f7f7f8")
    readonly property color panel: omarchyMode ? omarchyPalette.dark_background : (darkMode ? "#1c1c1c" : "#ececed")
    readonly property color surface: omarchyMode ? omarchyPalette.lighter_background : (darkMode ? "#242424" : "#f8f8f8")
    readonly property color surfaceHover: omarchyMode ? omarchyPalette.selection : (darkMode ? "#2b2b2b" : "#e7edf2")
    readonly property color surfaceSelected: omarchyMode ? omarchyPalette.selection : (darkMode ? "#1d2a35" : "#dce7f0")
    readonly property color line: omarchyMode ? omarchyPalette.muted : (darkMode ? "#303030" : "#c9cace")
    readonly property color lineStrong: omarchyMode ? omarchyPalette.dark_foreground : (darkMode ? "#484848" : "#92969c")
    readonly property color text: omarchyMode ? omarchyPalette.foreground : (darkMode ? "#ededed" : "#25272a")
    readonly property color textMuted: omarchyMode ? omarchyPalette.light_foreground : (darkMode ? "#ababab" : "#6e7277")
    readonly property color textFaint: omarchyMode ? omarchyPalette.dark_foreground : (darkMode ? "#767676" : "#989ba0")
    readonly property color accent: omarchyMode ? omarchyPalette.accent : (darkMode ? "#6da3d8" : "#5980a6")
    readonly property color accentSoft: omarchyMode ? omarchyPalette.selection : (darkMode ? "#1d2a35" : "#d6e2ed")
    readonly property color mint: omarchyMode ? omarchyPalette.green : (darkMode ? "#7bbf92" : "#5980a6")
    readonly property color cyan: omarchyMode ? omarchyPalette.cyan : (darkMode ? "#9fc7db" : "#5980a6")
    readonly property color warning: omarchyMode ? omarchyPalette.yellow : (darkMode ? "#c5a570" : "#a67928")
    readonly property color danger: omarchyMode ? omarchyPalette.red : (darkMode ? "#de7979" : "#a24d4d")
    readonly property color accentText: omarchyMode ? omarchyPalette.background : (darkMode ? "#101010" : "#ffffff")
    readonly property color previewOverlay: omarchyMode ? omarchyPalette.selection : (darkMode ? "#66303a45" : "#665980a6")
    readonly property color previewControl: omarchyMode ? omarchyPalette.dark_background : (darkMode ? "#bb111111" : "#337ca1be")
    readonly property color previewControlBorder: omarchyMode ? omarchyPalette.bright_foreground : (darkMode ? "#b3d3e5" : "#b3d2e6")
    readonly property color previewText: omarchyMode ? omarchyPalette.foreground : (darkMode ? text : "#ffffff")
    readonly property string uiFont: "Barlow"
    readonly property string monoFont: "JetBrains Mono"
    readonly property int fontSizeCaption: omarchyFontSizes.caption
    readonly property int fontSizeBody: omarchyFontSizes.body
    readonly property int fontSizeHeading: omarchyFontSizes.heading
    readonly property int radius: 0
    readonly property int smallRadius: 0
}
