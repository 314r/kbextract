import QtQuick
import QtQuick.Controls.Basic
import Kbextract

// Intentionally custom on every platform: only the surrounding controls and
// scrolling belong to the platform style.
TextArea {
    id: body
    objectName: "annotationText"

    required property string markdown
    readonly property int wrapColumn: 120
    readonly property real minimumHorizontalPadding: 32
    readonly property real wrapWidth: annotationFontMetrics.advanceWidth("M".repeat(wrapColumn))
    readonly property real responsiveHorizontalPadding: Math.max(
        minimumHorizontalPadding, (width - wrapWidth) / 2)

    height: implicitHeight
    text: markdown
    textFormat: TextEdit.PlainText
    readOnly: true
    ContextMenu.menu: null
    selectByMouse: true
    persistentSelection: true
    wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
    color: Theme.text
    selectedTextColor: Theme.accentText
    selectionColor: Theme.accent
    font.family: Theme.monoFont
    font.pointSize: Theme.adaptiveTypography ? Theme.fontPointSizeReader : -1
    font.pixelSize: Theme.adaptiveTypography ? -1 : Theme.fontSizeReader
    leftPadding: responsiveHorizontalPadding
    rightPadding: responsiveHorizontalPadding
    topPadding: 28
    bottomPadding: 28
    Accessible.name: qsTr("Highlights and notes")

    FontMetrics {
        id: annotationFontMetrics
        font: body.font
    }

    MarkdownHighlighter {
        textDocument: body.textDocument
        headingPixelSize: Theme.fontSizeReaderHeading
        headingPointSize: Theme.adaptiveTypography ? Theme.fontPointSizeReaderHeading : -1
    }

    background: Rectangle { color: "transparent" }
}
