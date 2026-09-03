#include <QtTest>

#include <QClipboard>
#include <QGuiApplication>

#include "clipboard_helper.h"

class ClipboardHelperTest : public QObject
{
    Q_OBJECT

private slots:
    void copiesText();
    void rejectsEmptyText();
};

void ClipboardHelperTest::copiesText()
{
    ClipboardHelper helper;
    const QString markdown = QStringLiteral("## Chapter\n\n> [!quote]\n> Highlight");

    QVERIFY(helper.copyText(markdown));
    QCOMPARE(QGuiApplication::clipboard()->text(QClipboard::Clipboard), markdown);
}

void ClipboardHelperTest::rejectsEmptyText()
{
    ClipboardHelper helper;
    QGuiApplication::clipboard()->setText(QStringLiteral("existing"), QClipboard::Clipboard);

    QVERIFY(!helper.copyText({}));
    QCOMPARE(QGuiApplication::clipboard()->text(QClipboard::Clipboard), QStringLiteral("existing"));
}

QTEST_MAIN(ClipboardHelperTest)

#include "clipboard_helper_test.moc"
