#pragma once

#include <QPointer>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument *textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged)
    Q_PROPERTY(int headingPixelSize READ headingPixelSize WRITE setHeadingPixelSize NOTIFY headingPixelSizeChanged)
    Q_PROPERTY(qreal headingPointSize READ headingPointSize WRITE setHeadingPointSize NOTIFY headingPointSizeChanged)

public:
    explicit MarkdownHighlighter(QObject *parent = nullptr);

    QQuickTextDocument *textDocument() const;
    void setTextDocument(QQuickTextDocument *textDocument);

    int headingPixelSize() const;
    void setHeadingPixelSize(int pixelSize);
    qreal headingPointSize() const;
    void setHeadingPointSize(qreal pointSize);

signals:
    void textDocumentChanged();
    void headingPixelSizeChanged();
    void headingPointSizeChanged();

protected:
    void highlightBlock(const QString &text) override;

private:
    QPointer<QQuickTextDocument> m_textDocument;
    int m_headingPixelSize = 20;
    qreal m_headingPointSize = -1.0;
};
