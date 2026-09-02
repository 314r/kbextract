import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import QtCore

import Kbextract

ApplicationWindow {
    id: window

    visible: true
    width: 1360
    height: 900
    minimumWidth: 1040
    minimumHeight: 680
    title: qsTr("kbextract")
    color: Theme.canvas

    function applyOmarchyTheme() {
        Theme.omarchyDark = omarchyTheme.dark
        Theme.omarchyPalette = omarchyTheme.palette
        Theme.omarchyFontSizes = omarchyTheme.fontSizes
    }

    component ToolButton: Button {
        id: control

        property color ink: control.enabled ? Theme.text : Theme.textFaint
        property color fill: control.down || control.hovered ? Theme.surfaceHover : Theme.surface

        implicitHeight: Math.max(30, contentItem.implicitHeight + topPadding + bottomPadding)
        font.family: Theme.monoFont
        font.pixelSize: Theme.fontSizeBody
        font.weight: Font.Medium

        contentItem: Text {
            text: control.text
            color: control.ink
            font: control.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            color: control.fill
            border.width: control.activeFocus ? 2 : 1
            border.color: control.activeFocus ? Theme.accent : Theme.line
        }
    }

    component SectionLabel: Text {
        color: Theme.text
        font.family: Theme.monoFont
        font.pixelSize: Theme.fontSizeCaption
        font.weight: Font.DemiBold
        font.letterSpacing: 1.1
    }

    component SeparatorLine: Rectangle {
        color: Theme.line
    }

    KoboLibrary {
        id: koboLibrary
    }

    OmarchyTheme {
        id: omarchyTheme
        onPaletteChanged: window.applyOmarchyTheme()
        onFontSizesChanged: window.applyOmarchyTheme()
    }

    Settings {
        id: appearanceSettings
        category: "Appearance"
        property string colorMode: "omarchy"
    }

    FileDialog {
        id: databaseDialog
        title: qsTr("Choose KoboReader.sqlite")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            qsTr("Kobo database (KoboReader.sqlite)"),
            qsTr("SQLite databases (*.sqlite *.db)"),
            qsTr("All files (*)")
        ]
        onAccepted: koboLibrary.addDatabase(selectedFile.toLocalFile())
    }

    Component.onCompleted: {
        Theme.mode = appearanceSettings.colorMode
        applyOmarchyTheme()
        koboLibrary.refreshDevices()
    }

    header: Rectangle {
        implicitHeight: Math.max(48, modeButton.implicitHeight + 20)
        color: Theme.canvasGlass

        SeparatorLine {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            spacing: 6

            Item { Layout.fillWidth: true }

            Button {
                id: modeButton

                Layout.preferredWidth: 88
                Layout.preferredHeight: Math.max(28, implicitHeight)
                text: Theme.mode === "omarchy" ? qsTr("OMARCHY") : (Theme.mode === "light" ? qsTr("LIGHT") : qsTr("DARK"))
                font.family: Theme.monoFont
                font.pixelSize: Theme.fontSizeBody
                font.weight: Font.Medium

                contentItem: Text {
                    text: modeButton.text
                    color: Theme.text
                    font: modeButton.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: modeButton.down || modeButton.hovered ? Theme.surfaceHover : "transparent"
                    border.width: modeButton.activeFocus ? 2 : 1
                    border.color: modeButton.activeFocus ? Theme.accent : Theme.line
                }

                onClicked: {
                    Theme.mode = Theme.mode === "omarchy" ? "light" : (Theme.mode === "light" ? "dark" : "omarchy")
                    appearanceSettings.colorMode = Theme.mode
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: sourcePanel

            Layout.preferredWidth: window.width >= 1220 ? 290 : 246
            Layout.minimumWidth: 246
            Layout.fillHeight: true
            color: Theme.panel

            SeparatorLine {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                width: 1
            }

            ColumnLayout {
                id: sourceLayout

                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                SectionLabel {
                    text: qsTr("DEVICE")
                }

                ComboBox {
                    id: deviceSelector

                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(34, implicitHeight)
                    model: koboLibrary.devices
                    textRole: "displayName"
                    currentIndex: koboLibrary.currentDeviceIndex
                    enabled: count > 0
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeBody

                    delegate: ItemDelegate {
                        required property int index
                        required property var modelData

                        width: deviceSelector.width
                        height: Math.max(34, implicitHeight)
                        text: modelData.displayName
                        highlighted: deviceSelector.highlightedIndex === index
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.fontSizeBody

                        contentItem: Text {
                            text: parent.text
                            color: Theme.text
                            font: parent.font
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        background: Rectangle {
                            color: parent.highlighted ? Theme.surfaceHover : Theme.surface
                        }
                    }

                    contentItem: Text {
                        leftPadding: 10
                        rightPadding: 28
                        text: deviceSelector.displayText.length > 0 ? deviceSelector.displayText : qsTr("No Kobo device")
                        color: deviceSelector.enabled ? Theme.text : Theme.textFaint
                        font: deviceSelector.font
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    indicator: Text {
                        x: deviceSelector.width - width - 10
                        y: (deviceSelector.height - height) / 2
                        text: qsTr("v")
                        color: deviceSelector.enabled ? Theme.textMuted : Theme.textFaint
                        font.family: Theme.monoFont
                        font.pixelSize: Theme.fontSizeCaption
                    }

                    background: Rectangle {
                        color: Theme.surface
                        border.width: deviceSelector.activeFocus ? 2 : 1
                        border.color: deviceSelector.activeFocus ? Theme.accent : Theme.line
                    }

                    popup: Popup {
                        y: deviceSelector.height - 1
                        width: deviceSelector.width
                        implicitHeight: Math.min(contentItem.implicitHeight + 2, 240)
                        padding: 1

                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: deviceSelector.popup.visible ? deviceSelector.delegateModel : null
                            currentIndex: deviceSelector.highlightedIndex
                            ScrollIndicator.vertical: ScrollIndicator { }
                        }

                        background: Rectangle {
                            color: Theme.surface
                            border.width: 1
                            border.color: Theme.lineStrong
                        }
                    }

                    onActivated: function(index) {
                        koboLibrary.currentDeviceIndex = index
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ToolButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        text: qsTr("REFRESH")
                        fill: "transparent"
                        onClicked: koboLibrary.refreshDevices()
                    }

                    ToolButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        text: qsTr("BROWSE...")
                        fill: "transparent"
                        onClicked: databaseDialog.open()
                    }
                }

                SeparatorLine {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                }

                RowLayout {
                    Layout.fillWidth: true

                    SectionLabel {
                        text: qsTr("BOOKS")
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: koboLibrary.books.length
                        color: Theme.textFaint
                        font.family: Theme.monoFont
                        font.pixelSize: Theme.fontSizeCaption
                    }
                }

                Item {
                    id: bookListArea

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: bookList

                        anchors.fill: parent
                        clip: true
                        spacing: 6
                        model: koboLibrary.books
                        currentIndex: koboLibrary.currentBookIndex

                        delegate: Button {
                            id: bookRow

                            required property int index
                            required property var modelData

                            width: bookList.width
                            height: Math.max(
                                72,
                                contentItem.implicitHeight + topPadding + bottomPadding
                            )
                            hoverEnabled: true

                            contentItem: Column {
                                spacing: 4

                                Text {
                                    width: parent.width
                                    text: bookRow.modelData.title
                                    color: Theme.text
                                    font.family: Theme.uiFont
                                    font.pixelSize: Theme.fontSizeBody
                                    font.weight: Font.Medium
                                    elide: Text.ElideRight
                                }

                                Text {
                                    width: parent.width
                                    visible: text.length > 0
                                    text: bookRow.modelData.author
                                    color: Theme.textMuted
                                    font.family: Theme.uiFont
                                    font.pixelSize: Theme.fontSizeCaption
                                    elide: Text.ElideRight
                                }

                                Text {
                                    width: parent.width
                                    text: qsTr("%1 highlight%2  ·  %3 note%4")
                                        .arg(bookRow.modelData.highlightCount)
                                        .arg(bookRow.modelData.highlightCount === 1 ? "" : "s")
                                        .arg(bookRow.modelData.noteCount)
                                        .arg(bookRow.modelData.noteCount === 1 ? "" : "s")
                                    color: Theme.textFaint
                                    font.family: Theme.monoFont
                                    font.pixelSize: Theme.fontSizeCaption
                                    elide: Text.ElideRight
                                }
                            }

                            background: Rectangle {
                                color: bookList.currentIndex === bookRow.index
                                    ? Theme.surfaceSelected
                                    : (bookRow.hovered ? Theme.surfaceHover : Theme.surface)
                                border.width: bookList.currentIndex === bookRow.index || bookRow.activeFocus ? 2 : 1
                                border.color: bookList.currentIndex === bookRow.index || bookRow.activeFocus ? Theme.accent : Theme.line
                            }

                            onClicked: koboLibrary.currentBookIndex = index
                        }

                        ScrollBar.vertical: ScrollBar {
                            id: bookListVerticalScrollBar

                            parent: sourcePanel
                            x: sourcePanel.width - width
                            y: sourceLayout.y + bookListArea.y
                            height: bookListArea.height
                            z: 2
                            policy: ScrollBar.AsNeeded
                            padding: 2

                            contentItem: Rectangle {
                                implicitWidth: 6
                                implicitHeight: 6
                                radius: width / 2
                                color: bookListVerticalScrollBar.pressed
                                    ? Theme.accent
                                    : bookListVerticalScrollBar.hovered
                                        ? Theme.textMuted
                                        : Theme.textFaint
                                opacity: bookListVerticalScrollBar.size < 1.0 ? 0.8 : 0.0
                            }

                            background: Rectangle {
                                color: "transparent"
                            }
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 12
                        visible: bookList.count === 0
                        text: koboLibrary.statusText
                        color: Theme.textFaint
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.fontSizeBody
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.canvas

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    id: bookHeader

                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(76, bookHeaderContent.implicitHeight + 32)
                    visible: koboLibrary.currentBookIndex >= 0
                    color: Theme.panel

                    SeparatorLine {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                    }

                    Column {
                        id: bookHeaderContent

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 24
                        anchors.rightMargin: 24
                        spacing: 5

                        Text {
                            width: parent.width
                            text: koboLibrary.currentBookTitle
                            color: Theme.text
                            font.family: Theme.uiFont
                            font.pixelSize: Theme.fontSizeHeading
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            visible: text.length > 0
                            text: koboLibrary.currentBookAuthor
                            color: Theme.textMuted
                            font.family: Theme.uiFont
                            font.pixelSize: Theme.fontSizeBody
                            elide: Text.ElideRight
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ScrollView {
                        id: annotationScroll

                        anchors.fill: parent
                        visible: koboLibrary.currentBookIndex >= 0
                            && koboLibrary.currentBookMarkdown.length > 0
                        clip: true
                        contentWidth: availableWidth
                        contentHeight: annotationText.height

                        TextArea {
                            id: annotationText

                            readonly property int wrapColumn: 120
                            readonly property real minimumHorizontalPadding: 32
                            readonly property real wrapWidth: annotationFontMetrics.advanceWidth("M".repeat(wrapColumn))
                            readonly property real responsiveHorizontalPadding: Math.max(
                                minimumHorizontalPadding,
                                (width - wrapWidth) / 2
                            )

                            width: annotationScroll.availableWidth
                            height: implicitHeight
                            text: koboLibrary.currentBookMarkdown
                            textFormat: TextEdit.PlainText
                            readOnly: true
                            selectByMouse: true
                            persistentSelection: true
                            wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
                            color: Theme.text
                            selectedTextColor: Theme.accentText
                            selectionColor: Theme.accent
                            font.family: Theme.monoFont
                            font.pixelSize: 15
                            leftPadding: responsiveHorizontalPadding
                            rightPadding: responsiveHorizontalPadding
                            topPadding: 28
                            bottomPadding: 28

                            FontMetrics {
                                id: annotationFontMetrics
                                font: annotationText.font
                            }

                            MarkdownHighlighter {
                                textDocument: annotationText.textDocument
                                headingPixelSize: 20
                            }

                            background: Rectangle {
                                color: "transparent"
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            id: annotationVerticalScrollBar

                            parent: annotationScroll
                            x: annotationScroll.mirrored ? 0 : annotationScroll.width - width
                            y: annotationScroll.topPadding
                            height: annotationScroll.availableHeight
                            z: 1
                            policy: ScrollBar.AsNeeded
                            padding: 2

                            contentItem: Rectangle {
                                implicitWidth: 6
                                implicitHeight: 6
                                radius: width / 2
                                color: annotationVerticalScrollBar.pressed
                                    ? Theme.accent
                                    : annotationVerticalScrollBar.hovered
                                        ? Theme.textMuted
                                        : Theme.textFaint
                                opacity: annotationVerticalScrollBar.size < 1.0 ? 0.8 : 0.0
                            }

                            background: Rectangle {
                                color: "transparent"
                            }
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 48, 480)
                        visible: koboLibrary.currentBookIndex < 0
                        text: qsTr("Select a book to view its highlights and notes.")
                        color: Theme.textFaint
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.fontSizeBody
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 48, 480)
                        visible: koboLibrary.currentBookIndex >= 0
                            && koboLibrary.currentBookMarkdown.length === 0
                        text: koboLibrary.annotationStatusText
                        color: Theme.textFaint
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.fontSizeBody
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }

    footer: Rectangle {
        implicitHeight: 48
        color: Theme.panel

        SeparatorLine {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
        }
    }
}
