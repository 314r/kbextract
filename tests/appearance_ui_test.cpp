#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFont>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>

#include <memory>

#include "clipboard_helper.h"
#include "file_url.h"
#include "kobo_library.h"
#include "markdown_highlighter.h"
#include "omarchy_theme.h"
#include "system_appearance.h"

namespace {
QString themeStateRoot;

// Exercise the production UI and appearance bridges without accessing devices
// or the user's Omarchy configuration.
class EmptyLibrary : public KoboLibrary
{
public:
    explicit EmptyLibrary(QObject *parent = nullptr)
        : KoboLibrary([] { return QList<KoboVolume>(); }, 60000, parent) {}
};

class IsolatedTheme : public OmarchyTheme
{
public:
    explicit IsolatedTheme(QObject *parent = nullptr)
        : OmarchyTheme(themeStateRoot, parent) {}
};

QList<QQuickItem *> visualDescendants(QQuickItem *parent, const QString &name)
{
    QList<QQuickItem *> result;
    for (auto *child : parent->childItems()) {
        if (child->objectName() == name)
            result.append(child);
        result.append(visualDescendants(child, name));
    }
    return result;
}
} // namespace

class AppearanceUiTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void themeChangesPreserveTypographyAndGeometry();
    void desktopFontChangesKeepCompactSizes();
    void controlsFitTextAndConfirmation();
    void dropdownRowsUseCompactFonts();

private:
    QList<int> fontSizes() const;
    QList<QRectF> geometry() const;
    QQuickItem *control(const char *name) const;

    std::unique_ptr<QQmlApplicationEngine> m_engine;
    QQuickWindow *m_window = nullptr;
    QQuickWindow *m_settings = nullptr;
    QObject *m_theme = nullptr;
    QFont m_originalFont;
    QStringList m_warnings;
};

void AppearanceUiTest::initTestCase()
{
    qmlRegisterType<ClipboardHelper>("Kbextract", 1, 0, "ClipboardHelper");
    qmlRegisterType<FileUrl>("Kbextract", 1, 0, "FileUrl");
    qmlRegisterType<EmptyLibrary>("Kbextract", 1, 0, "KoboLibrary");
    qmlRegisterType<MarkdownHighlighter>("Kbextract", 1, 0, "MarkdownHighlighter");
    qmlRegisterType<IsolatedTheme>("Kbextract", 1, 0, "OmarchyTheme");
    qmlRegisterType<SystemAppearance>("Kbextract", 1, 0, "SystemAppearance");
    qmlRegisterType(QUrl(QStringLiteral("qrc:/qt/qml/Kbextract/AppSession.qml")),
                    "Kbextract", 1, 0, "AppSession");
    qmlRegisterType(QUrl(QStringLiteral("qrc:/qt/qml/Kbextract/AnnotationBody.qml")),
                    "Kbextract", 1, 0, "AnnotationBody");
    qmlRegisterType(QUrl(QStringLiteral("qrc:/qt/qml/Kbextract/SettingsWindow.qml")),
                    "Kbextract", 1, 0, "SettingsWindow");
    qmlRegisterType(QUrl(QStringLiteral("qrc:/qt/qml/Kbextract/AppearanceSettingsPage.qml")),
                    "Kbextract", 1, 0, "AppearanceSettingsPage");
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/qt/qml/Kbextract/Theme.qml")),
                             "Kbextract", 1, 0, "Theme");
}

