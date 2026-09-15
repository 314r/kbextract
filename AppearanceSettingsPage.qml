pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Kbextract

Item {
    id: appearancePage

    required property string currentThemeMode
    required property var availableThemeModes

    signal themeModeSelected(string mode)

    function themeModeLabel(mode) {
        return mode === "system"
            ? qsTr("System")
            : mode === "light"
                ? qsTr("Light")
                : mode === "dark"
                    ? qsTr("Dark")
                    : qsTr("Omarchy")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 0

        Text {
            Layout.fillWidth: true
            text: qsTr("Appearance")
            color: Theme.text
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeHeading
            font.weight: Font.DemiBold
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 32
            spacing: 24

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Theme")
                    color: Theme.text
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeBody
                    font.weight: Font.DemiBold
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Choose the color theme used by kbextract.")
                    color: Theme.textMuted
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeBody
                    wrapMode: Text.WordWrap
                }
            }

            ComboBox {
                id: themeSelector
                objectName: "themeSelector"

                Layout.preferredWidth: 180
                Layout.minimumWidth: 180
                Layout.maximumWidth: 180
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: implicitHeight
                implicitHeight: Math.max(Theme.controlMinHeight,
                    Math.ceil(Math.max(contentItem.implicitHeight, indicator.implicitHeight))
                    + topPadding + bottomPadding)
                leftPadding: Theme.controlHorizontalPadding
                rightPadding: Theme.controlHorizontalPadding + indicator.implicitWidth + spacing
                verticalPadding: Theme.controlVerticalPadding
                model: appearancePage.availableThemeModes
                currentIndex: appearancePage.availableThemeModes.indexOf(
                    appearancePage.currentThemeMode
                )
                displayText: currentIndex >= 0
                    ? appearancePage.themeModeLabel(appearancePage.availableThemeModes[currentIndex])
                    : ""
                font.family: Theme.uiFont
                font.pixelSize: Theme.fontSizeBody
                Accessible.name: qsTr("Theme")
                Accessible.description: qsTr("Choose the color theme used by kbextract.")

                delegate: ItemDelegate {
                    id: themeOption
                    objectName: "themeOption"

                    required property int index
                    required property var modelData

                    width: themeSelector.width
                    implicitHeight: Math.max(Theme.controlMinHeight,
                        Math.ceil(contentItem.implicitHeight) + topPadding + bottomPadding)
                    horizontalPadding: Theme.controlHorizontalPadding
                    verticalPadding: Theme.controlVerticalPadding
                    text: appearancePage.themeModeLabel(modelData)
                    highlighted: themeSelector.highlightedIndex === index
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeBody

                    contentItem: Text {
                        text: themeOption.text
                        color: Theme.text
                        font: themeOption.font
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    background: Rectangle {
                        color: themeOption.highlighted ? Theme.surfaceHover : Theme.surface
                    }
                }

                contentItem: Text {
                    text: themeSelector.displayText
                    color: Theme.text
                    font: themeSelector.font
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                indicator: Text {
                    x: themeSelector.width - width - Theme.controlHorizontalPadding
                    y: (themeSelector.height - height) / 2
                    text: qsTr("v")
                    color: Theme.textMuted
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.fontSizeCaption
                }

                background: Rectangle {
                    color: Theme.surface
                    border.width: themeSelector.activeFocus ? 2 : 1
                    border.color: themeSelector.activeFocus ? Theme.accent : Theme.line
                }

                popup: Popup {
                    y: themeSelector.height - 1
                    width: themeSelector.width
                    implicitHeight: Math.min(contentItem.implicitHeight + 2, 240)
                    padding: 1

                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: themeSelector.popup.visible ? themeSelector.delegateModel : null
                        currentIndex: themeSelector.highlightedIndex
                        ScrollIndicator.vertical: ScrollIndicator { }
                    }

                    background: Rectangle {
                        color: Theme.surface
                        border.width: 1
                        border.color: Theme.lineStrong
                    }
                }

                onActivated: function(index) {
                    appearancePage.themeModeSelected(appearancePage.availableThemeModes[index])
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
