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
    font.family: Theme.uiFont
    font.pointSize: Theme.fontPointSizeBody

    function applyOmarchyTheme() {
        Theme.omarchyDark = omarchyTheme.dark
        Theme.omarchyPalette = omarchyTheme.palette
        Theme.omarchyAvailable = omarchyTheme.available
    }

    function availableThemeModes() {
        const modes = ["system", "light", "dark"]
        if (omarchyTheme.available)
            modes.push("omarchy")
        return modes
    }

    function normalizedThemeMode(candidate) {
        return availableThemeModes().indexOf(candidate) >= 0 ? candidate : "system"
    }

    function setThemeMode(candidate) {
        const normalizedMode = normalizedThemeMode(candidate)
        Theme.mode = normalizedMode
        if (appearanceSettings.colorMode !== normalizedMode)
            appearanceSettings.colorMode = normalizedMode
    }

    function normalizedTextScalePercent(candidate) {
        const rounded = Math.round(Number(candidate) / 10) * 10
        return Math.max(80, Math.min(200, isFinite(rounded) ? rounded : 100))
    }

    function setTextScalePercent(candidate) {
        const normalized = normalizedTextScalePercent(candidate)
        Theme.textScalePercent = normalized
        if (appearanceSettings.textScalePercent !== normalized) {
            appearanceSettings.textScalePercent = normalized
            appearanceSettings.setValue("textScalePercent", normalized)
            appearanceSettings.sync()
        }
    }

    function adjustTextScalePercent(delta) {
        setTextScalePercent(Theme.effectiveTextScalePercent + delta)
    }

    function openSettings() {
        settingsWindow.show()
        settingsWindow.raise()
        settingsWindow.requestActivate()
    }

    function themeModeLabel() {
        if (Theme.mode === "light")
            return qsTr("LIGHT")
        if (Theme.mode === "dark")
            return qsTr("DARK")
        if (Theme.mode === "omarchy")
            return qsTr("OMARCHY")
        return qsTr("SYSTEM")
    }

    function themeModeIcon() {
        if (Theme.mode === "light")
            return "assets/icons/theme-light.svg"
        if (Theme.mode === "dark")
            return "assets/icons/theme-dark.svg"
        if (Theme.mode === "omarchy")
            return "assets/icons/theme-omarchy.svg"
        return "assets/icons/theme-system.svg"
    }

    component ToolButton: Button {
        id: control

        property color ink: control.enabled ? Theme.text : Theme.textFaint
        property color fill: control.down || control.hovered ? Theme.surfaceHover : Theme.surface
        property string alternateText: ""

        horizontalPadding: Theme.controlHorizontalPadding
        verticalPadding: Theme.controlVerticalPadding
        implicitWidth: Math.ceil(contentItem.implicitWidth) + leftPadding + rightPadding
        implicitHeight: Math.max(Theme.controlMinHeight,
            Math.ceil(contentItem.implicitHeight) + topPadding + bottomPadding)
        height: implicitHeight
        font.family: Theme.monoFont
        font.pointSize: Theme.fontPointSizeBody
        font.weight: Font.Medium

        contentItem: Item {
            implicitWidth: Math.max(label.implicitWidth, alternateLabel.implicitWidth)
            implicitHeight: Math.max(label.implicitHeight, alternateLabel.implicitHeight)

            Text {
                id: label

                anchors.fill: parent
                text: control.text
                color: control.ink
                font: control.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            Text {
                id: alternateLabel

                visible: false
                text: control.alternateText
                font: control.font
            }
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
        font.pointSize: Theme.fontPointSizeCaption
        font.weight: Font.DemiBold
        font.letterSpacing: 1.1
    }

    component SeparatorLine: Rectangle {
        color: Theme.line
    }

    AppSession {
        id: session
        adaptiveTypography: true
    }
    readonly property var koboLibrary: session.library

    OmarchyTheme {
        id: omarchyTheme
        active: Theme.mode === "omarchy" && available
        onPaletteChanged: window.applyOmarchyTheme()
        onAvailableChanged: {
            window.applyOmarchyTheme()
            if (!available && Theme.mode === "omarchy")
                window.setThemeMode("system")
        }
    }

    Settings {
        id: appearanceSettings
        category: "Appearance"
        property string colorMode: "system"
        property int textScalePercent: 100
    }

    SettingsWindow {
        id: settingsWindow

        transientParent: window
        currentThemeMode: Theme.mode
        availableThemeModes: window.availableThemeModes()
        currentTextScalePercent: Theme.effectiveTextScalePercent

        onThemeModeSelected: function(mode) {
            window.setThemeMode(mode)
        }
        onTextScalePercentSelected: function(percent) {
            window.setTextScalePercent(percent)
        }
    }

    Shortcut {
        sequences: [StandardKey.ZoomIn, "Ctrl+="]
        onActivated: window.adjustTextScalePercent(10)
    }

    Shortcut {
        sequences: [StandardKey.ZoomOut]
        onActivated: window.adjustTextScalePercent(-10)
    }

    Shortcut {
        sequence: "Ctrl+0"
        onActivated: window.setTextScalePercent(100)
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
        onAccepted: session.openDatabase(selectedFile)
    }

    Component.onCompleted: {
        applyOmarchyTheme()
        setThemeMode(appearanceSettings.colorMode)
        setTextScalePercent(appearanceSettings.textScalePercent)
    }

    onClosing: settingsWindow.close()

    header: Rectangle {
        implicitHeight: Math.max(48, modeButton.implicitHeight + 18)
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
                objectName: "modeButton"

                implicitWidth: 36
                implicitHeight: 30
                Layout.preferredWidth: implicitWidth
                Layout.preferredHeight: implicitHeight
                text: window.themeModeLabel()
                display: AbstractButton.IconOnly
                hoverEnabled: true
                icon.source: window.themeModeIcon()
                icon.width: 18
                icon.height: 18
                icon.color: Theme.text
                icon.cache: true
                Accessible.name: qsTr("Open settings. Current theme: %1").arg(text)

                ToolTip.visible: hovered
                ToolTip.delay: 500
                ToolTip.text: qsTr("Settings — Theme: %1").arg(text)
                ToolTip.toolTip.font.family: Theme.uiFont
                ToolTip.toolTip.font.pointSize: Theme.fontPointSizeBody

                background: Rectangle {
                    color: modeButton.down || modeButton.hovered ? Theme.surfaceHover : "transparent"
                    border.width: modeButton.activeFocus ? 2 : 1
                    border.color: modeButton.activeFocus ? Theme.accent : Theme.line
                }

                onClicked: window.openSettings()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: sourcePanel
            objectName: "sidebar"

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
                    objectName: "deviceSelector"

                    Layout.fillWidth: true
                    implicitHeight: Math.max(Theme.controlMinHeight,
                        Math.ceil(Math.max(contentItem.implicitHeight, indicator.implicitHeight))
                        + topPadding + bottomPadding)
                    height: implicitHeight
                    leftPadding: Theme.controlHorizontalPadding
                    rightPadding: Theme.controlHorizontalPadding + indicator.implicitWidth + spacing
                    verticalPadding: Theme.controlVerticalPadding
                    model: koboLibrary.devices
                    textRole: "displayName"
                    currentIndex: koboLibrary.currentDeviceIndex
                    enabled: count > 0
                    font.family: Theme.uiFont
                    font.pointSize: Theme.fontPointSizeBody

                    delegate: ItemDelegate {
                        required property int index
                        required property var modelData

                        width: deviceSelector.width
                        implicitHeight: Math.max(Theme.controlMinHeight,
                            Math.ceil(contentItem.implicitHeight) + topPadding + bottomPadding)
                        horizontalPadding: Theme.controlHorizontalPadding
                        verticalPadding: Theme.controlVerticalPadding
                        text: modelData.displayName
                        highlighted: deviceSelector.highlightedIndex === index
                        font.family: Theme.uiFont
                        font.pointSize: Theme.fontPointSizeBody

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
                        text: deviceSelector.displayText.length > 0 ? deviceSelector.displayText : qsTr("No Kobo device")
                        color: deviceSelector.enabled ? Theme.text : Theme.textFaint
                        font: deviceSelector.font
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    indicator: Text {
                        x: deviceSelector.width - width - Theme.controlHorizontalPadding
                        y: (deviceSelector.height - height) / 2
                        text: qsTr("v")
                        color: deviceSelector.enabled ? Theme.textMuted : Theme.textFaint
                        font.family: Theme.monoFont
                        font.pointSize: Theme.fontPointSizeCaption
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
                        session.selectDevice(index)
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
                        onClicked: session.refresh()
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
                        font.pointSize: Theme.fontPointSizeCaption
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
                        spacing: 0
                        model: koboLibrary.books
                        currentIndex: koboLibrary.currentBookIndex

                        delegate: Button {
                            id: bookRow

                            required property int index
                            required property var modelData
                            readonly property bool selected: bookList.currentIndex === index
                            readonly property bool outlined: selected || down
                            readonly property int separatorThickness: activeFocus ? 2 : 1

                            width: bookList.width
                            z: outlined ? 1 : 0
                            height: Math.max(
                                72,
                                contentItem.implicitHeight + topPadding * 2 + 2
                            )
                            topPadding: 8
                            bottomPadding: outlined ? topPadding : topPadding + separatorThickness
                            hoverEnabled: true

                            contentItem: Item {
                                implicitHeight: bookRowText.implicitHeight

                                Column {
                                    id: bookRowText

                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 4

                                    Text {
                                        width: parent.width
                                        text: bookRow.modelData.title
                                        color: Theme.text
                                        font.family: Theme.uiFont
                                        font.pointSize: Theme.fontPointSizeBody
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        width: parent.width
                                        visible: text.length > 0
                                        text: bookRow.modelData.author
                                        color: Theme.textMuted
                                        font.family: Theme.uiFont
                                        font.pointSize: Theme.fontPointSizeBody
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
                                        font.pointSize: Theme.fontPointSizeCaption
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            background: Rectangle {
                                x: 0
                                y: bookRow.outlined ? -1 : 0
                                width: bookRow.width
                                height: bookRow.height + (bookRow.outlined ? 1 : 0)
                                color: bookRow.outlined
                                    ? Theme.surfaceSelected
                                    : (bookRow.hovered ? Theme.surfaceHover : "transparent")
                                border.width: bookRow.outlined ? 2 : 0
                                border.color: Theme.accent

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    visible: !bookRow.outlined
                                    height: bookRow.separatorThickness
                                    color: bookRow.activeFocus ? Theme.accent : Theme.line
                                }
                            }

                            onClicked: session.selectBook(index)
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
                        font.pointSize: Theme.fontPointSizeBody
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        Rectangle {
            id: mainColumn
            objectName: "mainColumn"
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
                            font.pointSize: Theme.fontPointSizeHeading
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            visible: text.length > 0
                            text: koboLibrary.currentBookAuthor
                            color: Theme.textMuted
                            font.family: Theme.uiFont
                            font.pointSize: Theme.fontPointSizeBody
                            elide: Text.ElideRight
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ScrollView {
                        id: annotationScroll

                        function resetToTop() {
                            annotationText.deselect()
                            annotationText.cursorPosition = 0

                            const flickable = annotationScroll.contentItem
                            flickable.cancelFlick()
                            flickable.contentY = flickable.originY
                            flickable.returnToBounds()
                        }

                        anchors.fill: parent
                        visible: koboLibrary.currentBookIndex >= 0
                            && koboLibrary.currentBookMarkdown.length > 0
                        clip: true
                        contentWidth: availableWidth
                        contentHeight: annotationText.height

                        Connections {
                            target: koboLibrary

                            function onCurrentBookChanged() {
                                annotationScroll.resetToTop()
                                Qt.callLater(annotationScroll.resetToTop)
                            }
                        }

                        AnnotationBody {
                            id: annotationText
                            width: annotationScroll.availableWidth
                            markdown: koboLibrary.currentBookMarkdown
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
                        font.pointSize: Theme.fontPointSizeBody
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
                        font.pointSize: Theme.fontPointSizeBody
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
                Rectangle {
                    id: copyFooter
                    objectName: "copyFooter"
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(48,
                        copyTextButton.height + 18,
                        copyObsidianButton.height + 18,
                        copyAllButton.height + 18)
                    Layout.minimumHeight: Layout.preferredHeight
                    color: Theme.panel

                    SeparatorLine {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 1
                    }

                    Item {
                        id: copyButtons
                        width: copyTextButton.width + copyObsidianButton.width
                            + copyAllButton.width + spacing * 2
                        height: Math.max(copyTextButton.height,
                            copyObsidianButton.height, copyAllButton.height)
                        readonly property real spacing: 8
                        anchors.right: parent.right
                        anchors.rightMargin: 24
                        anchors.verticalCenter: parent.verticalCenter

                        ToolButton {
                            id: copyTextButton
                            objectName: "copyTextButton"

                            anchors.right: copyObsidianButton.left
                            anchors.rightMargin: copyButtons.spacing
                            anchors.verticalCenter: parent.verticalCenter

                            property bool copyConfirmed: session.textCopied

                            text: copyConfirmed ? qsTr("COPIED") : qsTr("COPY TEXT")
                            alternateText: copyConfirmed ? qsTr("COPY TEXT") : qsTr("COPIED")
                            enabled: koboLibrary.currentBookPlainText.length > 0

                            onClicked: session.copy("text")
                        }

                        ToolButton {
                            id: copyObsidianButton
                            objectName: "copyObsidianButton"

                            anchors.right: copyAllButton.left
                            anchors.rightMargin: copyButtons.spacing
                            anchors.verticalCenter: parent.verticalCenter

                            property bool copyConfirmed: session.obsidianCopied

                            text: copyConfirmed ? qsTr("COPIED") : qsTr("COPY OBS MD")
                            alternateText: copyConfirmed ? qsTr("COPY OBS MD") : qsTr("COPIED")
                            enabled: koboLibrary.currentBookObsidianMarkdown.length > 0

                            onClicked: session.copy("obsidian")
                        }

                        ToolButton {
                            id: copyAllButton
                            objectName: "copyAllButton"

                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter

                            property bool copyConfirmed: session.markdownCopied

                            text: copyConfirmed ? qsTr("COPIED") : qsTr("COPY MD")
                            alternateText: copyConfirmed ? qsTr("COPY MD") : qsTr("COPIED")
                            enabled: koboLibrary.currentBookMarkdown.length > 0

                            onClicked: session.copy("markdown")
                        }
                    }
                }
            }
        }
    }
}
