#include <QtTest>

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "omarchy_theme.h"

namespace {

bool writeTextFile(const QString &path, const QByteArray &contents)
{
    if (!QDir().mkpath(QFileInfo(path).path()))
        return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(contents) == contents.size();
}

QColor paletteColor(const QVariantMap &palette, const QString &key)
{
    return palette.value(key).value<QColor>();
}

} // namespace

class OmarchyThemeTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsCanonicalPalette();
    void mapsLegacyPaletteAliases();
    void suppliesCanonicalFallbackPalette();
    void reloadsPaletteChanges();
    void reportsAvailabilityAndControlsPolling();
};

void OmarchyThemeTest::loadsCanonicalPalette()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(writeTextFile(directory.filePath(QStringLiteral("theme/colors.toml")), QByteArrayLiteral(
        "mode = \"light\"\n"
        "background = \"#010203\"\n"
        "dark_background = \"#111213\"\n"
        "darker_background = \"#212223\"\n"
        "lighter_background = \"#313233\"\n"
        "foreground = \"#f1f2f3\"\n"
        "dark_foreground = \"#414243\"\n"
        "light_foreground = \"#d1d2d3\"\n"
        "bright_foreground = \"#ffffff\"\n"
        "selection = \"#515253\"\n"
        "muted = \"#616263\"\n"
        "accent = \"#718293\"\n")));

    OmarchyTheme theme(directory.path());
    const QVariantMap palette = theme.palette();

    QVERIFY(!theme.dark());
    QCOMPARE(paletteColor(palette, QStringLiteral("background")), QColor(QStringLiteral("#010203")));
    QCOMPARE(paletteColor(palette, QStringLiteral("dark_background")), QColor(QStringLiteral("#111213")));
    QCOMPARE(paletteColor(palette, QStringLiteral("darker_background")), QColor(QStringLiteral("#212223")));
    QCOMPARE(paletteColor(palette, QStringLiteral("lighter_background")), QColor(QStringLiteral("#313233")));
    QCOMPARE(paletteColor(palette, QStringLiteral("foreground")), QColor(QStringLiteral("#f1f2f3")));
    QCOMPARE(paletteColor(palette, QStringLiteral("selection")), QColor(QStringLiteral("#515253")));
    QCOMPARE(paletteColor(palette, QStringLiteral("accent")), QColor(QStringLiteral("#718293")));
}

void OmarchyThemeTest::mapsLegacyPaletteAliases()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(writeTextFile(directory.filePath(QStringLiteral("theme/colors.toml")), QByteArrayLiteral(
        "mode = \"dark\"\n"
        "bg = \"#101112\"\n"
        "dark_bg = \"#202122\"\n"
        "darker_bg = \"#303132\"\n"
        "lighter_bg = \"#404142\"\n"
        "fg = \"#e0e1e2\"\n"
        "dark_fg = \"#505152\"\n"
        "light_fg = \"#d0d1d2\"\n"
        "bright_fg = \"#f0f1f2\"\n"
        "selection_background = \"#606162\"\n")));

    OmarchyTheme theme(directory.path());
    const QVariantMap palette = theme.palette();

    QVERIFY(theme.dark());
    QCOMPARE(paletteColor(palette, QStringLiteral("background")), QColor(QStringLiteral("#101112")));
    QCOMPARE(paletteColor(palette, QStringLiteral("dark_background")), QColor(QStringLiteral("#202122")));
    QCOMPARE(paletteColor(palette, QStringLiteral("darker_background")), QColor(QStringLiteral("#303132")));
    QCOMPARE(paletteColor(palette, QStringLiteral("lighter_background")), QColor(QStringLiteral("#404142")));
    QCOMPARE(paletteColor(palette, QStringLiteral("foreground")), QColor(QStringLiteral("#e0e1e2")));
    QCOMPARE(paletteColor(palette, QStringLiteral("selection")), QColor(QStringLiteral("#606162")));
    QVERIFY(!palette.contains(QStringLiteral("dark_bg")));
    QVERIFY(!palette.contains(QStringLiteral("selection_background")));
}

void OmarchyThemeTest::suppliesCanonicalFallbackPalette()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    OmarchyTheme theme(directory.filePath(QStringLiteral("missing-state")));
    const QVariantMap palette = theme.palette();
    const QStringList requiredKeys = {
        QStringLiteral("background"),
        QStringLiteral("dark_background"),
        QStringLiteral("darker_background"),
        QStringLiteral("lighter_background"),
        QStringLiteral("foreground"),
        QStringLiteral("dark_foreground"),
        QStringLiteral("light_foreground"),
        QStringLiteral("bright_foreground"),
        QStringLiteral("muted"),
        QStringLiteral("accent"),
        QStringLiteral("selection"),
        QStringLiteral("red"),
        QStringLiteral("green"),
        QStringLiteral("yellow"),
        QStringLiteral("cyan"),
    };

    QVERIFY(theme.dark());
    for (const QString &key : requiredKeys)
        QVERIFY2(paletteColor(palette, key).isValid(), qPrintable(key));
}

void OmarchyThemeTest::reloadsPaletteChanges()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString colorsPath = directory.filePath(QStringLiteral("theme/colors.toml"));
    QVERIFY(writeTextFile(colorsPath, QByteArrayLiteral(
        "mode = \"dark\"\n"
        "background = \"#111111\"\n")));

    OmarchyTheme theme(directory.path());
    QSignalSpy paletteChangedSpy(&theme, &OmarchyTheme::paletteChanged);

    QVERIFY(writeTextFile(colorsPath, QByteArrayLiteral(
        "mode = \"light\"\n"
        "background = \"#eeeeee\"\n")));
    theme.reload();

    QCOMPARE(paletteChangedSpy.count(), 1);
    QVERIFY(!theme.dark());
    QCOMPARE(paletteColor(theme.palette(), QStringLiteral("background")), QColor(QStringLiteral("#eeeeee")));
}

void OmarchyThemeTest::reportsAvailabilityAndControlsPolling()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    OmarchyTheme theme(directory.path());
    QVERIFY(!theme.available());
    QVERIFY(!theme.active());

    QSignalSpy availableChangedSpy(&theme, &OmarchyTheme::availableChanged);
    QVERIFY(writeTextFile(directory.filePath(QStringLiteral("theme/colors.toml")), QByteArrayLiteral(
        "mode = \"dark\"\n"
        "background = \"#111111\"\n")));
    theme.reload();

    QVERIFY(theme.available());
    QCOMPARE(availableChangedSpy.count(), 1);

    QSignalSpy activeChangedSpy(&theme, &OmarchyTheme::activeChanged);
    theme.setActive(true);
    QVERIFY(theme.active());
    QCOMPARE(activeChangedSpy.count(), 1);
    theme.setActive(false);
    QVERIFY(!theme.active());
    QCOMPARE(activeChangedSpy.count(), 2);
}

QTEST_GUILESS_MAIN(OmarchyThemeTest)

#include "omarchy_theme_test.moc"
