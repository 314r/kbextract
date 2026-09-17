#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontInfo>
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
    void desktopFontChangesUpdateTypography();
    void textScalePersistsAndClamps();
    void controlsFitTextAndConfirmation();
    void readerUsesCurrentScaleAndWraps();
    void dropdownRowsUseCurrentFontSize();

private:
    bool loadUi();
    QList<qreal> fontPointSizes() const;
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
    QVERIFY2(loadUi(), qPrintable(m_warnings.join('\n')));
    for (const auto *name : {"copyTextButton", "copyObsidianButton", "copyAllButton",
                             "deviceSelector", "appearanceButton", "themeSelector", "modeButton",
                             "textSizeDecreaseButton", "textSizeResetButton", "textSizeIncreaseButton",
                             "annotationText", "sidebar", "mainColumn", "copyFooter"}) {
        QVERIFY2(control(name), name);
    }
}

bool AppearanceUiTest::loadUi()
{
    m_engine = std::make_unique<QQmlApplicationEngine>();
    connect(m_engine.get(), &QQmlEngine::warnings, this, [this](const QList<QQmlError> &errors) {
        for (const auto &error : errors)
            m_warnings.append(error.toString());
    });
    m_engine->load(QUrl(QStringLiteral("qrc:/qt/qml/Kbextract/Main.qml")));
    if (m_engine->rootObjects().isEmpty())
        return false;
    m_window = qobject_cast<QQuickWindow *>(m_engine->rootObjects().constFirst());
    if (!m_window)
        return false;
    m_settings = m_window->findChild<QQuickWindow *>(QStringLiteral("settingsWindow"));
    if (!m_settings)
        return false;
    m_theme = m_engine->singletonInstance<QObject *>("Kbextract", "Theme");
    if (!m_theme)
        return false;
    m_window->resize(1040, 680);
    m_settings->resize(600, 400);
    m_settings->show();
    QTest::qWait(30);
    return true;
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

QList<qreal> AppearanceUiTest::fontPointSizes() const
{
    QList<qreal> sizes;
    for (const auto *name : {"fontPointSizeCaption", "fontPointSizeBody", "fontPointSizeHeading",
                             "fontPointSizeReader", "fontPointSizeReaderHeading"}) {
        sizes.append(m_theme->property(name).toReal());
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
    QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, 140)));
    QCoreApplication::processEvents();
    const QList<qreal> expectedSizes = fontPointSizes();
    const auto originalGeometry = geometry();
    QVERIFY(m_theme->property("omarchyAvailable").toBool());
    for (const auto *mode : {"system", "light", "dark", "omarchy", "system"}) {
        m_theme->setProperty("mode", QString::fromLatin1(mode));
        QTest::qWait(100);
        QCOMPARE(fontPointSizes(), expectedSizes);
        QCOMPARE(m_theme->property("effectiveTextScalePercent").toInt(), 140);
        QCOMPARE(geometry(), originalGeometry);
        for (const auto *name : {"copyTextButton", "deviceSelector", "appearanceButton", "themeSelector"}) {
            QCOMPARE(control(name)->property("font").value<QFont>().pointSizeF(),
                     m_theme->property("fontPointSizeBody").toReal());
        }

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
    QCOMPARE(fontPointSizes(), expectedSizes);
    QCOMPARE(m_theme->property("effectiveTextScalePercent").toInt(), 140);
    QCOMPARE(geometry(), originalGeometry);
}

void AppearanceUiTest::desktopFontChangesUpdateTypography()
{
    QFont larger = QGuiApplication::font();
    larger.setPointSizeF(18.0);
    QGuiApplication::setFont(larger);
    const qreal resolvedSize = QFontInfo(QGuiApplication::font()).pointSizeF();
    QTRY_COMPARE(m_theme->property("systemFontPointSize").toReal(), resolvedSize);
    const QList<qreal> expectedSizes{
        resolvedSize * 10.0 / 12.0,
        resolvedSize,
        resolvedSize * 16.0 / 12.0,
        resolvedSize * 15.0 / 12.0,
        resolvedSize * 20.0 / 12.0,
    };
    QCOMPARE(fontPointSizes(), expectedSizes);
    QCOMPARE(m_window->property("font").value<QFont>().pointSizeF(), resolvedSize);
    QCOMPARE(m_settings->property("font").value<QFont>().pointSizeF(), resolvedSize);
    QObject *tooltip = QQmlProperty::read(control("modeButton"),
        QStringLiteral("ToolTip.toolTip"), qmlContext(control("modeButton"))).value<QObject *>();
    QVERIFY(tooltip);
    QCOMPARE(tooltip->property("font").value<QFont>().pointSizeF(), resolvedSize);

    QFont different(QStringLiteral("serif"));
    different.setPointSizeF(16.0);
    QGuiApplication::setFont(different);
    QTRY_COMPARE(m_theme->property("uiFont").toString(), QGuiApplication::font().family());
    const qreal differentSize = QFontInfo(QGuiApplication::font()).pointSizeF();
    QTRY_COMPARE(m_theme->property("systemFontPointSize").toReal(), differentSize);
    QCOMPARE(control("themeSelector")->property("font").value<QFont>().family(),
             QGuiApplication::font().family());
    QCOMPARE(control("themeSelector")->property("font").value<QFont>().pointSizeF(), differentSize);
    QCOMPARE(tooltip->property("font").value<QFont>().family(), QGuiApplication::font().family());
    QCOMPARE(m_theme->property("fontPointSizeBody").toReal(), differentSize);
}

