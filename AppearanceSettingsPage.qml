pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Kbextract

Item {
    id: appearancePage

    implicitHeight: contentLayout.implicitHeight + 64

    required property string currentThemeMode
    required property var availableThemeModes
    required property int currentTextScalePercent

    signal themeModeSelected(string mode)
    signal textScalePercentSelected(int percent)

    function themeModeLabel(mode) {
        return mode === "system"
            ? qsTr("System")
            : mode === "light"
                ? qsTr("Light")
                : mode === "dark"
                    ? qsTr("Dark")
                    : qsTr("Omarchy")
    }

    component TextSizeButton: Button {
        id: textSizeButton

        horizontalPadding: Theme.controlHorizontalPadding
        verticalPadding: Theme.controlVerticalPadding
        implicitHeight: Math.max(Theme.controlMinHeight,
            Math.ceil(contentItem.implicitHeight) + topPadding + bottomPadding)
        width: implicitWidth
        height: implicitHeight
        font.family: Theme.uiFont
        font.pointSize: Theme.fontPointSizeBody
        palette.buttonText: enabled ? Theme.text : Theme.textFaint

        contentItem: Text {
            text: textSizeButton.text
            color: textSizeButton.enabled ? Theme.text : Theme.textFaint
            font: textSizeButton.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: textSizeButton.down || textSizeButton.hovered
                ? Theme.surfaceHover : Theme.surface
            border.width: textSizeButton.activeFocus ? 2 : 1
            border.color: textSizeButton.activeFocus ? Theme.accent : Theme.lineStrong
        }
    }

    ColumnLayout {
        id: contentLayout

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 32
        spacing: 0

        Text {
            Layout.fillWidth: true
            text: qsTr("Appearance")
            color: Theme.text
            font.family: Theme.uiFont
            font.pointSize: Theme.fontPointSizeHeading
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
                    font.pointSize: Theme.fontPointSizeBody
                    font.weight: Font.DemiBold
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Choose the color theme used by kbextract.")
                    color: Theme.textMuted
                    font.family: Theme.uiFont
                    font.pointSize: Theme.fontPointSizeBody
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
                implicitHeight: Math.max(Theme.controlMinHeight,
                    Math.ceil(Math.max(contentItem.implicitHeight, indicator.implicitHeight))
                    + topPadding + bottomPadding)
                height: implicitHeight
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
                font.pointSize: Theme.fontPointSizeBody
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
                    font.pointSize: Theme.fontPointSizeBody

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
                    font.pointSize: Theme.fontPointSizeCaption
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

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 28
            spacing: 24

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Text size")
                    color: Theme.text
                    font.family: Theme.uiFont
                    font.pointSize: Theme.fontPointSizeBody
                    font.weight: Font.DemiBold
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Scale the system text size throughout kbextract.")
                    color: Theme.textMuted
                    font.family: Theme.uiFont
                    font.pointSize: Theme.fontPointSizeBody
                    wrapMode: Text.WordWrap
                }
            }

            RowLayout {
                spacing: 0
                Layout.alignment: Qt.AlignVCenter

                TextSizeButton {
                    objectName: "textSizeDecreaseButton"
                    text: qsTr("−")
                    enabled: appearancePage.currentTextScalePercent > 80
                    Accessible.name: qsTr("Decrease text size")
                    onClicked: appearancePage.textScalePercentSelected(
                        appearancePage.currentTextScalePercent - 10)
                }

                TextSizeButton {
                    objectName: "textSizeResetButton"
                    text: qsTr("%1%").arg(appearancePage.currentTextScalePercent)
                    Accessible.name: qsTr("Reset text size to system default")
                    Accessible.description: qsTr("Current text size is %1 percent")
                        .arg(appearancePage.currentTextScalePercent)
                    onClicked: appearancePage.textScalePercentSelected(100)
                }

                TextSizeButton {
                    objectName: "textSizeIncreaseButton"
                    text: qsTr("+")
                    enabled: appearancePage.currentTextScalePercent < 200
                    Accessible.name: qsTr("Increase text size")
                    onClicked: appearancePage.textScalePercentSelected(
                        appearancePage.currentTextScalePercent + 10)
                }
            }
        }
    }
}
