#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>

#include "clipboard_helper.h"
#include "kobo_library.h"
#include "markdown_highlighter.h"
#include "omarchy_theme.h"
#include "system_appearance.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setOrganizationName(QStringLiteral("kbextract"));
    app.setApplicationName(QStringLiteral("kbextract"));
    app.setApplicationDisplayName(QStringLiteral("kbextract"));
    app.setApplicationVersion(QStringLiteral(KBEXTRACT_VERSION));
    app.setOrganizationDomain(QStringLiteral("314r.github.io"));
    app.setDesktopFileName(QStringLiteral("io.github._314r.kbextract"));
    app.setWindowIcon(QIcon(QStringLiteral(":/qt/qml/Kbextract/assets/icons/kbextract.svg")));

    qmlRegisterType<ClipboardHelper>("Kbextract", 1, 0, "ClipboardHelper");
    qmlRegisterType<KoboLibrary>("Kbextract", 1, 0, "KoboLibrary");
    qmlRegisterType<MarkdownHighlighter>("Kbextract", 1, 0, "MarkdownHighlighter");
    qmlRegisterType<OmarchyTheme>("Kbextract", 1, 0, "OmarchyTheme");
    qmlRegisterType<SystemAppearance>("Kbextract", 1, 0, "SystemAppearance");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Kbextract", "Main");

    return app.exec();
}
