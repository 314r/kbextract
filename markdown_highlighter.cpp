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

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    if (!text.startsWith(QStringLiteral("## ")))
        return;

    QTextCharFormat headingFormat;
    headingFormat.setFontWeight(QFont::Bold);
    headingFormat.setProperty(QTextFormat::FontPixelSize, m_headingPixelSize);
    setFormat(0, text.size(), headingFormat);
}
