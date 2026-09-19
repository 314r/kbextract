#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlExpression>
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
    void chromeFollowsBodyMetricsAndFitsSidebar();
    void settingsFollowLiveOmarchyPalette();
    void settingsSidebarNavigation();
    void settingsCloseAndReopen();
    void appearanceSubsectionsUseKirigamiSpacing();

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
                             "annotationText", "sidebar", "mainColumn", "copyFooter",
                             "deviceLabel", "booksLabel", "bookCount", "refreshButton", "browseButton",
                             "deviceActions", "exportButton", "exportPlaceholder"}) {
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
    const QString objectName = QString::fromLatin1(name);
    if (auto *item = m_window->findChild<QQuickItem *>(objectName))
        return item;
    // Drawer delegates are visually parented into the window overlay.
    const auto items = visualDescendants(m_settings->contentItem(), objectName);
    return items.isEmpty() ? nullptr : items.constFirst();
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
                             "deviceSelector", "appearanceButton", "themeSelector",
                             "refreshButton", "browseButton", "modeButton"}) {
        const auto *item = control(name);
        rectangles.append(QRectF(item->mapToScene(QPointF()), item->size()));
    }
    return rectangles;
}

void AppearanceUiTest::themeChangesPreserveTypographyAndGeometry()
{
    QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, 140)));
    QTest::qWait(100);
    const QList<qreal> expectedSizes = fontPointSizes();
    const auto originalGeometry = geometry();
    QVERIFY(m_theme->property("omarchyAvailable").toBool());
    for (const auto *mode : {"system", "light", "dark", "omarchy", "system"}) {
        m_theme->setProperty("mode", QString::fromLatin1(mode));
        QTest::qWait(100);
        QCOMPARE(fontPointSizes(), expectedSizes);
        QCOMPARE(m_theme->property("effectiveTextScalePercent").toInt(), 140);
        QCOMPARE(geometry(), originalGeometry);
        for (const auto *name : {"copyTextButton", "deviceSelector", "appearanceButton", "themeSelector",
                                 "refreshButton", "browseButton"}) {
            QCOMPARE(control(name)->property("font").value<QFont>().pointSizeF(),
                     m_theme->property("fontPointSizeBody").toReal());
            QCOMPARE(control(name)->property("font").value<QFont>().family(),
                     m_theme->property("uiFont").toString());
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

void AppearanceUiTest::settingsFollowLiveOmarchyPalette()
{
    // Select Omarchy through the actual standard combo box using the keyboard.
    auto *selector = control("themeSelector");
    selector->forceActiveFocus();
    QTest::keyClick(m_settings, Qt::Key_End);
    QTRY_COMPARE(m_theme->property("mode").toString(), QStringLiteral("omarchy"));

    QFile colors(themeStateRoot + QStringLiteral("/theme/colors.toml"));
    QVERIFY(colors.open(QIODevice::WriteOnly | QIODevice::Truncate));
    const QByteArray fixture("mode = \"light\"\nbackground = \"#f0ead8\"\n"
                             "foreground = \"#203040\"\naccent = \"#704020\"\n"
                             "dark_background = \"#e3dcc8\"\nlighter_background = \"#f7f1e3\"\n"
                             "light_foreground = \"#605944\"\ndark_foreground = \"#766b55\"\n"
                             "selection = \"#e5d7b5\"\nmuted = \"#b8ac91\"\n");
    QCOMPARE(colors.write(fixture), fixture.size());
    colors.close();
    // Let the production file watcher propagate the edit into both UI toolkits.
    QTRY_COMPARE(m_settings->color(), QColor("#f0ead8"));
    QTRY_COMPARE(control("themeFieldLabel")->property("color").value<QColor>(), QColor("#203040"));
    QCOMPARE(QQmlProperty::read(selector, "palette.buttonText").value<QColor>(), QColor("#203040"));
    QCOMPARE(QQmlProperty::read(selector, "palette.highlight").value<QColor>(), QColor("#704020"));
    QCOMPARE(QQmlProperty::read(control("appearanceButton"), "palette.highlight").value<QColor>(), QColor("#e5d7b5"));
    QCOMPARE(QQmlProperty::read(control("exportButton"), "icon.color").value<QColor>(), QColor("#203040"));

    for (const int scale : {80, 200}) {
        QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, scale)));
        QCoreApplication::processEvents();
        for (const auto *name : {"themeFieldLabel", "textSizeFieldLabel"}) {
            const QFont sectionFont = control(name)->property("font").value<QFont>();
            QVERIFY(qAbs(sectionFont.pointSizeF()
                - m_theme->property("fontPointSizeBody").toReal() * 1.15) < 0.01);
            QCOMPARE(sectionFont.weight(), QFont::DemiBold);
        }
        QCOMPARE(control("textSizeDecreaseButton")->isEnabled(), scale > 80);
        QCOMPARE(control("textSizeIncreaseButton")->isEnabled(), scale < 200);
        const QString artifactDir = qEnvironmentVariable("KBEXTRACT_UI_ARTIFACT_DIR");
        if (!artifactDir.isEmpty()) {
            QVERIFY(QDir().mkpath(artifactDir));
            QVERIFY(m_settings->grabWindow().save(artifactDir + QStringLiteral("/settings-%1.png").arg(scale)));
        }
    }
}