void AppearanceUiTest::textScalePersistsAndClamps()
{
    QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, 110)));
    QCoreApplication::processEvents();
    QCOMPARE(m_theme->property("effectiveTextScalePercent").toInt(), 110);
    QSettings().sync();
    QCOMPARE(QSettings().value("Appearance/textScalePercent").toInt(), 110);

    m_engine.reset();
    m_window = nullptr;
    m_settings = nullptr;
    m_theme = nullptr;
    QVERIFY2(loadUi(), qPrintable(m_warnings.join('\n')));
    QCOMPARE(m_theme->property("effectiveTextScalePercent").toInt(), 110);

    m_engine.reset();
    m_window = nullptr;
    m_settings = nullptr;
    m_theme = nullptr;
    QSettings settings;
    settings.setValue(QStringLiteral("Appearance/textScalePercent"), 1000);
    settings.sync();
    QVERIFY2(loadUi(), qPrintable(m_warnings.join('\n')));
    QCOMPARE(m_theme->property("effectiveTextScalePercent").toInt(), 200);
    QSettings().sync();
    QCOMPARE(QSettings().value("Appearance/textScalePercent").toInt(), 200);

    m_engine.reset();
    m_window = nullptr;
    m_settings = nullptr;
    m_theme = nullptr;
    settings.setValue(QStringLiteral("Appearance/textScalePercent"), -10);
    settings.sync();
    QVERIFY2(loadUi(), qPrintable(m_warnings.join('\n')));
    QCOMPARE(m_theme->property("effectiveTextScalePercent").toInt(), 80);
    QSettings().sync();
    QCOMPARE(QSettings().value("Appearance/textScalePercent").toInt(), 80);

    auto *reset = control("textSizeResetButton");
    QTest::mouseClick(m_settings, Qt::LeftButton, Qt::NoModifier,
                      reset->mapToScene(QPointF(reset->width() / 2, reset->height() / 2)).toPoint());
    QTRY_COMPARE(m_theme->property("effectiveTextScalePercent").toInt(), 100);
    QSettings().sync();
    QCOMPARE(QSettings().value("Appearance/textScalePercent").toInt(), 100);
}

