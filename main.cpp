#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "kobo_library.h"
#include "markdown_highlighter.h"
#include "omarchy_theme.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setOrganizationName(QStringLiteral("kbextract"));
    app.setApplicationName(QStringLiteral("kbextract"));
    app.setApplicationDisplayName(QStringLiteral("kbextract"));

    qmlRegisterType<KoboLibrary>("Kbextract", 1, 0, "KoboLibrary");
    qmlRegisterType<MarkdownHighlighter>("Kbextract", 1, 0, "MarkdownHighlighter");
    qmlRegisterType<OmarchyTheme>("Kbextract", 1, 0, "OmarchyTheme");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Kbextract", "Main");

    return app.exec();
}
