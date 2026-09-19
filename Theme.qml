pragma Singleton

import QtQuick

QtObject {
    id: theme

    property string mode: "system"
    property bool systemDark: false
    property bool omarchyDark: true
    property bool omarchyAvailable: false

    property var systemPalette: ({
        "background": "#f2f2f3",
        "dark_background": "#ececed",
        "darker_background": "#f7f7f8",
        "lighter_background": "#f8f8f8",
        "foreground": "#25272a",
        "dark_foreground": "#989ba0",
        "light_foreground": "#6e7277",
        "bright_foreground": "#25272a",
        "muted": "#c9cace",
        "accent": "#5980a6",
        "accent_text": "#ffffff",
        "selection": "#dce7f0",
        "red": "#a24d4d",
        "green": "#3f7652",
        "yellow": "#8b671f",
        "cyan": "#3d7089"
    })
    property string systemUiFont: "sans-serif"
    property string systemMonoFont: "monospace"
    property bool adaptiveTypography: false
    property real systemFontPointSize: 9
    property int textScalePercent: 100

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

    readonly property string effectiveMode: mode === "omarchy" && !omarchyAvailable ? "system" : mode
    readonly property bool systemMode: effectiveMode === "system"
    readonly property bool omarchyMode: effectiveMode === "omarchy"
    readonly property bool darkMode: effectiveMode === "dark"
        || (systemMode && systemDark)
        || (omarchyMode && omarchyDark)
    readonly property var activePalette: systemMode ? systemPalette : omarchyPalette

    readonly property color canvas: systemMode ? activePalette.background : (omarchyMode ? activePalette.background : (darkMode ? "#141414" : "#f2f2f3"))
    readonly property color canvasGlass: systemMode ? activePalette.darker_background : (omarchyMode ? activePalette.darker_background : (darkMode ? "#1a1a1a" : "#f7f7f8"))
    readonly property color panel: systemMode ? activePalette.dark_background : (omarchyMode ? activePalette.dark_background : (darkMode ? "#1c1c1c" : "#ececed"))
    readonly property color surface: systemMode ? activePalette.lighter_background : (omarchyMode ? activePalette.lighter_background : (darkMode ? "#242424" : "#f8f8f8"))
    readonly property color surfaceHover: systemMode ? activePalette.selection : (omarchyMode ? activePalette.selection : (darkMode ? "#2b2b2b" : "#e7edf2"))
    readonly property color surfaceSelected: systemMode ? activePalette.selection : (omarchyMode ? activePalette.selection : (darkMode ? "#1d2a35" : "#dce7f0"))
    readonly property color line: systemMode ? activePalette.muted : (omarchyMode ? activePalette.muted : (darkMode ? "#303030" : "#c9cace"))
    readonly property color lineStrong: systemMode ? activePalette.dark_foreground : (omarchyMode ? activePalette.dark_foreground : (darkMode ? "#484848" : "#92969c"))
    readonly property color text: systemMode ? activePalette.foreground : (omarchyMode ? activePalette.foreground : (darkMode ? "#ededed" : "#25272a"))
    readonly property color textMuted: systemMode ? activePalette.light_foreground : (omarchyMode ? activePalette.light_foreground : (darkMode ? "#ababab" : "#6e7277"))
    readonly property color textFaint: systemMode ? activePalette.dark_foreground : (omarchyMode ? activePalette.dark_foreground : (darkMode ? "#767676" : "#989ba0"))
    readonly property color accent: systemMode ? activePalette.accent : (omarchyMode ? activePalette.accent : (darkMode ? "#6da3d8" : "#5980a6"))
    readonly property color accentSoft: systemMode ? activePalette.selection : (omarchyMode ? activePalette.selection : (darkMode ? "#1d2a35" : "#d6e2ed"))
    readonly property color mint: systemMode ? activePalette.green : (omarchyMode ? activePalette.green : (darkMode ? "#7bbf92" : "#5980a6"))
    readonly property color cyan: systemMode ? activePalette.cyan : (omarchyMode ? activePalette.cyan : (darkMode ? "#9fc7db" : "#5980a6"))
    readonly property color warning: systemMode ? activePalette.yellow : (omarchyMode ? activePalette.yellow : (darkMode ? "#c5a570" : "#a67928"))
    readonly property color danger: systemMode ? activePalette.red : (omarchyMode ? activePalette.red : (darkMode ? "#de7979" : "#a24d4d"))
    readonly property color accentText: systemMode ? activePalette.accent_text : (omarchyMode ? activePalette.background : (darkMode ? "#101010" : "#ffffff"))
    readonly property color previewOverlay: omarchyMode ? activePalette.selection : (darkMode ? "#66303a45" : "#665980a6")
    readonly property color previewControl: omarchyMode ? activePalette.dark_background : (darkMode ? "#bb111111" : "#337ca1be")
    readonly property color previewControlBorder: omarchyMode ? activePalette.bright_foreground : (darkMode ? "#b3d3e5" : "#b3d2e6")
    readonly property color previewText: omarchyMode ? activePalette.foreground : (darkMode ? text : "#ffffff")

    readonly property string uiFont: systemUiFont
    readonly property string monoFont: systemMonoFont
    readonly property int effectiveTextScalePercent: Math.max(80, Math.min(200, textScalePercent))
    readonly property real effectiveFontPointSize: Math.max(1, systemFontPointSize)
        * effectiveTextScalePercent / 100
    // These ratios preserve the existing 10/12/16/15/20 hierarchy while the
    // system point size and the user's override remain independent of colors.
    readonly property real fontPointSizeCaption: effectiveFontPointSize * 10 / 12
    readonly property real fontPointSizeBody: effectiveFontPointSize
    readonly property real fontPointSizeHeading: effectiveFontPointSize * 16 / 12
    readonly property real fontPointSizeReader: effectiveFontPointSize * 15 / 12
    readonly property real fontPointSizeReaderHeading: effectiveFontPointSize * 20 / 12
    // The macOS presentation keeps its existing fixed reader sizes for now.
    readonly property int fontSizeCaption: 10
    readonly property int fontSizeBody: 12
    readonly property int fontSizeHeading: 16
    readonly property int fontSizeReader: 15
    readonly property int fontSizeReaderHeading: 20
    // Control chrome follows native GTK/Qt button proportions: vertical padding
    // tracks the font descent (~4px at 11pt) instead of 40% of the line box.
    // 24px is GTK's usual min-height, not the old 30px pixel-UI floor.
    readonly property int controlVerticalPadding: bodyMetrics.height > 0
        ? Math.max(3, Math.round(bodyMetrics.descent))
        : 4
    readonly property int controlHorizontalPadding: bodyMetrics.averageCharWidth > 0
        ? Math.round(bodyMetrics.averageCharWidth)
        : 10
    readonly property int controlMinHeight: Math.max(24,
        Math.round(bodyMetrics.height) + 2 * controlVerticalPadding)
    readonly property int controlIconSize: Math.max(16, Math.round(controlMinHeight * 0.6))
    readonly property int radius: 0
    readonly property int smallRadius: 0

    readonly property FontMetrics bodyMetrics: FontMetrics {
        font.family: theme.uiFont
        font.pointSize: theme.fontPointSizeBody
    }
}
