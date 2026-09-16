#include <QtTest>

#include <QClipboard>
#include <QDir>
#include <QGuiApplication>
#include <QPalette>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QStyleHints>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include <memory>

#include "clipboard_helper.h"
#include "file_url.h"
#include "kobo_library.h"
#include "markdown_highlighter.h"
#include "system_appearance.h"

namespace {
QList<KoboVolume> mountedVolumes;

class TestLibrary : public KoboLibrary
{
public:
    explicit TestLibrary(QObject *parent = nullptr)
        : KoboLibrary([] { return mountedVolumes; }, 60000, parent) {}
};

void registerTypes()
{
    qmlRegisterType<TestLibrary>("Kbextract", 1, 0, "KoboLibrary");
    qmlRegisterType<ClipboardHelper>("Kbextract", 1, 0, "ClipboardHelper");
    qmlRegisterType<FileUrl>("Kbextract", 1, 0, "FileUrl");
    qmlRegisterType<MarkdownHighlighter>("Kbextract", 1, 0, "MarkdownHighlighter");
    qmlRegisterType<SystemAppearance>("Kbextract", 1, 0, "SystemAppearance");
    for (const auto *name : {"AppSession", "AnnotationBody", "MacCopyButton"}) {
        qmlRegisterType(QUrl(QStringLiteral("qrc:/qt/qml/Kbextract/%1.qml").arg(name)),
                        "Kbextract", 1, 0, name);
    }
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/qt/qml/Kbextract/Theme.qml")),
                            "Kbextract", 1, 0, "Theme");
}

bool createFixture(const QString &root)
{
    if (!QDir().mkpath(root + "/.kobo"))
        return false;
    bool ok;
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", "mac-ui-fixture");
        db.setDatabaseName(root + "/.kobo/KoboReader.sqlite");
        if (!db.open())
            return false;
        QSqlQuery query(db);
        ok = query.exec("CREATE TABLE content (ContentID TEXT, BookID TEXT, Title TEXT, Attribution TEXT, "
                        "ContentType INTEGER, VolumeIndex INTEGER, ChapterIDBookmarked TEXT)")
            && query.exec("CREATE TABLE Bookmark (BookmarkID TEXT, VolumeID TEXT, ContentID TEXT, Text TEXT, "
                          "Annotation TEXT, Hidden TEXT, StartContainerChildIndex INTEGER, StartOffset INTEGER, ChapterProgress REAL)")
            && query.exec("INSERT INTO content VALUES ('one', NULL, 'A very long book title that will be elided in the sidebar', "
                          "'A very long author name for layout coverage', 6, 0, NULL), "
                          "('two', NULL, 'Second book', 'Another author', 6, 0, NULL)");
        query.prepare("INSERT INTO Bookmark VALUES ('highlight', 'one', 'chapter', ?, NULL, 'false', 1, 1, 0.1)");
        query.addBindValue(QStringLiteral("A highlighted passage with enough text to exercise scrolling.\n\n").repeated(200));
        ok = ok && query.exec()
            && query.exec("INSERT INTO Bookmark VALUES ('note', 'two', 'chapter', 'Second passage', 'A note', 'false', 1, 1, 0.1)");
        db.close();
    }
    QSqlDatabase::removeDatabase("mac-ui-fixture");
    return ok;
}
} // namespace

class MacUiTest : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void ignoresLegacyAppearance_data();
    void ignoresLegacyAppearance();
    void commandsAndCopyFeedback();
    void selectionCopyAndNavigation();
    void disconnectClearsCommands();
    void sidebarAndMinimumWindow();
    void opensFileUrl();
    void closeExitsApplication();

private:
    QObject *object(const char *name) const;
    QQuickItem *item(const char *name) const;
    void trigger(const char *name);
    void mountFixtures();

    QTemporaryDir m_devices;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
    QQuickWindow *m_window = nullptr;
    KoboLibrary *m_library = nullptr;
    QStringList m_warnings;
};

QObject *MacUiTest::object(const char *name) const
{
    return m_window->findChild<QObject *>(QString::fromLatin1(name));
}

QQuickItem *MacUiTest::item(const char *name) const
{
    return qobject_cast<QQuickItem *>(object(name));
}

void MacUiTest::trigger(const char *name)
{
    auto *action = object(name);
    QVERIFY2(action, name);
    QVERIFY(QMetaObject::invokeMethod(action, "trigger"));
    QCoreApplication::processEvents();
}

