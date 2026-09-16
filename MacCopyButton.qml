import QtQuick
import QtQuick.Controls.macOS
import QtQuick.Layouts

Button {
    id: control
    required property bool confirmed
    required property string label
    readonly property string confirmationText: qsTr("COPIED")
    readonly property real fixedWidth: Math.ceil(Math.max(
        normalWidthProbe.implicitWidth, confirmationWidthProbe.implicitWidth))

    text: confirmed ? confirmationText : label
    Layout.minimumWidth: fixedWidth
    Layout.preferredWidth: fixedWidth
    Layout.maximumWidth: fixedWidth
    Accessible.name: action ? action.text : label

    // Native styles can include the current label in their background metrics.
    // Constant-label probes keep the layout width independent of feedback state.
    Button {
        id: normalWidthProbe
        visible: false
        font: control.font
        text: control.label
    }

    Button {
        id: confirmationWidthProbe
        visible: false
        font: control.font
        text: control.confirmationText
    }
}