void AppearanceUiTest::settingsSidebarNavigation()
{
    auto *appearance = control("appearanceButton");
    auto *exportButton = control("exportButton");
    auto *placeholder = control("exportPlaceholder");
    auto *sectionTitle = control("settingsSectionTitle");
    QVERIFY(sectionTitle);
    QCOMPARE(sectionTitle->property("text").toString(), QStringLiteral("Appearance"));
    QVERIFY(appearance->property("highlighted").toBool());
    QVERIFY(!placeholder->isVisible());

    appearance->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyClick(m_settings, Qt::Key_Down);
    QTRY_VERIFY(exportButton->hasActiveFocus());
    QVERIFY(exportButton->property("visualFocus").toBool());
    QTest::keyClick(m_settings, Qt::Key_Space);
    QTRY_COMPARE(m_settings->property("currentSectionIndex").toInt(), 1);
    QVERIFY(placeholder->isVisible());
    QVERIFY(exportButton->property("highlighted").toBool());
    QVERIFY(!appearance->property("highlighted").toBool());
    QTest::keyClick(m_settings, Qt::Key_Up);
    QTRY_VERIFY(appearance->hasActiveFocus());
    QTest::keyClick(m_settings, Qt::Key_Space);
    QTRY_COMPARE(m_settings->property("currentSectionIndex").toInt(), 0);

    for (const QSize size : {QSize(600, 400), QSize(760, 520)}) {
        m_settings->resize(size);
        for (int scale : {100, 200}) {
            QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, scale)));
            for (const auto *mode : {"system", "light", "dark", "omarchy"}) {
                QVERIFY(QMetaObject::invokeMethod(m_settings, "themeModeSelected", Q_ARG(QString, QString::fromLatin1(mode))));
                QTest::qWait(30);
                for (auto *item : {appearance, exportButton}) {
                    const QRectF bounds(item->mapToScene(QPointF()), item->size());
                    QVERIFY(QRectF(QPointF(), size).contains(bounds));
                    auto *content = item->property("contentItem").value<QQuickItem *>();
                    QVERIFY(content);
                    QVERIFY(item->property("availableWidth").toReal() >= content->implicitWidth());
                    QCOMPARE(item->property("font").value<QFont>().pointSizeF(),
                             m_theme->property("fontPointSizeBody").toReal());
                }
                for (auto *item : {exportButton, appearance}) {
                    QTest::mouseClick(m_settings, Qt::LeftButton, Qt::NoModifier,
                        item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
                    QTRY_COMPARE(m_settings->property("currentSectionIndex").toInt(), item == appearance ? 0 : 1);
                    QCOMPARE(sectionTitle->property("text").toString(), item->property("text").toString());
                    QCOMPARE(placeholder->isVisible(), item == exportButton);
                    QCOMPARE(m_theme->property("mode").toString(), QString::fromLatin1(mode));
                    QCOMPARE(m_theme->property("effectiveTextScalePercent").toInt(), scale);
                    const QString artifactDir = qEnvironmentVariable("KBEXTRACT_UI_ARTIFACT_DIR");
                    if (!artifactDir.isEmpty()) {
                        QVERIFY(QDir().mkpath(artifactDir));
                        QVERIFY(m_settings->grabWindow().save(artifactDir
                            + QStringLiteral("/sidebar-%1-%2-%3-%4.png").arg(QString::fromLatin1(mode))
                                  .arg(size.width()).arg(scale).arg(item == appearance ? "appearance" : "export")));
                    }
                }
            }
        }
    }
}

