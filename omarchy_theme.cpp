#include "omarchy_theme.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTextStream>

namespace {

QString defaultOmarchyStateRoot()
{
    return QDir::homePath() + QStringLiteral("/.local/state/omarchy/current");
}

QString defaultOmarchyUserShellPath()
{
    return QDir::homePath() + QStringLiteral("/.config/omarchy/shell.toml");
}

QString canonicalPaletteKey(const QString &key)
{
    if (key == QLatin1String("bg"))
        return QStringLiteral("background");
    if (key == QLatin1String("dark_bg"))
        return QStringLiteral("dark_background");
    if (key == QLatin1String("darker_bg"))
        return QStringLiteral("darker_background");
    if (key == QLatin1String("lighter_bg"))
        return QStringLiteral("lighter_background");
    if (key == QLatin1String("fg"))
        return QStringLiteral("foreground");
    if (key == QLatin1String("dark_fg"))
        return QStringLiteral("dark_foreground");
    if (key == QLatin1String("light_fg"))
        return QStringLiteral("light_foreground");
    if (key == QLatin1String("bright_fg"))
        return QStringLiteral("bright_foreground");
    if (key == QLatin1String("selection_background"))
        return QStringLiteral("selection");
    return key;
}

QVariantMap fallbackPalette()
{
    return {
        {QStringLiteral("background"), QColor(QStringLiteral("#030203"))},
        {QStringLiteral("dark_background"), QColor(QStringLiteral("#080708"))},
        {QStringLiteral("darker_background"), QColor(QStringLiteral("#050505"))},
        {QStringLiteral("lighter_background"), QColor(QStringLiteral("#232223"))},
        {QStringLiteral("foreground"), QColor(QStringLiteral("#b5bfc4"))},
        {QStringLiteral("dark_foreground"), QColor(QStringLiteral("#747b7f"))},
        {QStringLiteral("light_foreground"), QColor(QStringLiteral("#a9b2b6"))},
        {QStringLiteral("bright_foreground"), QColor(QStringLiteral("#b3bbbf"))},
        {QStringLiteral("muted"), QColor(QStringLiteral("#5c5c5c"))},
        {QStringLiteral("accent"), QColor(QStringLiteral("#b85d4d"))},
        {QStringLiteral("selection"), QColor(QStringLiteral("#2d2b2d"))},
        {QStringLiteral("red"), QColor(QStringLiteral("#b85d4d"))},
        {QStringLiteral("orange"), QColor(QStringLiteral("#b86f4d"))},
        {QStringLiteral("green"), QColor(QStringLiteral("#687d60"))},
        {QStringLiteral("yellow"), QColor(QStringLiteral("#877364"))},
        {QStringLiteral("cyan"), QColor(QStringLiteral("#b5bfc4"))},
        {QStringLiteral("blue"), QColor(QStringLiteral("#7895a3"))},
        {QStringLiteral("magenta"), QColor(QStringLiteral("#997789"))},
        {QStringLiteral("brown"), QColor(QStringLiteral("#70432e"))},
        {QStringLiteral("bright_red"), QColor(QStringLiteral("#dc7c6a"))},
        {QStringLiteral("bright_yellow"), QColor(QStringLiteral("#a79282"))},
        {QStringLiteral("bright_green"), QColor(QStringLiteral("#8da584"))},
        {QStringLiteral("bright_cyan"), QColor(QStringLiteral("#d6e0e6"))},
        {QStringLiteral("bright_blue"), QColor(QStringLiteral("#9ab7c4"))},
        {QStringLiteral("bright_magenta"), QColor(QStringLiteral("#bd9aaa"))},
    };
}

QString readTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll()).trimmed();
}

QHash<QString, int> readFontSettings(const QString &path)
{
    QHash<QString, int> settings;
    if (path.isEmpty())
        return settings;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return settings;

    QTextStream stream(&file);
    QString section;
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        const qsizetype comment = line.indexOf(QLatin1Char('#'));
        if (comment >= 0)
            line.truncate(comment);
        line = line.trimmed();

        if (line.isEmpty())
            continue;

        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            section = line.mid(1, line.size() - 2).trimmed();
            continue;
        }

        if (section != QLatin1String("font"))
            continue;

        const qsizetype separator = line.indexOf(QLatin1Char('='));
        if (separator < 0)
            continue;

        const QString key = line.left(separator).trimmed();
        QString value = line.mid(separator + 1).trimmed();
        if (value.size() >= 2
            && ((value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
                || (value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\''))))) {
            value = value.mid(1, value.size() - 2);
        }

        bool ok = false;
        const int pixelSize = value.toInt(&ok);
        if (ok)
            settings.insert(key, pixelSize);
    }

    return settings;
}

