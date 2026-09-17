#include "markdown_highlighter.h"

#include <QFont>
#include <QTextCharFormat>
#include <QTextFormat>

MarkdownHighlighter::MarkdownHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent)
{
}

QQuickTextDocument *MarkdownHighlighter::textDocument() const
{
    return m_textDocument;
}

void MarkdownHighlighter::setTextDocument(QQuickTextDocument *textDocument)
{
    if (m_textDocument == textDocument)
        return;

    m_textDocument = textDocument;
    setDocument(textDocument ? textDocument->textDocument() : nullptr);
    emit textDocumentChanged();
}

int MarkdownHighlighter::headingPixelSize() const
{
    return m_headingPixelSize;
}

void MarkdownHighlighter::setHeadingPixelSize(int pixelSize)
{
    if (pixelSize < 1 || m_headingPixelSize == pixelSize)
        return;

    m_headingPixelSize = pixelSize;
    rehighlight();
    emit headingPixelSizeChanged();
}

qreal MarkdownHighlighter::headingPointSize() const
{
    return m_headingPointSize;
}

void MarkdownHighlighter::setHeadingPointSize(qreal pointSize)
{
    const qreal normalized = pointSize > 0.0 ? pointSize : -1.0;
    if (m_headingPointSize == normalized)
        return;

    m_headingPointSize = normalized;
    rehighlight();
    emit headingPointSizeChanged();
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    if (!text.startsWith(QStringLiteral("## ")))
        return;

    QTextCharFormat headingFormat;
    headingFormat.setFontWeight(QFont::Bold);
    if (m_headingPointSize > 0.0)
        headingFormat.setFontPointSize(m_headingPointSize);
    else
        headingFormat.setProperty(QTextFormat::FontPixelSize, m_headingPixelSize);
    setFormat(0, text.size(), headingFormat);
}