void MacUiTest::init()
{
    mountedVolumes.clear();
    QSettings().clear();
    const QString savedMode = QTest::currentDataTag() && *QTest::currentDataTag()
        ? QString::fromUtf8(QTest::currentDataTag()) : QStringLiteral("dark");
    QSettings().setValue("Appearance/colorMode", savedMode);
    m_warnings.clear();
    m_engine = std::make_unique<QQmlApplicationEngine>();
    connect(m_engine.get(), &QQmlEngine::warnings, this, [this](const QList<QQmlError> &errors) {
        for (const auto &error : errors)
            m_warnings.append(error.toString());
    });
    m_engine->load(QUrl("qrc:/qt/qml/Kbextract/MacMain.qml"));
    QVERIFY2(!m_engine->rootObjects().isEmpty(), qPrintable(m_warnings.join('\n')));
    m_window = qobject_cast<QQuickWindow *>(m_engine->rootObjects().first());
    QVERIFY(m_window);
    m_library = m_window->findChild<KoboLibrary *>();
    QVERIFY(m_library);
    m_window->resize(1040, 680);
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    for (const auto *name : {"copyTextButton", "copyObsidianButton", "copyAllButton", "deviceSelector",
                             "annotationText", "bookList", "sidebar", "mainColumn", "copyFooter"})
        QVERIFY2(item(name), name);
}

void MacUiTest::cleanup()
{
    m_engine.reset();
    m_window = nullptr;
    m_library = nullptr;
    mountedVolumes.clear();
    QVERIFY2(m_warnings.isEmpty(), qPrintable(m_warnings.join('\n')));
}

void MacUiTest::mountFixtures()
{
    // Each test gets fresh mount directories; never consult actual volumes.
    const QString first = m_devices.filePath(QUuid::createUuid().toString());
    const QString second = m_devices.filePath(QUuid::createUuid().toString());
    QVERIFY(createFixture(first));
    QVERIFY(createFixture(second));
    mountedVolumes = {{first, "First Kobo with a long device name"}, {second, "Second Kobo"}};
    trigger("refreshAction");
    QCOMPARE(m_library->devices().size(), 2);
    QCOMPARE(m_library->books().size(), 2);
    m_library->setCurrentBookIndex(0);
    QCoreApplication::processEvents();
}

void MacUiTest::ignoresLegacyAppearance_data()
{
    QTest::addColumn<QString>("savedMode");
    QTest::newRow("light") << QStringLiteral("light");
    QTest::newRow("dark") << QStringLiteral("dark");
    QTest::newRow("omarchy") << QStringLiteral("omarchy");
}

void MacUiTest::ignoresLegacyAppearance()
{
    QFETCH(QString, savedMode);
    auto *theme = m_engine->singletonInstance<QObject *>("Kbextract", "Theme");
    QVERIFY(theme);
    QCOMPARE(theme->property("mode").toString(), "system");
    QCOMPARE(QSettings().value("Appearance/colorMode").toString(), savedMode);
    QVERIFY(!object("settingsWindow"));
    QVERIFY(!object("modeButton"));
    for (const auto *name : {"copyTextAction", "copyObsidianAction", "copyMarkdownAction",
                             "copySelectionAction", "selectAllAction"})
        QVERIFY(!object(name)->property("enabled").toBool());

    const auto original = QGuiApplication::palette();
    QPalette changed = original;
    changed.setColor(QPalette::Base, QColor("#17212c"));
    changed.setColor(QPalette::Text, QColor("#e3e7ed"));
    changed.setColor(QPalette::Highlight, QColor("#af60df"));
    QGuiApplication::setPalette(changed);
    QTRY_COMPARE(item("annotationText")->property("color").value<QColor>(), QColor("#e3e7ed"));
    QCOMPARE(item("annotationText")->property("selectionColor").value<QColor>(), QColor("#af60df"));
    QGuiApplication::setPalette(original);
}