void AppearanceUiTest::settingsCloseAndReopen()
{
    auto *closeButton = control("settingsCloseButton");
    QVERIFY(closeButton);
    QVERIFY(m_settings->setProperty("currentSectionIndex", 1));
    QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, 140)));

    for (const bool keyboard : {false, true}) {
        if (keyboard) {
            closeButton->forceActiveFocus(Qt::TabFocusReason);
            QTest::keyClick(m_settings, Qt::Key_Space);
        } else {
            QTest::mouseClick(m_settings, Qt::LeftButton, Qt::NoModifier,
                closeButton->mapToScene(QPointF(closeButton->width() / 2, closeButton->height() / 2)).toPoint());
        }
        QTRY_VERIFY(!m_settings->isVisible());
        QVERIFY(m_window->isVisible());
        auto *openButton = control("modeButton");
        QTest::mouseClick(m_window, Qt::LeftButton, Qt::NoModifier,
            openButton->mapToScene(QPointF(openButton->width() / 2, openButton->height() / 2)).toPoint());
        QTRY_VERIFY(m_settings->isVisible());
        QCOMPARE(m_settings->property("currentSectionIndex").toInt(), 1);
        QCOMPARE(m_settings->property("currentTextScalePercent").toInt(), 140);
        QCOMPARE(control("settingsSectionTitle")->property("text").toString(), QStringLiteral("Export"));
    }
}