void AppearanceUiTest::controlsFitTextAndConfirmation()
{
    for (const int scale : {80, 100, 200}) {
        QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, scale)));
        QTest::qWait(10);

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
                                 "deviceSelector", "appearanceButton", "themeSelector",
                                 "textSizeDecreaseButton", "textSizeResetButton",
                                 "textSizeIncreaseButton"}) {
            auto *item = control(name);
            auto *content = item->property("contentItem").value<QQuickItem *>();
            QVERIFY(content);
            QVERIFY(item->height() >= 30);
            QVERIFY2(item->height() >= content->implicitHeight() + 12,
                     qPrintable(QStringLiteral("%1 has insufficient vertical padding at %2%: "
                                               "height %3, implicit %4, content %5")
                         .arg(QString::fromLatin1(name)).arg(scale)
                         .arg(item->height()).arg(item->implicitHeight())
                         .arg(content->implicitHeight())));
            QVERIFY2(item->property("availableWidth").toReal() >= content->implicitWidth(),
                     qPrintable(QStringLiteral("%1 has insufficient horizontal space at %2%: "
                                               "available %3, content %4")
                         .arg(QString::fromLatin1(name)).arg(scale)
                         .arg(item->property("availableWidth").toReal())
                         .arg(content->implicitWidth())));
            const auto *window = item->window();
            const QRectF bounds(item->mapToScene(QPointF()), item->size());
            QVERIFY2(QRectF(0, 0, window->width(), window->height()).contains(bounds),
                     qPrintable(QStringLiteral("%1 is outside its window at %2%: %3,%4 %5x%6; "
                                               "main=%7,%8 %9x%10; footer=%11,%12 %13x%14")
                         .arg(QString::fromLatin1(name)).arg(scale)
                         .arg(bounds.x()).arg(bounds.y()).arg(bounds.width()).arg(bounds.height())
                         .arg(mainBounds.x()).arg(mainBounds.y())
                         .arg(mainBounds.width()).arg(mainBounds.height())
                         .arg(footerBounds.x()).arg(footerBounds.y())
                         .arg(footerBounds.width()).arg(footerBounds.height())));
        }
        QObject *tooltip = QQmlProperty::read(control("modeButton"),
            QStringLiteral("ToolTip.toolTip"), qmlContext(control("modeButton"))).value<QObject *>();
        QVERIFY(tooltip);
        QCOMPARE(tooltip->property("font").value<QFont>().pointSizeF(),
                 m_theme->property("fontPointSizeBody").toReal());
        for (const auto *name : {"copyTextButton", "copyObsidianButton", "copyAllButton"}) {
            auto *button = control(name);
            const QRectF bounds(button->mapToItem(footer, QPointF()), button->size());
            QVERIFY(QRectF(QPointF(), footer->size()).contains(bounds));
            const QSizeF originalSize = button->size();
            button->setProperty("copyConfirmed", true);
            QCoreApplication::processEvents();
            QVERIFY2(button->size() == originalSize,
                     qPrintable(QStringLiteral("%1 changed size at %2%: %3x%4 to %5x%6")
                         .arg(QString::fromLatin1(name)).arg(scale)
                         .arg(originalSize.width()).arg(originalSize.height())
                         .arg(button->width()).arg(button->height())));
            button->setProperty("copyConfirmed", false);
            QCoreApplication::processEvents();
            QVERIFY2(button->size() == originalSize,
                     qPrintable(QStringLiteral("%1 did not restore size at %2%")
                         .arg(QString::fromLatin1(name)).arg(scale)));
        }
    }
}

void AppearanceUiTest::readerUsesCurrentScaleAndWraps()
{
    auto *reader = control("annotationText");
    reader->setWidth(360);
    reader->setProperty("markdown", QStringLiteral(
        "## A reader heading\n\n"
        "This deliberately long paragraph verifies that reader text continues to wrap "
        "inside a narrow reading column as adaptive typography becomes larger. "
        "It repeats enough words to occupy several visual lines at every supported size."));
    auto *highlighter = reader->findChild<MarkdownHighlighter *>();
    QVERIFY(highlighter);

    qreal smallContentHeight = 0.0;
    for (const int scale : {80, 200}) {
        QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, scale)));
        QTest::qWait(10);
        QCOMPARE(reader->property("font").value<QFont>().pointSizeF(),
                 m_theme->property("fontPointSizeReader").toReal());
        QCOMPARE(highlighter->headingPointSize(),
                 m_theme->property("fontPointSizeReaderHeading").toReal());
        QVERIFY(reader->property("leftPadding").toReal() >= 32.0);
        QVERIFY(reader->property("rightPadding").toReal() >= 32.0);
        QVERIFY(reader->property("contentWidth").toReal() <= reader->width());
        const qreal contentHeight = reader->property("contentHeight").toReal();
        QVERIFY(contentHeight > 0.0);
        if (scale == 80)
            smallContentHeight = contentHeight;
        else
            QVERIFY(contentHeight > smallContentHeight);
    }
}

void AppearanceUiTest::dropdownRowsUseCurrentFontSize()
{
    QObject *popup = control("themeSelector")->property("popup").value<QObject *>();
    QVERIFY(popup);
    auto *list = popup->property("contentItem").value<QQuickItem *>();
    QVERIFY(list);
    for (const int scale : {80, 100, 200}) {
        QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, scale)));
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTRY_VERIFY(popup->property("visible").toBool());
        QTRY_VERIFY(visualDescendants(list, QStringLiteral("themeOption")).size() >= 3);
        for (auto *row : visualDescendants(list, QStringLiteral("themeOption"))) {
            QCOMPARE(row->property("font").value<QFont>().pointSizeF(),
                     m_theme->property("fontPointSizeBody").toReal());
            QVERIFY(row->height() >= 30);
            auto *content = row->property("contentItem").value<QQuickItem *>();
            QVERIFY(content);
            QVERIFY(row->height() >= content->implicitHeight() + 12);
        }
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTRY_VERIFY(!popup->property("visible").toBool());
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