QVariantMap loadFontSizes(const QString &themeShellPath, const QString &userShellPath)
{
    QHash<QString, int> settings = readFontSettings(themeShellPath);
    const QHash<QString, int> userSettings = readFontSettings(userShellPath);
    for (auto it = userSettings.cbegin(); it != userSettings.cend(); ++it)
        settings.insert(it.key(), it.value());

    int baseSize = settings.value(QStringLiteral("base-size"), 12);
    baseSize = qMax(1, baseSize);

    const auto fontSize = [&settings, baseSize](const QString &key, double multiplier) {
        const int overrideSize = settings.value(key, 0);
        return overrideSize > 0 ? overrideSize : qMax(1, qRound(baseSize * multiplier));
    };

    return {
        {QStringLiteral("caption"), fontSize(QStringLiteral("caption"), 0.833)},
        {QStringLiteral("body"), fontSize(QStringLiteral("body"), 1.0)},
        {QStringLiteral("heading"), fontSize(QStringLiteral("heading"), 1.333)},
    };
}

} // namespace

OmarchyTheme::OmarchyTheme(QObject *parent)
    : OmarchyTheme(defaultOmarchyStateRoot(), defaultOmarchyUserShellPath(), parent)
{
}

OmarchyTheme::OmarchyTheme(const QString &stateRoot, QObject *parent)
    : OmarchyTheme(stateRoot, QString(), parent)
{
}

OmarchyTheme::OmarchyTheme(const QString &stateRoot, const QString &userShellPath, QObject *parent)
    : QObject(parent)
    , m_stateRoot(QDir::cleanPath(stateRoot))
    , m_userShellPath(userShellPath.isEmpty() ? QString() : QDir::cleanPath(userShellPath))
{
    connect(&m_refreshTimer, &QTimer::timeout, this, &OmarchyTheme::reload);
    m_refreshTimer.setInterval(1500);
    reload();
}

QVariantMap OmarchyTheme::palette() const
{
    return m_palette;
}

QVariantMap OmarchyTheme::fontSizes() const
{
    return m_fontSizes;
}

bool OmarchyTheme::dark() const
{
    return m_dark;
}

QString OmarchyTheme::name() const
{
    return m_name;
}

bool OmarchyTheme::available() const
{
    return m_available;
}

bool OmarchyTheme::active() const
{
    return m_active;
}

void OmarchyTheme::setActive(bool active)
{
    if (m_active == active)
        return;

    m_active = active;
    if (m_active) {
        reload();
        m_refreshTimer.start();
    } else {
        m_refreshTimer.stop();
    }
    emit activeChanged();
}

void OmarchyTheme::reload()
{
    QVariantMap palette = fallbackPalette();
    const QVariantMap fontSizes = loadFontSizes(
        QDir(m_stateRoot).filePath(QStringLiteral("theme/shell.toml")),
        m_userShellPath);
    QString mode = QStringLiteral("dark");

    const QString colorsPath = QDir(m_stateRoot).filePath(QStringLiteral("theme/colors.toml"));
    QFile file(colorsPath);
    const bool available = QFileInfo(colorsPath).isReadable();
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
                continue;

            const qsizetype separator = line.indexOf(QLatin1Char('='));
            if (separator < 0)
                continue;

            const QString key = line.left(separator).trimmed();
            QString value = line.mid(separator + 1).trimmed();
            if (value.size() >= 2 && value.startsWith(QLatin1Char('"'))
                && value.endsWith(QLatin1Char('"'))) {
                value = value.mid(1, value.size() - 2);
            }

            if (key == QLatin1String("mode")) {
                mode = value;
                continue;
            }

            const QColor color(value);
            if (color.isValid())
                palette.insert(canonicalPaletteKey(key), color);
        }
    }

    const bool dark = mode.compare(QLatin1String("dark"), Qt::CaseInsensitive) == 0;
    const QString name = readTextFile(QDir(m_stateRoot).filePath(QStringLiteral("theme.name")));
    const bool paletteDidChange = m_palette != palette || m_dark != dark;
    const bool fontSizesDidChange = m_fontSizes != fontSizes;
    const bool nameDidChange = m_name != name;
    const bool availabilityDidChange = m_available != available;

    m_palette = std::move(palette);
    m_fontSizes = fontSizes;
    m_dark = dark;
    m_name = name;
    m_available = available;

    if (paletteDidChange)
        emit paletteChanged();
    if (fontSizesDidChange)
        emit fontSizesChanged();
    if (nameDidChange)
        emit nameChanged();
    if (availabilityDidChange)
        emit availableChanged();
}
