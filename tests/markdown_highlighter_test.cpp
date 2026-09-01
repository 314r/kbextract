#include <QtTest>

#include <QFont>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextFormat>
#include <QTextLayout>

#include "markdown_highlighter.h"

class MarkdownHighlighterTest : public QObject
{
    Q_OBJECT

private slots:
    void stylesOnlyLevelTwoHeadings();
};

void MarkdownHighlighterTest::stylesOnlyLevelTwoHeadings()
{
    const QString markdown = QStringLiteral(
        "## Chapter One\n"
        "\n"
        "> Highlight\n"
        "Note paragraph\n"
        "### Different heading");

    QTextDocument document;
    MarkdownHighlighter highlighter;
    highlighter.setHeadingPixelSize(20);
    highlighter.setDocument(&document);
    document.setPlainText(markdown);
    highlighter.rehighlight();

    const QTextBlock headingBlock = document.firstBlock();
    const QList<QTextLayout::FormatRange> headingFormats = headingBlock.layout()->formats();
    QCOMPARE(headingFormats.size(), 1);
    QCOMPARE(headingFormats.constFirst().start, 0);
    QCOMPARE(headingFormats.constFirst().length, headingBlock.text().size());
    QCOMPARE(headingFormats.constFirst().format.fontWeight(), static_cast<int>(QFont::Bold));
    QCOMPARE(headingFormats.constFirst().format.property(QTextFormat::FontPixelSize).toInt(), 20);

    QTextBlock block = headingBlock.next();
    while (block.isValid()) {
        QVERIFY(block.layout()->formats().isEmpty());
        block = block.next();
    }

    QCOMPARE(document.toPlainText(), markdown);
}

QTEST_GUILESS_MAIN(MarkdownHighlighterTest)

#include "markdown_highlighter_test.moc"
