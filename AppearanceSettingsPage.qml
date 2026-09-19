pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

import Kbextract

Item {
    id: appearancePage
    implicitHeight: contentLayout.implicitHeight + 6 * Kirigami.Units.largeSpacing

    required property string currentThemeMode
    required property var availableThemeModes
    required property int currentTextScalePercent

    signal themeModeSelected(string mode)
    signal textScalePercentSelected(int percent)

    function themeModeLabel(mode) {
        return mode === "system" ? qsTr("System")
            : mode === "light" ? qsTr("Light")
            : mode === "dark" ? qsTr("Dark") : qsTr("Omarchy")
    }

    ColumnLayout {
        id: contentLayout
        objectName: "appearanceForm"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 3 * Kirigami.Units.largeSpacing
        anchors.leftMargin: 3 * Kirigami.Units.largeSpacing
        anchors.rightMargin: 3 * Kirigami.Units.largeSpacing
        spacing: 4 * Kirigami.Units.largeSpacing

        // Keep both groups on the page's leading edge. FormLayout centers its
        // internal grid independently, so it is unsuitable for these stacked rows.
        ColumnLayout {
            id: themeSection
            objectName: "themeSection"
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                objectName: "themeFieldLabel"
                text: qsTr("Theme")
                level: 3
                type: Kirigami.Heading.Primary
                font.family: Theme.uiFont
                font.pointSize: Theme.fontPointSizeBody * 1.15
            }

            ComboBox {
                id: themeSelector
                objectName: "themeSelector"
                Layout.fillWidth: true
                implicitContentWidthPolicy: ComboBox.WidestText
                model: appearancePage.availableThemeModes.map(function(mode) {
                    return { value: mode, label: appearancePage.themeModeLabel(mode) }
                })
                textRole: "label"
                valueRole: "value"
                currentIndex: appearancePage.availableThemeModes.indexOf(appearancePage.currentThemeMode)
                Accessible.name: qsTr("Theme")
                Accessible.description: qsTr("Choose the color theme used by kbextract.")

                delegate: ItemDelegate {
                    id: themeOption
                    objectName: "themeOption"
                    required property int index
                    required property var modelData
                    width: themeSelector.width
                    text: modelData.label
                    highlighted: themeSelector.highlightedIndex === index
                    font: themeSelector.font
                }

                onActivated: function(index) {
                    appearancePage.themeModeSelected(appearancePage.availableThemeModes[index])
                }
            }

            Label {
                objectName: "themeDescription"
                Layout.fillWidth: true
                text: qsTr("Choose the color theme used by kbextract.")
                color: Theme.textFaint
                wrapMode: Text.WordWrap
            }
        }

        ColumnLayout {
            id: textSizeSection
            objectName: "textSizeSection"
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                objectName: "textSizeFieldLabel"
                text: qsTr("Text size")
                level: 3
                type: Kirigami.Heading.Primary
                font.family: Theme.uiFont
                font.pointSize: Theme.fontPointSizeBody * 1.15
            }

            RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Button {
                    objectName: "textSizeDecreaseButton"
                    text: qsTr("−")
                    implicitWidth: Math.max(implicitHeight, contentItem.implicitWidth + leftPadding + rightPadding)
                    enabled: appearancePage.currentTextScalePercent > 80
                    Accessible.name: qsTr("Decrease text size")
                    onClicked: appearancePage.textScalePercentSelected(appearancePage.currentTextScalePercent - 10)
                }
                Button {
                    objectName: "textSizeResetButton"
                    text: qsTr("%1%").arg(appearancePage.currentTextScalePercent)
                    Accessible.name: qsTr("Reset text size to system default")
                    Accessible.description: qsTr("Current text size is %1 percent").arg(appearancePage.currentTextScalePercent)
                    onClicked: appearancePage.textScalePercentSelected(100)
                }
                Button {
                    id: increaseButton
                    objectName: "textSizeIncreaseButton"
                    text: qsTr("+")
                    implicitWidth: Math.max(implicitHeight, contentItem.implicitWidth + leftPadding + rightPadding)
                    enabled: appearancePage.currentTextScalePercent < 200
                    Accessible.name: qsTr("Increase text size")
                    onClicked: appearancePage.textScalePercentSelected(appearancePage.currentTextScalePercent + 10)
                }
            }

            Label {
                objectName: "textSizeDescription"
                Layout.fillWidth: true
                text: qsTr("Scale the system text size throughout kbextract.")
                color: Theme.textFaint
                wrapMode: Text.WordWrap
            }
        }
    }
}
