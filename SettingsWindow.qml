pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Fusion
import org.kde.kirigami as Kirigami

import Kbextract

Kirigami.AbstractApplicationWindow {
    id: settingsWindow
    objectName: "settingsWindow"

    property string currentThemeMode: "system"
    property var availableThemeModes: ["system", "light", "dark"]
    property int currentTextScalePercent: 100

    property int currentSectionIndex: 0
    readonly property var sections: [
        { key: "appearance", title: qsTr("Appearance"), icon: "assets/icons/settings-appearance.svg" },
        { key: "export", title: qsTr("Export"), icon: "assets/icons/settings-export.svg" }
    ]

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

    palette.window: Theme.canvas
    palette.windowText: Theme.text
    palette.base: Theme.surface
    palette.alternateBase: Theme.panel
    palette.text: Theme.text
    palette.button: Theme.surface
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText
    palette.brightText: Theme.accentText
    palette.accent: Theme.accent
    palette.mid: Theme.line
    palette.dark: Theme.lineStrong
    palette.light: Theme.surfaceHover
    palette.link: Theme.accent
    palette.placeholderText: Theme.textMuted
    palette.toolTipBase: Theme.surface
    palette.toolTipText: Theme.text
    palette.disabled.text: Theme.textFaint
    palette.disabled.buttonText: Theme.textFaint
    palette.disabled.windowText: Theme.textFaint

    Item {
        id: themeRoot
        // Keep Kirigami and standard Qt controls on the same live app palette.
        Kirigami.Theme.inherit: false
        Kirigami.Theme.backgroundColor: Theme.canvas
        Kirigami.Theme.alternateBackgroundColor: Theme.panel
        Kirigami.Theme.textColor: Theme.text
        Kirigami.Theme.disabledTextColor: Theme.textFaint
        Kirigami.Theme.highlightColor: Theme.accent
        Kirigami.Theme.highlightedTextColor: Theme.accentText
        Kirigami.Theme.linkColor: Theme.accent
        Kirigami.Theme.positiveTextColor: Theme.mint
        Kirigami.Theme.neutralTextColor: Theme.warning
        Kirigami.Theme.negativeTextColor: Theme.danger

        anchors.fill: parent
        FontMetrics {
            id: headingMetrics
            font.family: Theme.uiFont
            font.pointSize: Theme.fontPointSizeHeading
        }
        Kirigami.GlobalDrawer {
            id: navigationDrawer
            objectName: "settingsSidebar"
            parent: themeRoot
            modal: false
            drawerOpen: true
            closePolicy: Popup.NoAutoClose
            interactive: false
            handleVisible: false
            collapsible: false
            width: {
                let result = 11 * Kirigami.Units.gridUnit
                for (let index = 0; index < navigationItems.count; ++index) {
                    const item = navigationItems.itemAt(index)
                    if (item)
                        result = Math.max(result, item.implicitWidth
                            + 2 * Kirigami.Units.largeSpacing
                            + navigationScroll.leftPadding + navigationScroll.rightPadding)
                }
                return Math.ceil(result)
            }
            height: parent.height
            padding: 0
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0
            font: settingsWindow.font
            palette: settingsWindow.palette
            Kirigami.Theme.inherit: true

            background: Rectangle {
                color: Theme.panel
                Kirigami.Separator {
                    anchors.right: navigationDrawer.edge === Qt.LeftEdge ? parent.right : undefined
                    anchors.left: navigationDrawer.edge === Qt.RightEdge ? parent.left : undefined
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    color: Theme.line
                }
            }

            contentItem: ScrollView {
                id: navigationScroll
                contentWidth: availableWidth
                clip: true
                ColumnLayout {
                    width: navigationScroll.availableWidth
                    spacing: Kirigami.Units.smallSpacing

                    Label {
                        text: qsTr("Settings")
                        font.capitalization: Font.AllUppercase
                        font.weight: Font.DemiBold
                        color: Theme.textMuted
                        Layout.fillWidth: true
                        Layout.preferredHeight: headingMetrics.height
                        Layout.topMargin: 2 * Kirigami.Units.largeSpacing
                        Layout.bottomMargin: 2 * Kirigami.Units.largeSpacing - Kirigami.Units.smallSpacing
                        Layout.leftMargin: 2 * Kirigami.Units.largeSpacing
                        verticalAlignment: Text.AlignVCenter
                    }

                    Repeater {
                        id: navigationItems
                        model: settingsWindow.sections
                        delegate: ItemDelegate {
                            id: navigationItem
                            required property int index
                            required property var modelData
                            objectName: modelData.key + "Button"
                            Layout.fillWidth: true
                            Layout.leftMargin: Kirigami.Units.largeSpacing
                            Layout.rightMargin: Kirigami.Units.largeSpacing
                            text: modelData.title
                            icon.source: modelData.icon
                            icon.width: Kirigami.Units.iconSizes.smallMedium
                            icon.height: Kirigami.Units.iconSizes.smallMedium
                            icon.color: Theme.text
                            spacing: Kirigami.Units.largeSpacing
                            horizontalPadding: Kirigami.Units.largeSpacing
                            verticalPadding: Kirigami.Units.smallSpacing * 2
                            font: settingsWindow.font
                            highlighted: settingsWindow.currentSectionIndex === index
                            palette.highlight: Theme.surfaceSelected
                            palette.highlightedText: Theme.text
                            palette.text: Theme.text
                            Accessible.role: Accessible.PageTab
                            Accessible.selected: highlighted
                            focusPolicy: Qt.StrongFocus
                            hoverEnabled: true
                            background: Rectangle {
                                radius: Kirigami.Units.cornerRadius
                                color: navigationItem.down || navigationItem.hovered
                                    ? Theme.surfaceHover
                                    : navigationItem.highlighted ? Theme.surfaceSelected : "transparent"
                                border.width: navigationItem.visualFocus ? 2 : 0
                                border.color: Theme.accent
                            }
                            onClicked: settingsWindow.currentSectionIndex = index
                            Keys.onDownPressed: navigationItems.itemAt((index + 1) % navigationItems.count).forceActiveFocus(Qt.TabFocusReason)
                            Keys.onUpPressed: navigationItems.itemAt((index + navigationItems.count - 1) % navigationItems.count).forceActiveFocus(Qt.TabFocusReason)
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: navigationDrawer.edge === Qt.LeftEdge ? navigationDrawer.width : 0
            anchors.rightMargin: navigationDrawer.edge === Qt.RightEdge ? navigationDrawer.width : 0
            spacing: 0

            Rectangle {
                objectName: "settingsHeader"
                Layout.fillWidth: true
                implicitHeight: headerRow.implicitHeight + 2 * Kirigami.Units.largeSpacing
                color: Theme.panel

                RowLayout {
                    id: headerRow
                    anchors.fill: parent
                    anchors.leftMargin: 3 * Kirigami.Units.largeSpacing
                    anchors.rightMargin: Kirigami.Units.smallSpacing
                    anchors.topMargin: Kirigami.Units.largeSpacing
                    anchors.bottomMargin: Kirigami.Units.largeSpacing
                    spacing: Kirigami.Units.largeSpacing

                    Label {
                        objectName: "settingsSectionTitle"
                        Layout.fillWidth: true
                        text: settingsWindow.sections[settingsWindow.currentSectionIndex].title
                        font.family: Theme.uiFont
                        font.pointSize: Theme.fontPointSizeHeading
                        font.weight: Font.DemiBold
                        color: Theme.text
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    ToolButton {
                        id: closeButton
                        objectName: "settingsCloseButton"
                        text: qsTr("Close settings")
                        display: AbstractButton.IconOnly
                        icon.source: "assets/icons/close.svg"
                        icon.width: Theme.controlIconSize
                        icon.height: Theme.controlIconSize
                        icon.color: Theme.text
                        implicitWidth: Math.max(Theme.controlMinHeight, implicitContentWidth + leftPadding + rightPadding)
                        implicitHeight: Math.max(Theme.controlMinHeight, implicitContentHeight + topPadding + bottomPadding)
                        focusPolicy: Qt.StrongFocus
                        hoverEnabled: true
                        Accessible.name: text
                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        ToolTip.text: text
                        ToolTip.toolTip.font.family: Theme.uiFont
                        ToolTip.toolTip.font.pointSize: Theme.fontPointSizeBody
                        background: Rectangle {
                            radius: Kirigami.Units.cornerRadius
                            color: closeButton.down || closeButton.hovered ? Theme.surfaceHover : "transparent"
                            border.width: closeButton.visualFocus ? 2 : 0
                            border.color: Theme.accent
                        }
                        onClicked: settingsWindow.close()
                    }
                }

                Kirigami.Separator {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    color: Theme.line
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: settingsWindow.currentSectionIndex

                ScrollView {
                    id: appearanceScroll
                    clip: true
                    contentWidth: availableWidth

                    AppearanceSettingsPage {
                        width: appearanceScroll.availableWidth
                        height: Math.max(implicitHeight, appearanceScroll.availableHeight)
                        currentThemeMode: settingsWindow.currentThemeMode
                        availableThemeModes: settingsWindow.availableThemeModes
                        currentTextScalePercent: settingsWindow.currentTextScalePercent
                        onThemeModeSelected: function(mode) { settingsWindow.themeModeSelected(mode) }
                        onTextScalePercentSelected: function(percent) { settingsWindow.textScalePercentSelected(percent) }
                    }
                }

                Item {
                    objectName: "exportPage"
                    Kirigami.PlaceholderMessage {
                        objectName: "exportPlaceholder"
                        anchors.centerIn: parent
                        width: parent.width - 4 * Kirigami.Units.largeSpacing
                        explanation: qsTr("Export settings are not available yet.")
                    }
                }
            }
        }
    }
}
