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

int fontSize(const QVariantMap &fontSizes, const QString &key)
{
    return fontSizes.value(key).toInt();
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
    void suppliesDefaultFontSizes();
    void scalesAndOverridesFontSizes();
    void userFontSizesOverrideTheme();
    void ignoresInvalidFontSizes();
    void reloadsFontSizeChanges();
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

void OmarchyThemeTest::suppliesDefaultFontSizes()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    OmarchyTheme theme(directory.path());

    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("caption")), 10);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("body")), 12);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("heading")), 16);
}

void OmarchyThemeTest::scalesAndOverridesFontSizes()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(writeTextFile(directory.filePath(QStringLiteral("theme/shell.toml")), QByteArrayLiteral(
        "[font]\n"
        "base-size = 13\n"
        "caption = 9\n"
        "heading = 21 # Theme emphasis\n")));

    OmarchyTheme theme(directory.path());

    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("caption")), 9);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("body")), 13);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("heading")), 21);
}

void OmarchyThemeTest::userFontSizesOverrideTheme()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString userShellPath = directory.filePath(QStringLiteral("config/shell.toml"));
    QVERIFY(writeTextFile(directory.filePath(QStringLiteral("state/theme/shell.toml")), QByteArrayLiteral(
        "[font]\n"
        "base-size = 13\n"
        "caption = 10\n"
        "heading = 18\n")));
    QVERIFY(writeTextFile(userShellPath, QByteArrayLiteral(
        "[font]\n"
        "base-size = 16\n"
        "heading = 22\n")));

    OmarchyTheme theme(directory.filePath(QStringLiteral("state")), userShellPath);

    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("caption")), 10);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("body")), 16);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("heading")), 22);
}

void OmarchyThemeTest::ignoresInvalidFontSizes()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(writeTextFile(directory.filePath(QStringLiteral("theme/shell.toml")), QByteArrayLiteral(
        "[font]\n"
        "base-size = invalid\n"
        "caption = -3\n"
        "body = 0\n"
        "heading = '17'\n")));

    OmarchyTheme theme(directory.path());

    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("caption")), 10);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("body")), 12);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("heading")), 17);
}

void OmarchyThemeTest::reloadsFontSizeChanges()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString shellPath = directory.filePath(QStringLiteral("theme/shell.toml"));
    QVERIFY(writeTextFile(shellPath, QByteArrayLiteral(
        "[font]\n"
        "base-size = 12\n")));

    OmarchyTheme theme(directory.path());
    QSignalSpy fontSizesChangedSpy(&theme, &OmarchyTheme::fontSizesChanged);
    QSignalSpy paletteChangedSpy(&theme, &OmarchyTheme::paletteChanged);

    QVERIFY(writeTextFile(shellPath, QByteArrayLiteral(
        "[font]\n"
        "base-size = 15\n")));
    theme.reload();

    QCOMPARE(fontSizesChangedSpy.count(), 1);
    QCOMPARE(paletteChangedSpy.count(), 0);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("caption")), 12);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("body")), 15);
    QCOMPARE(fontSize(theme.fontSizes(), QStringLiteral("heading")), 20);
}

QTEST_GUILESS_MAIN(OmarchyThemeTest)

#include "omarchy_theme_test.moc"