void MacUiTest::commandsAndCopyFeedback()
{
    mountFixtures();
    const QStringList actions{"copyTextAction", "copyObsidianAction", "copyMarkdownAction"};
    const QStringList buttons{"copyTextButton", "copyObsidianButton", "copyAllButton"};
    const QStringList labels{"COPY TEXT", "COPY OBS MD", "COPY MD"};
    const QStringList expected{m_library->currentBookPlainText(), m_library->currentBookObsidianMarkdown(),
                               m_library->currentBookMarkdown()};
    for (int i = 0; i < actions.size(); ++i) {
        auto *button = item(qPrintable(buttons[i]));
        QTest::qWait(20);
        QCOMPARE(button->property("text").toString(), labels[i]);
        const QSizeF size = button->size();
        trigger(qPrintable(actions[i]));
        QCOMPARE(QGuiApplication::clipboard()->text(), expected[i]);
        QVERIFY(button->property("confirmed").toBool());
        QCOMPARE(button->property("text").toString(), QStringLiteral("COPIED"));
        QTest::qWait(20);
        QCOMPARE(button->size(), size);
        QTRY_VERIFY_WITH_TIMEOUT(!button->property("confirmed").toBool(), 2000);
        QCOMPARE(button->property("text").toString(), labels[i]);
        QCOMPARE(button->size(), size);
        QTest::mouseClick(m_window, Qt::LeftButton, Qt::NoModifier,
                         button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
        QCOMPARE(QGuiApplication::clipboard()->text(), expected[i]);
        QVERIFY(button->property("confirmed").toBool());
    }
    m_library->setCurrentBookIndex(1);
    for (const auto &name : buttons)
        QVERIFY(!item(qPrintable(name))->property("confirmed").toBool());
}

void MacUiTest::selectionCopyAndNavigation()
{
    mountFixtures();
    auto *reader = item("annotationText");
    reader->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(reader, "select", Q_ARG(int, 0), Q_ARG(int, 12)));
    const QString selected = reader->property("selectedText").toString();
    QVERIFY(!selected.isEmpty());
    trigger("copySelectionAction");
    QCOMPARE(QGuiApplication::clipboard()->text(), selected);
    QGuiApplication::clipboard()->clear();
    QTest::keyClick(m_window, Qt::Key_C, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::clipboard()->text(), selected);
    trigger("selectAllAction");
    QCOMPARE(reader->property("selectedText").toString(), m_library->currentBookMarkdown());

    auto *scroll = item("annotationScroll");
    auto *flickable = scroll->property("contentItem").value<QObject *>();
    QVERIFY(flickable);
    flickable->setProperty("contentY", 300);
    item("bookList")->forceActiveFocus();
    QTest::keyClick(m_window, Qt::Key_Down);
    QCOMPARE(m_library->currentBookIndex(), 1);
    QVERIFY(reader->property("selectedText").toString().isEmpty());
    QTRY_COMPARE(flickable->property("contentY").toReal(), flickable->property("originY").toReal());
    QTest::keyClick(m_window, Qt::Key_Up);
    QCOMPARE(m_library->currentBookIndex(), 0);

    QVERIFY(QMetaObject::invokeMethod(item("deviceSelector"), "activated", Q_ARG(int, 1)));
    QCOMPARE(m_library->currentDeviceIndex(), 1);
    QCOMPARE(m_library->currentBookIndex(), -1);
}

void MacUiTest::disconnectClearsCommands()
{
    mountFixtures();
    mountedVolumes.clear();
    trigger("refreshAction");
    QVERIFY(m_library->devices().isEmpty());
    QCOMPARE(m_library->currentBookIndex(), -1);
    QVERIFY(!object("copyMarkdownAction")->property("enabled").toBool());
    QVERIFY(!item("deviceSelector")->isEnabled());
    QVERIFY(!item("annotationScroll")->isVisible());
}

