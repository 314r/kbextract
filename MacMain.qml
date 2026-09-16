pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.macOS
import QtQuick.Controls.Basic as Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import QtCore
import Kbextract

ApplicationWindow {
    id: window
    objectName: "macWindow"
    visible: true
    width: 1360
    height: 900
    minimumWidth: 1040
    minimumHeight: 680
    title: qsTr("kbextract")
    color: palette.base

    AppSession { id: session; objectName: "session" }
    readonly property var library: session.library

    // The custom presentation owns Appearance/colorMode. Do not read or write
    // that preference here, or start an Omarchy watcher on macOS.
    Component.onCompleted: Theme.mode = "system"

    Settings {
        id: windowSettings
        category: "MacWindow"
        property real sidebarWidth: 290
    }

    FileDialog {
        id: databaseDialog
        objectName: "databaseDialog"
        title: qsTr("Choose KoboReader.sqlite")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Kobo database (KoboReader.sqlite)"),
            qsTr("SQLite databases (*.sqlite *.db)"), qsTr("All files (*)")]
        onAccepted: session.openDatabase(selectedFile)
    }

    Action {
        id: openAction
        objectName: "openAction"
        text: qsTr("Open Database…")
        shortcut: StandardKey.Open
        onTriggered: databaseDialog.open()
    }
    Action {
        id: refreshAction
        objectName: "refreshAction"
        text: qsTr("Refresh Devices")
        shortcut: "Ctrl+R"
        onTriggered: session.refresh()
    }
    Action {
        id: closeAction
        objectName: "closeAction"
        text: qsTr("Close Window")
        shortcut: StandardKey.Close
        onTriggered: window.close()
    }
    Action {
        id: copySelectionAction
        objectName: "copySelectionAction"
        text: qsTr("Copy")
        shortcut: StandardKey.Copy
        enabled: annotationScroll.visible && annotationText.selectedText.length > 0
        onTriggered: annotationText.copy()
    }
    Action {
        id: selectAllAction
        objectName: "selectAllAction"
        text: qsTr("Select All")
        shortcut: StandardKey.SelectAll
        enabled: annotationScroll.visible
        onTriggered: {
            annotationText.forceActiveFocus()
            annotationText.selectAll()
        }
    }
    Action {
        id: copyTextAction
        objectName: "copyTextAction"
        text: qsTr("Copy text")
        enabled: session.canCopyText
        onTriggered: session.copy("text")
    }
    Action {
        id: copyObsidianAction
        objectName: "copyObsidianAction"
        text: qsTr("Copy Obsidian Markdown")
        enabled: session.canCopyObsidian
        onTriggered: session.copy("obsidian")
    }
    Action {
        id: copyMarkdownAction
        objectName: "copyMarkdownAction"
        text: qsTr("Copy Markdown")
        enabled: session.canCopyMarkdown
        onTriggered: session.copy("markdown")
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem { action: openAction }
            MenuItem { action: refreshAction }
            MenuSeparator { }
            MenuItem { action: closeAction }
        }
        Menu {
            title: qsTr("Edit")
            MenuItem { action: copySelectionAction }
            MenuItem { action: selectAllAction }
            MenuSeparator { }
            Menu {
                title: qsTr("Copy All As")
                MenuItem { action: copyTextAction }
                MenuItem { action: copyObsidianAction }
                MenuItem { action: copyMarkdownAction }
            }
        }
        // Cocoa supplies the application menu. Its automatic Window menu is
        // hidden, so expose the single-window commands explicitly.
        Menu {
            title: qsTr("Window")
            Action {
                objectName: "minimizeAction"
                text: qsTr("Minimize")
                shortcut: "Ctrl+M"
                onTriggered: window.showMinimized()
            }
            MenuItem {
                text: qsTr("Zoom")
                enabled: window.visibility !== Window.FullScreen
                onTriggered: {
                    if (window.visibility === Window.Maximized)
                        window.showNormal()
                    else
                        window.showMaximized()
                }
            }
            MenuSeparator { }
            MenuItem {
                text: qsTr("Bring All to Front")
                onTriggered: {
                    if (window.visibility === Window.Minimized)
                        window.showNormal()
                    window.raise()
                    window.requestActivate()
                }
            }
        }
    }

    SplitView {
        id: splitView
        objectName: "mainSplitView"
        anchors.fill: parent
        orientation: Qt.Horizontal
        handle: Rectangle {
            id: sidebarDivider
            objectName: "sidebarDivider"
            z: 1
            implicitWidth: 1
            color: Qt.tint(window.palette.base, Qt.rgba(window.palette.text.r,
                window.palette.text.g, window.palette.text.b, 0.12))

            // Keep the boundary visually quiet while retaining an easy drag target.
            containmentMask: Item {
                x: (sidebarDivider.width - width) / 2
                width: 8
                height: splitView.height
            }
        }
        onResizingChanged: {
            if (!resizing)
                windowSettings.sidebarWidth = sidebar.width
        }

        Basic.Pane {
            id: sidebar
            objectName: "sidebar"
            SplitView.minimumWidth: 246
            SplitView.maximumWidth: 420
            SplitView.preferredWidth: Math.max(246, Math.min(420, windowSettings.sidebarWidth))
            padding: 0
            // A pane receives presses in empty sidebar space, allowing SplitView
            // to filter them through the divider's extended containment mask.
            background: Rectangle {
                color: Qt.tint(window.palette.base, Qt.rgba(window.palette.text.r,
                    window.palette.text.g, window.palette.text.b, Theme.systemDark ? 0.06 : 0.04))
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Label { text: qsTr("Device"); font.bold: true }
                ComboBox {
                    id: deviceSelector
                    objectName: "deviceSelector"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    model: window.library.devices
                    textRole: "displayName"
                    currentIndex: window.library.currentDeviceIndex
                    displayText: count ? currentText : qsTr("No Kobo device")
                    enabled: count > 0
                    Accessible.name: qsTr("Kobo device")
                    onActivated: index => session.selectDevice(index)
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        objectName: "refreshButton"
                        action: refreshAction
                        text: qsTr("Refresh")
                    }
                    Button {
                        objectName: "openButton"
                        action: openAction
                        text: qsTr("Open…")
                        Accessible.name: qsTr("Open database")
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("Books"); font.bold: true }
                    Item { Layout.fillWidth: true }
                    Label { text: window.library.books.length; color: window.palette.placeholderText }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: bookList
                        objectName: "bookList"
                        anchors.fill: parent
                        clip: true
                        model: window.library.books
                        currentIndex: window.library.currentBookIndex
                        activeFocusOnTab: true
                        keyNavigationEnabled: false
                        Accessible.role: Accessible.List
                        Accessible.name: qsTr("Books with highlights and notes")

                        function moveSelection(offset) {
                            if (count > 0) {
                                session.selectBook(Math.max(0, Math.min(count - 1,
                                    currentIndex < 0 ? 0 : currentIndex + offset)))
                                positionViewAtIndex(currentIndex, ListView.Contain)
                            }
                        }
                        Keys.onUpPressed: moveSelection(-1)
                        Keys.onDownPressed: moveSelection(1)
                        Keys.onPressed: event => {
                            if (count && event.key === Qt.Key_Home) {
                                session.selectBook(0)
                                positionViewAtBeginning()
                                event.accepted = true
                            } else if (count && event.key === Qt.Key_End) {
                                session.selectBook(count - 1)
                                positionViewAtEnd()
                                event.accepted = true
                            }
                        }

                        // Book metadata is custom content, so use a customizable
                        // Basic delegate with platform palette roles, not overrides
                        // of a native-style control's internals.
                        delegate: Basic.ItemDelegate {
                            id: bookRow
                            objectName: "bookRow"
                            required property int index
                            required property var modelData
                            readonly property bool selected: bookList.currentIndex === index
                            readonly property bool activeSelection: window.active && bookList.activeFocus
                            readonly property color rowText: selected && activeSelection
                                ? window.palette.active.highlightedText : window.palette.text
                            width: bookList.width
                            padding: 8
                            implicitHeight: metadata.implicitHeight + topPadding + bottomPadding
                            activeFocusOnTab: false
                            Accessible.role: Accessible.ListItem
                            Accessible.name: modelData.title + ", " + modelData.author + ", " + counts.text
                            Accessible.selected: selected
                            onClicked: {
                                bookList.forceActiveFocus()
                                session.selectBook(index)
                            }
                            contentItem: Column {
                                id: metadata
                                spacing: 4
                                Label {
                                    width: parent.width
                                    text: bookRow.modelData.title
                                    font.bold: true
                                    color: bookRow.rowText
                                    elide: Text.ElideRight
                                }
                                Label {
                                    width: parent.width
                                    text: bookRow.modelData.author
                                    visible: text.length > 0
                                    color: bookRow.rowText
                                    elide: Text.ElideRight
                                }
                                Label {
                                    id: counts
                                    width: parent.width
                                    text: qsTr("%1 highlight%2 · %3 note%4")
                                        .arg(bookRow.modelData.highlightCount)
                                        .arg(bookRow.modelData.highlightCount === 1 ? "" : "s")
                                        .arg(bookRow.modelData.noteCount)
                                        .arg(bookRow.modelData.noteCount === 1 ? "" : "s")
                                    color: bookRow.selected ? bookRow.rowText : window.palette.placeholderText
                                    elide: Text.ElideRight
                                }
                            }
                            background: Rectangle {
                                radius: 5
                                color: bookRow.selected
                                    ? (bookRow.activeSelection ? window.palette.active.highlight
                                        : window.palette.inactive.highlight)
                                    : "transparent"
                            }
                        }
                        ScrollBar.vertical: ScrollBar { }
                    }
                    Label {
                        anchors.centerIn: parent
                        width: parent.width
                        visible: bookList.count === 0
                        text: window.library.statusText
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        color: window.palette.placeholderText
                    }
                }
            }
        }

        ColumnLayout {
            id: mainColumn
            objectName: "mainColumn"
            SplitView.fillWidth: true
            spacing: 0
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 24
                visible: window.library.currentBookIndex >= 0
                Label {
                    Layout.fillWidth: true
                    text: window.library.currentBookTitle
                    font.bold: true
                    font.pointSize: window.font.pointSize + 3
                    elide: Text.ElideRight
                }
                Label {
                    Layout.fillWidth: true
                    text: window.library.currentBookAuthor
                    visible: text.length > 0
                    elide: Text.ElideRight
                }
            }
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ScrollView {
                    id: annotationScroll
                    objectName: "annotationScroll"
                    anchors.fill: parent
                    clip: true
                    visible: window.library.currentBookIndex >= 0 && session.canCopyMarkdown
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    function resetToTop() {
                        annotationText.deselect()
                        annotationText.cursorPosition = 0
                        contentItem.cancelFlick()
                        contentItem.contentY = contentItem.originY
                        contentItem.returnToBounds()
                    }
                    Connections {
                        target: window.library
                        function onCurrentBookChanged() {
                            annotationScroll.resetToTop()
                            Qt.callLater(annotationScroll.resetToTop)
                        }
                    }
                    Flickable {
                        id: readerFlickable
                        width: annotationScroll.availableWidth
                        height: annotationScroll.availableHeight
                        contentWidth: width
                        contentHeight: annotationText.height
                        flickableDirection: Flickable.VerticalFlick
                        boundsBehavior: Flickable.StopAtBounds
                        AnnotationBody {
                            id: annotationText
                            width: readerFlickable.width
                            markdown: window.library.currentBookMarkdown
                            ContextMenu.menu: Menu {
                                objectName: "readerContextMenu"
                                popupType: Popup.Native
                                MenuItem { action: copySelectionAction }
                                MenuItem { action: selectAllAction }
                            }
                        }
                    }
                }
                Label {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 48, 480)
                    visible: !annotationScroll.visible
                    text: window.library.currentBookIndex < 0
                        ? qsTr("Select a book to view its highlights and notes.")
                        : window.library.annotationStatusText
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    color: window.palette.placeholderText
                }
            }

            Item {
                id: copyFooter
                objectName: "copyFooter"
                Layout.fillWidth: true
                Layout.preferredHeight: copyButtons.implicitHeight + 24

                RowLayout {
                    id: copyButtons
                    anchors.right: parent.right
                    anchors.rightMargin: 24
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8
                    MacCopyButton {
                        objectName: "copyTextButton"
                        action: copyTextAction
                        label: qsTr("COPY TEXT")
                        confirmed: session.textCopied
                    }
                    MacCopyButton {
                        objectName: "copyObsidianButton"
                        action: copyObsidianAction
                        label: qsTr("COPY OBS MD")
                        confirmed: session.obsidianCopied
                    }
                    MacCopyButton {
                        objectName: "copyAllButton"
                        action: copyMarkdownAction
                        label: qsTr("COPY MD")
                        confirmed: session.markdownCopied
                    }
                }
            }
        }
    }
}
