import QtQuick
import Kbextract

// One session per window. Platform presentations share commands and data,
// but own their controls, menus, dialogs, and appearance preferences.
QtObject {
    id: session

    readonly property KoboLibrary library: KoboLibrary { }
    readonly property ClipboardHelper clipboard: ClipboardHelper { }
    readonly property FileUrl fileUrl: FileUrl { }
    readonly property bool canCopyText: library.currentBookPlainText.length > 0
    readonly property bool canCopyObsidian: library.currentBookObsidianMarkdown.length > 0
    readonly property bool canCopyMarkdown: library.currentBookMarkdown.length > 0
    readonly property bool textCopied: textFeedback.running
    readonly property bool obsidianCopied: obsidianFeedback.running
    readonly property bool markdownCopied: markdownFeedback.running

    signal copied(string format)

    function openDatabase(url) {
        const path = fileUrl.localPath(url)
        return path.length > 0 && library.addDatabase(path)
    }

    function refresh() {
        library.refreshDevices()
    }

    function selectDevice(index) {
        library.currentDeviceIndex = index
    }

    function selectBook(index) {
        library.currentBookIndex = index
    }

    function copy(format) {
        let source = ""
        let feedback = null
        if (format === "text") {
            source = library.currentBookPlainText
            feedback = textFeedback
        } else if (format === "obsidian") {
            source = library.currentBookObsidianMarkdown
            feedback = obsidianFeedback
        } else if (format === "markdown") {
            source = library.currentBookMarkdown
            feedback = markdownFeedback
        }
        if (!feedback || !clipboard.copyText(source))
            return false
        feedback.restart()
        copied(format)
        return true
    }

    function applySystemAppearance() {
        Theme.systemDark = appearance.dark
        Theme.systemPalette = appearance.palette
        Theme.systemUiFont = appearance.uiFont.family
        Theme.systemMonoFont = appearance.fixedFont.family
    }

    readonly property SystemAppearance appearance: SystemAppearance {
        onPaletteChanged: session.applySystemAppearance()
        onFontsChanged: session.applySystemAppearance()
    }
    readonly property Timer textFeedback: Timer { interval: 1500 }
    readonly property Timer obsidianFeedback: Timer { interval: 1500 }
    readonly property Timer markdownFeedback: Timer { interval: 1500 }
    readonly property Connections libraryConnections: Connections {
        target: session.library
        function onCurrentBookChanged() {
            session.textFeedback.stop()
            session.obsidianFeedback.stop()
            session.markdownFeedback.stop()
        }
    }

    Component.onCompleted: {
        applySystemAppearance()
        refresh()
    }
}