void AppearanceUiTest::init()
{
    m_originalFont = QGuiApplication::font();
    QSettings().clear();
    QVERIFY(QDir().mkpath(themeStateRoot + QStringLiteral("/theme")));
    QFile colors(themeStateRoot + QStringLiteral("/theme/colors.toml"));
    QVERIFY(colors.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray palette("mode = \"dark\"\nbackground = \"#030203\"\n");
    QCOMPARE(colors.write(palette), palette.size());
    colors.close();
    m_warnings.clear();
    m_engine = std::make_unique<QQmlApplicationEngine>();
    connect(m_engine.get(), &QQmlEngine::warnings, this, [this](const QList<QQmlError> &errors) {
        for (const auto &error : errors)
            m_warnings.append(error.toString());
    });
    m_engine->load(QUrl(QStringLiteral("qrc:/qt/qml/Kbextract/Main.qml")));
    QVERIFY2(!m_engine->rootObjects().isEmpty(), qPrintable(m_warnings.join('\n')));
    m_window = qobject_cast<QQuickWindow *>(m_engine->rootObjects().constFirst());
    QVERIFY(m_window);
    m_settings = m_window->findChild<QQuickWindow *>(QStringLiteral("settingsWindow"));
    QVERIFY(m_settings);
    m_theme = m_engine->singletonInstance<QObject *>("Kbextract", "Theme");
    QVERIFY(m_theme);
    m_window->resize(1040, 680);
    m_settings->resize(600, 400);
    m_settings->show();
    QTest::qWait(30);
    for (const auto *name : {"copyTextButton", "copyObsidianButton", "copyAllButton",
                             "deviceSelector", "appearanceButton", "themeSelector", "modeButton",
                             "sidebar", "mainColumn", "copyFooter"}) {
        QVERIFY2(control(name), name);
    }
}

void AppearanceUiTest::cleanup()
{
    m_engine.reset();
    m_window = nullptr;
    m_settings = nullptr;
    m_theme = nullptr;
    QGuiApplication::setFont(m_originalFont);
    QVERIFY2(m_warnings.isEmpty(), qPrintable(m_warnings.join('\n')));
}

QQuickItem *AppearanceUiTest::control(const char *name) const
{
    return m_window->findChild<QQuickItem *>(QString::fromLatin1(name));
}

QList<int> AppearanceUiTest::fontSizes() const
{
    QList<int> sizes;
    for (const auto *name : {"fontSizeCaption", "fontSizeBody", "fontSizeHeading",
                             "fontSizeReader", "fontSizeReaderHeading"}) {
        sizes.append(m_theme->property(name).toInt());
    }
    return sizes;
}

QList<QRectF> AppearanceUiTest::geometry() const
{
    QList<QRectF> rectangles;
    for (const auto *name : {"copyTextButton", "copyObsidianButton", "copyAllButton",
                             "deviceSelector", "appearanceButton", "themeSelector"}) {
        const auto *item = control(name);
        rectangles.append(QRectF(item->mapToScene(QPointF()), item->size()));
    }
    return rectangles;
}

void AppearanceUiTest::themeChangesPreserveTypographyAndGeometry()
{
    const QList<int> expectedSizes{10, 12, 16, 15, 20};
    const auto originalGeometry = geometry();
    QVERIFY(m_theme->property("omarchyAvailable").toBool());
    for (const auto *mode : {"system", "light", "dark", "omarchy", "system"}) {
        m_theme->setProperty("mode", QString::fromLatin1(mode));
        QTest::qWait(10);
        QCOMPARE(fontSizes(), expectedSizes);
        QCOMPARE(geometry(), originalGeometry);
        for (const auto *name : {"copyTextButton", "deviceSelector", "appearanceButton", "themeSelector"})
            QCOMPARE(control(name)->property("font").value<QFont>().pixelSize(), 12);

        // Optional artifacts from the same windows exercised by the tests.
        const QString artifactDir = qEnvironmentVariable("KBEXTRACT_UI_ARTIFACT_DIR");
        if (!artifactDir.isEmpty()) {
            QVERIFY(QDir().mkpath(artifactDir));
            const QString prefix = artifactDir + '/' + QString::fromLatin1(mode);
            QVERIFY(m_window->grabWindow().save(prefix + QStringLiteral("-main.png")));
            QVERIFY(m_settings->grabWindow().save(prefix + QStringLiteral("-settings.png")));
        }
    }
    m_theme->setProperty("mode", QStringLiteral("omarchy"));
    QVERIFY(QFile::remove(themeStateRoot + QStringLiteral("/theme/colors.toml")));
    auto *omarchy = m_window->findChild<OmarchyTheme *>();
    QVERIFY(omarchy);
    omarchy->reload();
    QCOMPARE(m_theme->property("mode").toString(), QStringLiteral("system"));
    QCOMPARE(m_theme->property("effectiveMode").toString(), QStringLiteral("system"));
    QCoreApplication::processEvents();
    QCOMPARE(fontSizes(), expectedSizes);
    QCOMPARE(geometry(), originalGeometry);
}

void AppearanceUiTest::desktopFontChangesKeepCompactSizes()
{
    const auto originalGeometry = geometry();
    QFont larger = QGuiApplication::font();
    larger.setPixelSize(36);
    QGuiApplication::setFont(larger);
    QTest::qWait(10);
    QCOMPARE(fontSizes(), (QList<int>{10, 12, 16, 15, 20}));
    QCOMPARE(geometry(), originalGeometry);
    QCOMPARE(m_window->property("font").value<QFont>().pixelSize(), 12);
    QCOMPARE(m_settings->property("font").value<QFont>().pixelSize(), 12);
    QObject *tooltip = QQmlProperty::read(control("modeButton"),
        QStringLiteral("ToolTip.toolTip"), qmlContext(control("modeButton"))).value<QObject *>();
    QVERIFY(tooltip);
    QCOMPARE(tooltip->property("font").value<QFont>().pixelSize(), 12);

    QFont different(QStringLiteral("serif"));
    different.setPixelSize(36);
    QGuiApplication::setFont(different);
    QTRY_COMPARE(m_theme->property("uiFont").toString(), QGuiApplication::font().family());
    QCOMPARE(control("themeSelector")->property("font").value<QFont>().family(),
             QGuiApplication::font().family());
    QCOMPARE(control("themeSelector")->property("font").value<QFont>().pixelSize(), 12);
    QCOMPARE(tooltip->property("font").value<QFont>().family(), QGuiApplication::font().family());
    QCOMPARE(fontSizes(), (QList<int>{10, 12, 16, 15, 20}));
}

void AppearanceUiTest::controlsFitTextAndConfirmation()
{
    auto *sidebar = control("sidebar");
    auto *mainColumn = control("mainColumn");
    auto *footer = control("copyFooter");
    const QRectF sidebarBounds(sidebar->mapToScene(QPointF()), sidebar->size());
    const QRectF mainBounds(mainColumn->mapToScene(QPointF()), mainColumn->size());
    const QRectF footerBounds(footer->mapToScene(QPointF()), footer->size());
    QCOMPARE(sidebarBounds.bottom(), mainBounds.bottom());
    QCOMPARE(footerBounds.left(), mainBounds.left());
    QCOMPARE(footerBounds.right(), mainBounds.right());
    QCOMPARE(footerBounds.bottom(), mainBounds.bottom());
    QCOMPARE(footerBounds.left(), sidebarBounds.right());

    for (const auto *name : {"copyTextButton", "copyObsidianButton", "copyAllButton",
                             "deviceSelector", "appearanceButton", "themeSelector"}) {
        auto *item = control(name);
        auto *content = item->property("contentItem").value<QQuickItem *>();
        QVERIFY(content);
        QVERIFY(item->height() >= 30);
        QVERIFY(item->height() >= content->implicitHeight() + 12);
        QVERIFY(item->property("availableWidth").toReal() >= content->implicitWidth());
        const auto *window = item->window();
        const QRectF bounds(item->mapToScene(QPointF()), item->size());
        QVERIFY(QRectF(0, 0, window->width(), window->height()).contains(bounds));
    }
    for (const auto *name : {"copyTextButton", "copyObsidianButton", "copyAllButton"}) {
        auto *button = control(name);
        const QRectF bounds(button->mapToItem(footer, QPointF()), button->size());
        QVERIFY(QRectF(QPointF(), footer->size()).contains(bounds));
        const QSizeF originalSize = button->size();
        button->setProperty("copyConfirmed", true);
        QCoreApplication::processEvents();
        QCOMPARE(button->size(), originalSize);
        button->setProperty("copyConfirmed", false);
        QCoreApplication::processEvents();
        QCOMPARE(button->size(), originalSize);
    }
}

void AppearanceUiTest::dropdownRowsUseCompactFonts()
{
    QObject *popup = control("themeSelector")->property("popup").value<QObject *>();
    QVERIFY(popup);
    QVERIFY(QMetaObject::invokeMethod(popup, "open"));
    auto *list = popup->property("contentItem").value<QQuickItem *>();
    QVERIFY(list);
    QTRY_VERIFY(popup->property("visible").toBool());
    QTRY_VERIFY(visualDescendants(list, QStringLiteral("themeOption")).size() >= 3);
    for (auto *row : visualDescendants(list, QStringLiteral("themeOption"))) {
        QCOMPARE(row->property("font").value<QFont>().pixelSize(), 12);
        QVERIFY(row->height() >= 30);
        auto *content = row->property("contentItem").value<QQuickItem *>();
        QVERIFY(content);
        QVERIFY(row->height() >= content->implicitHeight() + 12);
    }
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QTemporaryDir settingsDir;
    if (!settingsDir.isValid())
        return 1;
    themeStateRoot = settingsDir.filePath(QStringLiteral("omarchy"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    app.setOrganizationName(QStringLiteral("kbextract-tests"));
    app.setApplicationName(QStringLiteral("appearance-ui"));
    AppearanceUiTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "appearance_ui_test.moc"