void MacUiTest::sidebarAndMinimumWindow()
{
    mountFixtures();
    QTest::qWait(30);
    QCOMPARE(item("sidebar")->width(), 290);
    QVERIFY(item("annotationText")->width() > 400);
    QVERIFY(item("annotationText")->height() > item("annotationScroll")->height());
    QVERIFY(item("annotationText")->isVisible());
    for (const auto *name : {"copyTextButton", "copyObsidianButton", "copyAllButton", "deviceSelector"}) {
        auto *control = item(name);
        const QRectF bounds(control->mapToScene(QPointF()), control->size());
        QVERIFY(QRectF(0, 0, m_window->width(), m_window->height()).contains(bounds));
    }
    // Drag the real split handle to both limits, then verify the persisted size.
    auto *sidebar = item("sidebar");
    auto *mainColumn = item("mainColumn");
    auto *footer = item("copyFooter");
    auto *divider = item("sidebarDivider");
    QVERIFY(divider);
    const QRectF sidebarBounds(sidebar->mapToScene(QPointF()), sidebar->size());
    const QRectF mainBounds(mainColumn->mapToScene(QPointF()), mainColumn->size());
    const QRectF footerBounds(footer->mapToScene(QPointF()), footer->size());
    QCOMPARE(sidebarBounds.bottom(), mainBounds.bottom());
    QCOMPARE(footerBounds.left(), mainBounds.left());
    QCOMPARE(footerBounds.right(), mainBounds.right());
    QCOMPARE(footerBounds.bottom(), mainBounds.bottom());
    for (const auto *name : {"copyTextButton", "copyObsidianButton", "copyAllButton"}) {
        auto *button = item(name);
        const QRectF bounds(button->mapToItem(footer, QPointF()), button->size());
        QVERIFY(QRectF(QPointF(), footer->size()).contains(bounds));
    }
    QCOMPARE(divider->width(), 1);
    QCOMPARE(divider->height(), sidebar->height());
    const QColor dividerColor = divider->property("color").value<QColor>();
    // Start outside the visible line to exercise its wider containment mask.
    const QPoint handle = divider->mapToScene(QPointF(divider->width() / 2 + 2, 200)).toPoint();
    QTest::mouseMove(m_window, handle);
    QCOMPARE(divider->property("color").value<QColor>(), dividerColor);
    QTest::mousePress(m_window, Qt::LeftButton, Qt::NoModifier, handle);
    QTest::mouseMove(m_window, QPoint(700, handle.y()), 30);
    QCOMPARE(divider->property("color").value<QColor>(), dividerColor);
    QTest::mouseRelease(m_window, Qt::LeftButton, Qt::NoModifier, QPoint(700, handle.y()));
    QTRY_COMPARE(sidebar->width(), 420);
    QTRY_COMPARE(divider->mapToScene(QPointF()).x(),
                 sidebar->mapToScene(QPointF(sidebar->width(), 0)).x());
    const QPoint nextHandle = divider->mapToScene(QPointF(divider->width() / 2 - 2, 200)).toPoint();
    QTest::mouseMove(m_window, nextHandle);
    QTest::mousePress(m_window, Qt::LeftButton, Qt::NoModifier, nextHandle);
    QTest::mouseMove(m_window, QPoint(100, nextHandle.y()), 30);
    QTest::mouseRelease(m_window, Qt::LeftButton, Qt::NoModifier, QPoint(100, nextHandle.y()));
    QTRY_COMPARE(sidebar->width(), 246);
    QTest::qWait(30);
    for (const auto *name : {"deviceSelector", "refreshButton", "openButton"}) {
        auto *control = item(name);
        const QRectF bounds(control->mapToItem(sidebar, QPointF()), control->size());
        QVERIFY(QRectF(0, 0, sidebar->width(), sidebar->height()).contains(bounds));
    }

    const QString artifactDir = qEnvironmentVariable("KBEXTRACT_UI_ARTIFACT_DIR");
    if (!artifactDir.isEmpty()) {
        QVERIFY(QDir().mkpath(artifactDir));
        QVERIFY(m_window->grabWindow().save(artifactDir + "/mac-main.png"));
        // App-local overrides only: do not change the user's macOS appearance.
        const auto originalScheme = QGuiApplication::styleHints()->colorScheme();
        const auto restoreScheme = qScopeGuard([originalScheme] {
            QGuiApplication::styleHints()->setColorScheme(originalScheme);
        });
        for (const auto scheme : {Qt::ColorScheme::Light, Qt::ColorScheme::Dark}) {
            QGuiApplication::styleHints()->setColorScheme(scheme);
            QTest::qWait(100);
            const QString suffix = scheme == Qt::ColorScheme::Dark ? "dark" : "light";
            QVERIFY(m_window->grabWindow().save(artifactDir + "/mac-main-" + suffix + ".png"));
        }
    }
    m_engine.reset();
    m_window = nullptr;
    QSettings().sync();
    QCOMPARE(QSettings().value("MacWindow/sidebarWidth").toReal(), 246);
    QCOMPARE(QSettings().value("Appearance/colorMode").toString(), "dark");
}

void MacUiTest::opensFileUrl()
{
    const QString root = m_devices.filePath(QString::fromUtf8("manual database #100% é"));
    QVERIFY(createFixture(root));
    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(object("session"), "openDatabase", Q_RETURN_ARG(QVariant, result),
        Q_ARG(QVariant, QUrl::fromLocalFile(root + "/.kobo/KoboReader.sqlite"))));
    QVERIFY(result.toBool());
    QCOMPARE(m_library->devices().size(), 1);
}

void MacUiTest::closeExitsApplication()
{
    QProcess child;
    child.start(QCoreApplication::applicationFilePath(), {"--close-probe"});
    QVERIFY(child.waitForStarted());
    QVERIFY2(child.waitForFinished(5000), child.readAll().constData());
    QCOMPARE(child.exitStatus(), QProcess::NormalExit);
    QCOMPARE(child.exitCode(), 0);
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QTemporaryDir settings;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    app.setOrganizationName("kbextract-tests");
    app.setApplicationName("mac-ui");
    registerTypes();
    if (app.arguments().contains("--close-probe")) {
        QQmlApplicationEngine engine;
        engine.load(QUrl("qrc:/qt/qml/Kbextract/MacMain.qml"));
        if (engine.rootObjects().isEmpty())
            return 2;
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QTimer::singleShot(100, window, [window] {
            QMetaObject::invokeMethod(window->findChild<QObject *>("closeAction"), "trigger");
        });
        QTimer::singleShot(3000, &app, [] { QCoreApplication::exit(3); });
        return app.exec();
    }
    app.setQuitOnLastWindowClosed(false);
    MacUiTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "mac_ui_test.moc"