void AppearanceUiTest::appearanceSubsectionsUseKirigamiSpacing()
{
    auto *form = control("appearanceForm");
    auto *themeSection = control("themeSection");
    auto *themeLabel = control("themeFieldLabel");
    auto *themeSelector = control("themeSelector");
    auto *themeDescription = control("themeDescription");
    auto *sizeLabel = control("textSizeFieldLabel");
    auto *sizeControl = control("textSizeDecreaseButton");
    auto *sizeDescription = control("textSizeDescription");
    QVERIFY(form);
    QVERIFY(themeSection);
    for (auto *item : {themeLabel, themeSelector, themeDescription,
                      sizeLabel, sizeControl, sizeDescription})
        QVERIFY(item);

    QQmlExpression spacingExpression(qmlContext(form), form, QStringLiteral("Kirigami.Units.largeSpacing"));
    const qreal largeSpacing = spacingExpression.evaluate().toReal();
    QVERIFY(!spacingExpression.hasError());
    QVERIFY(largeSpacing > 0);
    const qreal contentMargin = 3 * largeSpacing;
    auto *page = form->parentItem();
    auto *flickable = page->parentItem();
    while (flickable && flickable->metaObject()->indexOfProperty("contentY") < 0)
        flickable = flickable->parentItem();
    QVERIFY(flickable);

    // Include the real desktop's larger font metrics; offscreen Qt defaults to 9 pt.
    QFont desktopFont = m_originalFont;
    if (QFontDatabase::families().contains(QStringLiteral("Adwaita Sans")))
        desktopFont.setFamily(QStringLiteral("Adwaita Sans"));
    desktopFont.setPointSizeF(11.25);
    const auto bounds = [](QQuickItem *item) {
        return QRectF(item->mapToScene(QPointF()), item->size());
    };
    int fixture = 0;
    for (const QFont &font : {m_originalFont, desktopFont}) {
        QGuiApplication::setFont(font);
        for (const QSize size : {QSize(600, 400), QSize(760, 520)}) {
            m_settings->resize(size);
            for (const int scale : {80, 100, 200}) {
                QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, scale)));
                QVERIFY(flickable->setProperty("contentY", 0.0));
                QTest::qWait(30);
                auto *header = control("settingsHeader");
                auto *title = control("settingsSectionTitle");
                auto *closeButton = control("settingsCloseButton");
                QVERIFY(header && title && closeButton);
                const QRectF headerBounds = bounds(header);
                QCOMPARE(headerBounds.top(), 0.0);
                QCOMPARE(headerBounds.left(), bounds(page).left());
                QVERIFY(headerBounds.contains(bounds(title)));
                QVERIFY(headerBounds.contains(bounds(closeButton)));
                QVERIFY(bounds(title).right() <= bounds(closeButton).left());
                QVERIFY(title->width() >= title->implicitWidth());
                QVERIFY(qAbs(bounds(title).left() - bounds(themeLabel).left()) <= 1.0);
                QVERIFY(qAbs(bounds(themeLabel).top() - bounds(page).top() - contentMargin) <= 1.0);
                QVERIFY(qAbs(bounds(themeLabel).left() - bounds(page).left() - contentMargin) <= 1.0);
                QVERIFY(qAbs(bounds(page).right() - bounds(themeSelector).right() - contentMargin) <= 1.0);
                QVERIFY(qAbs(page->implicitHeight() - form->implicitHeight() - 2 * contentMargin) <= 1.0);
                for (auto *item : {themeLabel, themeSelector, themeDescription,
                                  sizeLabel, sizeControl, sizeDescription}) {
                    QVERIFY2(qAbs(bounds(item).left() - bounds(themeLabel).left()) <= 1.0,
                             qPrintable(QStringLiteral("%1 is misaligned at %2% in %3px window: %4 vs section %5")
                                 .arg(item->objectName()).arg(scale).arg(size.width())
                                 .arg(bounds(item).left()).arg(bounds(themeLabel).left())));
                    const QFont itemFont = item->property("font").value<QFont>();
                    QCOMPARE(itemFont.family(), m_theme->property("uiFont").toString());
                    const bool isSectionHeading = item == themeLabel || item == sizeLabel;
                    const qreal expectedPointSize = m_theme->property("fontPointSizeBody").toReal()
                        * (isSectionHeading ? 1.15 : 1.0);
                    QVERIFY(qAbs(itemFont.pointSizeF() - expectedPointSize) < 0.01);
                    QCOMPARE(itemFont.weight(), isSectionHeading ? QFont::DemiBold : QFont::Normal);
                    QVERIFY(bounds(item).right() <= bounds(page).right() - contentMargin + 1.0);
                }
                // Measure visible labels and controls, not FormLayout's outer boxes.
                const qreal sectionGap = bounds(sizeLabel).top() - bounds(themeDescription).bottom();
                QVERIFY(qAbs(sectionGap - 4 * largeSpacing) <= 1.0);
                const qreal smallGap = themeSection->property("spacing").toReal();
                QVERIFY(smallGap > 0 && smallGap < sectionGap);
                for (const auto &pair : {qMakePair(themeLabel, themeSelector),
                                         qMakePair(themeSelector, themeDescription),
                                         qMakePair(sizeLabel, sizeControl),
                                         qMakePair(sizeControl, sizeDescription)}) {
                    const qreal gap = bounds(pair.second).top() - bounds(pair.first).bottom();
                    QVERIFY2(qAbs(gap - smallGap) <= 1.0,
                             qPrintable(QStringLiteral("Unexpected visible gap after %1: %2, expected %3")
                                 .arg(pair.first->objectName()).arg(gap).arg(smallGap)));
                }
                const QString artifactDir = qEnvironmentVariable("KBEXTRACT_UI_ARTIFACT_DIR");
                if (!artifactDir.isEmpty()) {
                    QVERIFY(QDir().mkpath(artifactDir));
                    QVERIFY(m_settings->grabWindow().save(artifactDir
                        + QStringLiteral("/alignment-font%1-%2-%3.png").arg(fixture).arg(size.width()).arg(scale)));
                }
                const qreal maximumContentY = qMax(0.0, flickable->property("contentHeight").toReal() - flickable->height());
                QVERIFY(flickable->setProperty("contentY", maximumContentY));
                QCOMPARE(bounds(header), headerBounds);
                QTRY_VERIFY(bounds(sizeDescription).top() >= bounds(flickable).top() - 1.0);
                QTRY_VERIFY(bounds(sizeDescription).bottom() <= bounds(flickable).bottom() - contentMargin + 1.0);
            }
        }
        ++fixture;
    }
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
        QTRY_COMPARE(QRectF(footer->mapToScene(QPointF()), footer->size()).bottom(),
                     QRectF(mainColumn->mapToScene(QPointF()), mainColumn->size()).bottom());
        const QRectF footerBounds(footer->mapToScene(QPointF()), footer->size());
        QCOMPARE(sidebarBounds.bottom(), mainBounds.bottom());
        QCOMPARE(footerBounds.left(), mainBounds.left());
        QCOMPARE(footerBounds.right(), mainBounds.right());
        QCOMPARE(footerBounds.bottom(), mainBounds.bottom());
        QCOMPARE(footerBounds.left(), sidebarBounds.right());

        const int verticalSpace = 2 * m_theme->property("controlVerticalPadding").toInt();
        for (const auto *name : {"copyTextButton", "copyObsidianButton", "copyAllButton",
                                 "deviceSelector", "appearanceButton", "themeSelector",
                                 "textSizeDecreaseButton", "textSizeResetButton",
                                 "textSizeIncreaseButton", "refreshButton", "browseButton"}) {
            auto *item = control(name);
            auto *content = item->property("contentItem").value<QQuickItem *>();
            QVERIFY(content);
            const bool settingsControl = item->window() == m_settings;
            const qreal padding = settingsControl
                ? item->property("topPadding").toReal() + item->property("bottomPadding").toReal()
                : verticalSpace;
            if (!settingsControl)
                QVERIFY(item->height() >= m_theme->property("controlMinHeight").toInt());
            QVERIFY2(item->height() >= content->implicitHeight() + padding,
                     qPrintable(QStringLiteral("%1 has insufficient vertical padding at %2%: "
                                               "height %3, implicit %4, content %5, padding %6")
                         .arg(QString::fromLatin1(name)).arg(scale)
                         .arg(item->height()).arg(item->implicitHeight())
                         .arg(content->implicitHeight())
                         .arg(verticalSpace)));
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
            auto *content = row->property("contentItem").value<QQuickItem *>();
            QVERIFY(content);
            QVERIFY(row->height() >= content->implicitHeight()
                + row->property("topPadding").toReal() + row->property("bottomPadding").toReal());
        }
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTRY_VERIFY(!popup->property("visible").toBool());
    }
}

