import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Kbextract

ApplicationWindow {
    id: settingsWindow
    objectName: "settingsWindow"

    property string currentThemeMode: "system"
    property var availableThemeModes: ["system", "light", "dark"]
    property int currentTextScalePercent: 100
    property int currentSectionIndex: 0

    signal themeModeSelected(string mode)
    signal textScalePercentSelected(int percent)

    visible: false
    width: 760
    height: 520
    minimumWidth: 600
    minimumHeight: 400
    modality: Qt.NonModal
    title: qsTr("Settings")
    color: Theme.canvas
    font.family: Theme.uiFont
    font.pointSize: Theme.fontPointSizeBody

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 180
            Layout.fillHeight: true
            color: Theme.panel

            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                width: 1
                color: Theme.line
            }

            Button {
                id: appearanceButton
                objectName: "appearanceButton"

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 16
                implicitHeight: Math.max(Theme.controlMinHeight,
                    Math.ceil(contentItem.implicitHeight) + topPadding + bottomPadding)
                horizontalPadding: Theme.controlHorizontalPadding
                verticalPadding: Theme.controlVerticalPadding
                text: qsTr("Appearance")
                hoverEnabled: true
                font.family: Theme.uiFont
                font.pointSize: Theme.fontPointSizeBody
                font.weight: Font.DemiBold
                palette.buttonText: Theme.text
                Accessible.name: text
                Accessible.description: qsTr("Appearance settings section")

                background: Rectangle {
                    color: settingsWindow.currentSectionIndex === 0
                        ? Theme.surfaceSelected
                        : (appearanceButton.hovered ? Theme.surfaceHover : "transparent")
                    border.width: appearanceButton.activeFocus ? 2 : 1
                    border.color: appearanceButton.activeFocus
                        ? Theme.accent
                        : (settingsWindow.currentSectionIndex === 0 ? Theme.accent : "transparent")
                }

                onClicked: settingsWindow.currentSectionIndex = 0
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: settingsWindow.currentSectionIndex

            ScrollView {
                id: appearanceScroll

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth

                AppearanceSettingsPage {
                    width: appearanceScroll.availableWidth
                    height: Math.max(implicitHeight, appearanceScroll.availableHeight)
                    currentThemeMode: settingsWindow.currentThemeMode
                    availableThemeModes: settingsWindow.availableThemeModes
                    currentTextScalePercent: settingsWindow.currentTextScalePercent

                    onThemeModeSelected: function(mode) {
                        settingsWindow.themeModeSelected(mode)
                    }
                    onTextScalePercentSelected: function(percent) {
                        settingsWindow.textScalePercentSelected(percent)
                    }
                }
            }
        }
    }
}
