#pragma once

#include <QPointer>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument *textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged)
    Q_PROPERTY(int headingPixelSize READ headingPixelSize WRITE setHeadingPixelSize NOTIFY headingPixelSizeChanged)

public:
    explicit MarkdownHighlighter(QObject *parent = nullptr);

    QQuickTextDocument *textDocument() const;
    void setTextDocument(QQuickTextDocument *textDocument);

    int headingPixelSize() const;
    void setHeadingPixelSize(int pixelSize);

signals:
    void textDocumentChanged();
    void headingPixelSizeChanged();

protected:
    void highlightBlock(const QString &text) override;

private:
    QPointer<QQuickTextDocument> m_textDocument;
    int m_headingPixelSize = 20;
};