void AppearanceUiTest::chromeFollowsBodyMetricsAndFitsSidebar()
{
    QFont fixture = QGuiApplication::font();
    fixture.setPointSizeF(11.0);
    QGuiApplication::setFont(fixture);
    const qreal resolvedSize = QFontInfo(QGuiApplication::font()).pointSizeF();
    QTRY_COMPARE(m_theme->property("systemFontPointSize").toReal(), resolvedSize);

    QObject *qmlMetrics = m_theme->property("bodyMetrics").value<QObject *>();
    QVERIFY(qmlMetrics);
    const qreal bodyHeight = qmlMetrics->property("height").toReal();
    const qreal bodyDescent = qmlMetrics->property("descent").toReal();
    QVERIFY(bodyHeight > 0.0);
    QCOMPARE(m_theme->property("controlVerticalPadding").toInt(),
             qMax(3, qRound(bodyDescent)));
    QVERIFY(m_theme->property("controlHorizontalPadding").toInt() >= 1);
    QCOMPARE(m_theme->property("controlMinHeight").toInt(),
             qMax(24, qRound(bodyHeight)
                      + 2 * m_theme->property("controlVerticalPadding").toInt()));
    QVERIFY(m_theme->property("controlMinHeight").toInt() >= 24);

    auto *deviceLabel = control("deviceLabel");
    auto *booksLabel = control("booksLabel");
    QCOMPARE(deviceLabel->property("text").toString(), QStringLiteral("Device"));
    QCOMPARE(booksLabel->property("text").toString(), QStringLiteral("Books"));
    for (auto *label : {deviceLabel, booksLabel}) {
        const QFont font = label->property("font").value<QFont>();
        QCOMPARE(font.family(), m_theme->property("uiFont").toString());
        QCOMPARE(font.pointSizeF(), m_theme->property("fontPointSizeBody").toReal());
        QCOMPARE(font.weight(), static_cast<int>(QFont::DemiBold));
        QCOMPARE(font.letterSpacing(), 0.0);
    }

    QCOMPARE(control("bookCount")->property("font").value<QFont>().family(),
             m_theme->property("monoFont").toString());
    QCOMPARE(control("bookCount")->property("font").value<QFont>().pointSizeF(),
             m_theme->property("fontPointSizeCaption").toReal());
    QCOMPARE(control("annotationText")->property("font").value<QFont>().family(),
             m_theme->property("monoFont").toString());

    auto *modeButton = control("modeButton");
    QCOMPARE(modeButton->implicitHeight(), m_theme->property("controlMinHeight").toInt());
    QCOMPARE(modeButton->implicitWidth(), m_theme->property("controlMinHeight").toInt() + 6);
    QCOMPARE(QQmlProperty::read(modeButton, QStringLiteral("icon.width")).toInt(),
             m_theme->property("controlIconSize").toInt());
    QCOMPARE(QQmlProperty::read(modeButton, QStringLiteral("icon.height")).toInt(),
             m_theme->property("controlIconSize").toInt());

    for (const int scale : {100, 150}) {
        QVERIFY(QMetaObject::invokeMethod(m_settings, "textScalePercentSelected", Q_ARG(int, scale)));
        QTest::qWait(30);

        auto *combo = control("deviceSelector");
        auto *refresh = control("refreshButton");
        auto *browse = control("browseButton");
        for (auto *item : {combo, refresh, browse, control("copyTextButton")}) {
            QCOMPARE(item->property("font").value<QFont>().family(),
                     m_theme->property("uiFont").toString());
            QCOMPARE(item->property("font").value<QFont>().pointSizeF(),
                     m_theme->property("fontPointSizeBody").toReal());
        }
        QVERIFY(qAbs(combo->height() - refresh->height()) <= 1.0);
        QVERIFY(qAbs(combo->height() - browse->height()) <= 1.0);
        QVERIFY(qAbs(refresh->height() - browse->height()) <= 1.0);

        auto *sidebar = control("sidebar");
        QCOMPARE(sidebar->width(), 246);
        const QRectF sidebarBounds(sidebar->mapToScene(QPointF()), sidebar->size());
        for (auto *item : {refresh, browse, combo, control("deviceLabel")}) {
            const QRectF bounds(item->mapToScene(QPointF()), item->size());
            QVERIFY2(sidebarBounds.contains(bounds),
                     qPrintable(QStringLiteral("%1 is outside the sidebar at %2%: %3,%4 %5x%6")
                         .arg(item->objectName()).arg(scale)
                         .arg(bounds.x()).arg(bounds.y()).arg(bounds.width()).arg(bounds.height())));
            auto *content = item->property("contentItem").value<QQuickItem *>();
            if (content) {
                QVERIFY2(item->property("availableWidth").toReal() >= content->implicitWidth(),
                         qPrintable(QStringLiteral("%1 overflows its content at %2%: available %3, content %4")
                             .arg(item->objectName()).arg(scale)
                             .arg(item->property("availableWidth").toReal())
                             .arg(content->implicitWidth())));
            }
        }
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
