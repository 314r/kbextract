#include <QtTest>

#include <QColor>
#include <QFont>
#include <QGuiApplication>
#include <QPalette>

#include "system_appearance.h"

namespace {

QColor paletteColor(const QVariantMap &palette, const QString &key)
{
    return palette.value(key).value<QColor>();
}

} // namespace

class SystemAppearanceTest : public QObject
{
    Q_OBJECT

private slots:
    void mapsPlatformPaletteToThemeTokens();
    void followsApplicationFontChanges();
    void suppliesLiveApplicationAppearance();
};

void SystemAppearanceTest::mapsPlatformPaletteToThemeTokens()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#101112")));
    palette.setColor(QPalette::Base, QColor(QStringLiteral("#202122")));
    palette.setColor(QPalette::Button, QColor(QStringLiteral("#303132")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#e0e1e2")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#d0d1d2")));
    palette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#707172")));
    palette.setColor(QPalette::Mid, QColor(QStringLiteral("#505152")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#718293")));
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));

    const QVariantMap mapped = SystemAppearance::paletteFrom(palette, true);

    QCOMPARE(paletteColor(mapped, QStringLiteral("background")), QColor(QStringLiteral("#202122")));
    QCOMPARE(paletteColor(mapped, QStringLiteral("dark_background")), QColor(QStringLiteral("#101112")));
    QCOMPARE(paletteColor(mapped, QStringLiteral("lighter_background")), QColor(QStringLiteral("#303132")));
    QCOMPARE(paletteColor(mapped, QStringLiteral("foreground")), QColor(QStringLiteral("#e0e1e2")));
    QCOMPARE(paletteColor(mapped, QStringLiteral("accent")), QColor(QStringLiteral("#718293")));
    QCOMPARE(paletteColor(mapped, QStringLiteral("accent_text")), QColor(QStringLiteral("#ffffff")));
    QVERIFY(paletteColor(mapped, QStringLiteral("selection")).isValid());
}

void SystemAppearanceTest::followsApplicationFontChanges()
{
    const QFont original = QGuiApplication::font();
    const auto restore = qScopeGuard([original] { QGuiApplication::setFont(original); });
    SystemAppearance appearance;
    QSignalSpy fontsChanged(&appearance, &SystemAppearance::fontsChanged);
    QFont changed(QStringLiteral("monospace"));
    changed.setPixelSize(24);
    QGuiApplication::setFont(changed);

    QTRY_COMPARE(appearance.uiFont(), QGuiApplication::font());
    QVERIFY(!fontsChanged.isEmpty());
}

void SystemAppearanceTest::suppliesLiveApplicationAppearance()
{
    SystemAppearance appearance;

    QVERIFY(!appearance.palette().isEmpty());
    QVERIFY(appearance.palette().contains(QStringLiteral("accent")));
    QVERIFY(!appearance.uiFont().family().isEmpty());
    QVERIFY(!appearance.fixedFont().family().isEmpty());
}

QTEST_MAIN(SystemAppearanceTest)

#include "system_appearance_test.moc"
