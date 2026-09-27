#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>

#include "kobo_library.h"

namespace {

bool execute(QSqlDatabase &database, const QString &statement)
{
    QSqlQuery query(database);
    if (query.exec(statement))
        return true;
    qWarning().noquote() << query.lastError().text() << statement;
    return false;
}

bool createDatabase(const QString &path, bool withKoboSchema = true)
{
    const QString connectionName = QStringLiteral("kbextract-test-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    bool succeeded = false;

    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(path);
        if (!database.open())
            return false;

        if (!withKoboSchema) {
            succeeded = execute(database, QStringLiteral("CREATE TABLE unrelated (value TEXT)"));
        } else {
            succeeded = execute(database, QStringLiteral(
                "CREATE TABLE content ("
                "ContentID TEXT, BookID TEXT, Title TEXT, Attribution TEXT, "
                "ContentType INTEGER, VolumeIndex INTEGER, ChapterIDBookmarked TEXT)"))
                && execute(database, QStringLiteral(
                    "CREATE TABLE Bookmark ("
                    "BookmarkID TEXT, VolumeID TEXT, ContentID TEXT, Text TEXT, Annotation TEXT, "
                    "Hidden TEXT, StartContainerChildIndex INTEGER, StartOffset INTEGER, ChapterProgress REAL)"))
                && execute(database, QStringLiteral(
                    "INSERT INTO content VALUES "
                    "('book-alpha', NULL, 'Alpha', 'Author A', 6, 0, NULL), "
                    "('book-beta', NULL, 'beta', 'Author B', 6, 0, NULL), "
                    "('book-empty', NULL, 'Empty', 'Author C', 6, 0, NULL), "
                    "('book-split', NULL, 'Le labyrinthe des égarés', 'Amin Maalouf', 6, -1, NULL), "
                    "('chapter-1', 'book-alpha', 'Chapter' || char(10) || 'One', NULL, 9, 1, NULL), "
                    "('chapter-2', 'book-alpha', 'Chapter Two', NULL, 9, 2, NULL), "
                    "('book-beta!opf!chapter-2-1', 'book-beta', 'Beta Chapter', NULL, 899, 2, "
                        "'book-beta!opf!chapter-2'), "
                    "('book-split!OPS!p1.xhtml', 'book-split', 'p1.xhtml', NULL, 9, 4, NULL), "
                    "('book-split!OPS!p1chap8.xhtml', 'book-split', 'p1chap8.xhtml', NULL, 9, 12, NULL), "
                    "('book-split!OPS!p1chap10.xhtml', 'book-split', 'p1chap10.xhtml', NULL, 9, 14, NULL), "
                    "('book-split!OPS!p3.xhtml', 'book-split', 'p3.xhtml', NULL, 9, 26, NULL), "
                    "('book-split!OPS!p3chap11.xhtml', 'book-split', 'p3chap11.xhtml', NULL, 9, 37, NULL), "
                    "('book-split!OPS!p5.xhtml', 'book-split', 'p5.xhtml', NULL, 9, 47, NULL), "
                    "('book-split!OPS!p5chap3.xhtml', 'book-split', 'p5chap3.xhtml', NULL, 9, 50, NULL), "
                    "('book-split!OPS!p5chap4.xhtml', 'book-split', 'p5chap4.xhtml', NULL, 9, 51, NULL), "
                    "('book-split!OPS!p1.xhtml-1', 'book-split', 'I.' || char(8194) || 'Les étincelles japonaises', NULL, 899, 4, "
                        "'book-split!OPS!p1.xhtml'), "
                    "('book-split!OPS!p3.xhtml-1', 'book-split', 'III.' || char(8194) || 'Une si longue marche', NULL, 899, 6, "
                        "'book-split!OPS!p3.xhtml'), "
                    "('book-split!OPS!p5.xhtml-1', 'book-split', 'Épilogue.' || char(8194) || 'Un monde à reconstruire', NULL, 899, 8, "
                        "'book-split!OPS!p5.xhtml')"))
                && execute(database, QStringLiteral(
                    "INSERT INTO Bookmark VALUES "
                    "('later-highlight', 'book-alpha', 'chapter-2', 'Later text', NULL, 'false', 1, 4, 0.2), "
                    "('special-highlight', 'book-alpha', 'chapter-1', "
                        "'Stars *stay* & <tag>' || char(10) || 'second line' || char(10) || char(10) "
                        "|| 'Another paragraph' || char(10) || 'continues here', NULL, 'false', 1, 5, 0.1), "
                    "('note', 'book-alpha', 'chapter-1', 'Selected note text', "
                        "'Written note' || char(10) || 'second note line', 'false', 1, 8, 0.2), "
                    "('hidden', 'book-alpha', 'chapter-1', 'Hidden text', NULL, 'true', 1, 12, 0.3), "
                    "('dog-ear', 'book-alpha', 'chapter-1', 'Chapter bookmark', NULL, 'false', 0, 0, 0.4), "
                    "('annotation-only', 'book-beta', 'book-beta!opf!chapter-2', NULL, 'Margin note', '0', 2, 3, 0.1), "
                    "('split-p1-later', 'book-split', 'book-split!OPS!p1chap10.xhtml', "
                        "'Second part-one highlight', NULL, 'false', 1, 2, 0.1), "
                    "('split-p1-earlier', 'book-split', 'book-split!OPS!p1chap8.xhtml', "
                        "'First part-one highlight', NULL, 'false', 1, 2, 0.1), "
                    "('split-p3', 'book-split', 'book-split!OPS!p3chap11.xhtml', "
                        "'Part-three highlight', NULL, 'false', 1, 2, 0.1), "
                    "('split-p5-first', 'book-split', 'book-split!OPS!p5chap3.xhtml', "
                        "'First epilogue highlight', NULL, 'false', 1, 2, 0.1), "
                    "('split-p5-second', 'book-split', 'book-split!OPS!p5chap4.xhtml', "
                        "'Second epilogue highlight', NULL, 'false', 1, 2, 0.1), "
                    "('fallback-title', 'missing-volume', 'chapter-3', 'Orphaned highlight', NULL, 'false', 3, 2, 0.1), "
                    "('blank', 'book-empty', 'chapter-4', '   ', '   ', 'false', 4, 2, 0.1)"));
        }

        database.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
    return succeeded;
}

QVariantMap bookWithVolumeId(const QVariantList &books, const QString &volumeId)
{
    for (const QVariant &bookValue : books) {
        const QVariantMap book = bookValue.toMap();
        if (book.value(QStringLiteral("volumeId")).toString() == volumeId)
            return book;
    }
    return {};
}

int devicePathOccurrences(const QVariantList &devices, const QString &databasePath)
{
    const QString canonicalPath = QFileInfo(databasePath).canonicalFilePath();
    int count = 0;
    for (const QVariant &deviceValue : devices) {
        const QString candidate = QFileInfo(deviceValue.toMap().value(QStringLiteral("databasePath")).toString()).canonicalFilePath();
        if (candidate == canonicalPath)
            ++count;
    }
    return count;
}

int bookIndexWithVolumeId(const QVariantList &books, const QString &volumeId)
{
    for (int index = 0; index < books.size(); ++index) {
        if (books.at(index).toMap().value(QStringLiteral("volumeId")).toString() == volumeId)
            return index;
    }
    return -1;
}

QString libraryStorePath(const QTemporaryDir &directory)
{
    return directory.filePath(QStringLiteral("library.sqlite"));
}

bool alterDatabase(const QString &path, const QStringList &statements)
{
    const QString connectionName = QStringLiteral("kbextract-test-alter-%1")
                                       .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    bool succeeded = false;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setDatabaseName(path);
        succeeded = database.open();
        for (const QString &statement : statements)
            succeeded = succeeded && execute(database, statement);
        database.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    return succeeded;
}

bool writeSerialNumber(const QString &databasePath, const QString &serial)
{
    const QDir databaseDirectory = QFileInfo(databasePath).dir();
    if (databaseDirectory.dirName() != QLatin1String(".kobo"))
        return false;
    if (!QDir().mkpath(databaseDirectory.filePath(QStringLiteral("Kobo"))))
        return false;

    QFile conf(databaseDirectory.filePath(QStringLiteral("Kobo/Kobo eReader.conf")));
    if (!conf.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const QByteArray body = QStringLiteral("[DeviceInfo]\nSerialNumber=%1\n").arg(serial).toUtf8();
    return conf.write(body) == body.size();
}

int deviceIndexForPath(const QVariantList &devices, const QString &databasePath)
{
    const QString canonicalPath = QFileInfo(databasePath).canonicalFilePath();
    for (int index = 0; index < devices.size(); ++index) {
        const QString candidate = QFileInfo(devices.at(index).toMap().value(QStringLiteral("databasePath")).toString()).canonicalFilePath();
        if (!candidate.isEmpty() && candidate == canonicalPath)
            return index;
    }
    return -1;
}

int deviceIndexForKey(const QVariantList &devices, const QString &deviceKey)
{
    for (int index = 0; index < devices.size(); ++index) {
        if (devices.at(index).toMap().value(QStringLiteral("deviceKey")).toString() == deviceKey)
            return index;
    }
    return -1;
}

QString mountedDatabasePath(const QString &mountPath)
{
    return QDir(mountPath).filePath(QStringLiteral(".kobo/KoboReader.sqlite"));
}

} // namespace

class KoboLibraryTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsAnnotatedBooks();
    void formatsSelectedBookAsMarkdown();
    void groupsSplitContentFilesUnderNavigationChapters();
    void clearsSelectedBookOnReload();
    void rejectsMissingDatabase();
    void reportsMalformedSchema();
    void deduplicatesManualDatabase();
    void detectsMountedDeviceArrivalWithoutResettingSelection();
    void closesAndRestoresRemovedDevice();
    void hidesUnreadableManualDatabase();
    void browsesSavedBooksAfterUnplug();
    void dropsHighlightRemovedOnDevice();
    void snapshotsStaySeparatePerSerial();
    void syncLeavesDeviceDatabaseUnchanged();
    void serialFollowsDeviceAcrossMounts();
    void pathIdentityStartsNewLibraryWithoutSerial();
    void selectsLastOpenedSavedLibrary();
};

void KoboLibraryTest::loadsAnnotatedBooks()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath));

    KoboLibrary library(libraryStorePath(directory));
    QVERIFY(library.addDatabase(databasePath));

    const QVariantList books = library.books();
    QCOMPARE(books.size(), 4);
    QCOMPARE(books.at(0).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Alpha"));
    QCOMPARE(books.at(1).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("beta"));
    QCOMPARE(books.at(2).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Le labyrinthe des égarés"));
    QCOMPARE(books.at(3).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("missing-volume"));

    const QVariantMap alpha = bookWithVolumeId(books, QStringLiteral("book-alpha"));
    QCOMPARE(alpha.value(QStringLiteral("author")).toString(), QStringLiteral("Author A"));
    QCOMPARE(alpha.value(QStringLiteral("highlightCount")).toInt(), 2);
    QCOMPARE(alpha.value(QStringLiteral("noteCount")).toInt(), 1);

    const QVariantMap beta = bookWithVolumeId(books, QStringLiteral("book-beta"));
    QCOMPARE(beta.value(QStringLiteral("highlightCount")).toInt(), 0);
    QCOMPARE(beta.value(QStringLiteral("noteCount")).toInt(), 1);

    const QVariantMap fallback = bookWithVolumeId(books, QStringLiteral("missing-volume"));
    QCOMPARE(fallback.value(QStringLiteral("title")).toString(), QStringLiteral("missing-volume"));
    QCOMPARE(fallback.value(QStringLiteral("author")).toString(), QString());
    QCOMPARE(fallback.value(QStringLiteral("highlightCount")).toInt(), 1);
    QCOMPARE(fallback.value(QStringLiteral("noteCount")).toInt(), 0);
    const QVariantMap split = bookWithVolumeId(books, QStringLiteral("book-split"));
    QCOMPARE(split.value(QStringLiteral("author")).toString(), QStringLiteral("Amin Maalouf"));
    QCOMPARE(split.value(QStringLiteral("highlightCount")).toInt(), 5);
    QCOMPARE(split.value(QStringLiteral("noteCount")).toInt(), 0);
    QVERIFY(library.statusText().contains(QStringLiteral("Loaded 4 annotated books")));
}

void KoboLibraryTest::formatsSelectedBookAsMarkdown()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath));

    KoboLibrary library(libraryStorePath(directory));
    QVERIFY(library.addDatabase(databasePath));

    const int alphaIndex = bookIndexWithVolumeId(library.books(), QStringLiteral("book-alpha"));
    QVERIFY(alphaIndex >= 0);
    library.setCurrentBookIndex(alphaIndex);

    QCOMPARE(library.currentBookIndex(), alphaIndex);
    QCOMPARE(library.currentBookTitle(), QStringLiteral("Alpha"));
    QCOMPARE(library.currentBookAuthor(), QStringLiteral("Author A"));
    QCOMPARE(library.annotationStatusText(), QString());
    QCOMPARE(library.currentBookMarkdown(), QStringLiteral(
        "## Chapter One\n\n"
        "> Stars *stay* & <tag> second line\n"
        ">\n"
        "> Another paragraph continues here\n\n\n"
        "> Selected note text\n\n"
        "Written note\n"
        "second note line\n\n\n"
        "## Chapter Two\n\n"
        "> Later text"));
    QCOMPARE(library.currentBookObsidianMarkdown(), QStringLiteral(
        "## Chapter One\n\n"
        "> [!quote]\n"
        "> Stars *stay* & <tag> second line\n"
        ">\n"
        "> Another paragraph continues here\n\n\n"
        "> [!quote]\n"
        "> Selected note text\n\n"
        "Written note\n"
        "second note line\n\n\n"
        "## Chapter Two\n\n"
        "> [!quote]\n"
        "> Later text"));
    QCOMPARE(library.currentBookPlainText(), QStringLiteral(
        "Chapter One\n\n"
        "Stars *stay* & <tag> second line\n\n"
        "Another paragraph continues here\n\n\n"
        "Selected note text\n\n"
        "Written note\n"
        "second note line\n\n\n"
        "Chapter Two\n\n"
        "Later text"));

    const int betaIndex = bookIndexWithVolumeId(library.books(), QStringLiteral("book-beta"));
    QVERIFY(betaIndex >= 0);
    library.setCurrentBookIndex(betaIndex);
    QCOMPARE(library.currentBookMarkdown(), QStringLiteral("## Beta Chapter\n\nMargin note"));
    QCOMPARE(library.currentBookObsidianMarkdown(), QStringLiteral("## Beta Chapter\n\nMargin note"));
    QCOMPARE(library.currentBookPlainText(), QStringLiteral("Beta Chapter\n\nMargin note"));

    const int fallbackIndex = bookIndexWithVolumeId(library.books(), QStringLiteral("missing-volume"));
    QVERIFY(fallbackIndex >= 0);
    library.setCurrentBookIndex(fallbackIndex);
    QCOMPARE(library.currentBookMarkdown(), QStringLiteral("## Untitled chapter\n\n> Orphaned highlight"));
    QCOMPARE(library.currentBookObsidianMarkdown(), QStringLiteral(
        "## Untitled chapter\n\n> [!quote]\n> Orphaned highlight"));
    QCOMPARE(library.currentBookPlainText(), QStringLiteral(
        "Untitled chapter\n\nOrphaned highlight"));

    library.setCurrentBookIndex(-1);
    QCOMPARE(library.currentBookIndex(), -1);
    QVERIFY(library.currentBookTitle().isEmpty());
    QVERIFY(library.currentBookAuthor().isEmpty());
    QVERIFY(library.currentBookMarkdown().isEmpty());
    QVERIFY(library.currentBookObsidianMarkdown().isEmpty());
    QVERIFY(library.currentBookPlainText().isEmpty());
}

void KoboLibraryTest::groupsSplitContentFilesUnderNavigationChapters()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath));

    KoboLibrary library(libraryStorePath(directory));
    QVERIFY(library.addDatabase(databasePath));

    const int bookIndex = bookIndexWithVolumeId(library.books(), QStringLiteral("book-split"));
    QVERIFY(bookIndex >= 0);
    library.setCurrentBookIndex(bookIndex);

    QCOMPARE(library.currentBookMarkdown(), QStringLiteral(
        "## I. Les étincelles japonaises\n\n"
        "> First part-one highlight\n\n\n"
        "> Second part-one highlight\n\n\n"
        "## III. Une si longue marche\n\n"
        "> Part-three highlight\n\n\n"
        "## Épilogue. Un monde à reconstruire\n\n"
        "> First epilogue highlight\n\n\n"
        "> Second epilogue highlight"));
    QVERIFY(!library.currentBookMarkdown().contains(QStringLiteral(".xhtml")));

    QCOMPARE(library.currentBookObsidianMarkdown(), QStringLiteral(
        "## I. Les étincelles japonaises\n\n"
        "> [!quote]\n"
        "> First part-one highlight\n\n\n"
        "> [!quote]\n"
        "> Second part-one highlight\n\n\n"
        "## III. Une si longue marche\n\n"
        "> [!quote]\n"
        "> Part-three highlight\n\n\n"
        "## Épilogue. Un monde à reconstruire\n\n"
        "> [!quote]\n"
        "> First epilogue highlight\n\n\n"
        "> [!quote]\n"
        "> Second epilogue highlight"));
    QCOMPARE(library.currentBookPlainText(), QStringLiteral(
        "I. Les étincelles japonaises\n\n"
        "First part-one highlight\n\n\n"
        "Second part-one highlight\n\n\n"
        "III. Une si longue marche\n\n"
        "Part-three highlight\n\n\n"
        "Épilogue. Un monde à reconstruire\n\n"
        "First epilogue highlight\n\n\n"
        "Second epilogue highlight"));
}

void KoboLibraryTest::clearsSelectedBookOnReload()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath));

    KoboLibrary library(libraryStorePath(directory));
    QVERIFY(library.addDatabase(databasePath));
    library.setCurrentBookIndex(0);
    QVERIFY(!library.currentBookMarkdown().isEmpty());

    library.refreshDevices();
    QCOMPARE(library.currentBookIndex(), -1);
    QVERIFY(library.currentBookMarkdown().isEmpty());
    QVERIFY(library.currentBookObsidianMarkdown().isEmpty());
    QVERIFY(library.currentBookPlainText().isEmpty());
}

void KoboLibraryTest::rejectsMissingDatabase()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    KoboLibrary library(libraryStorePath(directory));
    QVERIFY(!library.addDatabase(directory.filePath(QStringLiteral("missing.sqlite"))));
    QVERIFY(library.books().isEmpty());
    QVERIFY(library.statusText().contains(QStringLiteral("not a readable file")));
}

void KoboLibraryTest::reportsMalformedSchema()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath, false));

    KoboLibrary library(libraryStorePath(directory));
    QVERIFY(!library.addDatabase(databasePath));
    QVERIFY(library.books().isEmpty());
    QVERIFY(library.statusText().contains(QStringLiteral("Could not read Kobo highlights")));
}

void KoboLibraryTest::deduplicatesManualDatabase()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath));

    KoboLibrary library(libraryStorePath(directory));
    QVERIFY(library.addDatabase(databasePath));
    QVERIFY(library.addDatabase(databasePath));
    QCOMPARE(devicePathOccurrences(library.devices(), databasePath), 1);
}

void KoboLibraryTest::detectsMountedDeviceArrivalWithoutResettingSelection()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString mountPath = directory.filePath(QStringLiteral("KOBOeReader"));
    const QString databasePath = QDir(mountPath).filePath(QStringLiteral(".kobo/KoboReader.sqlite"));
    QVERIFY(QDir().mkpath(QFileInfo(databasePath).path()));
    QVERIFY(createDatabase(databasePath));

    QList<KoboVolume> volumes;
    KoboLibrary library([&volumes] { return volumes; }, 20, libraryStorePath(directory));
    library.refreshDevices();
    QCOMPARE(library.devices().size(), 0);

    volumes.append(KoboVolume{mountPath, QStringLiteral("Kobo Reader")});
    QTRY_COMPARE_WITH_TIMEOUT(library.devices().size(), 1, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(library.books().size(), 4, 5000);

    library.setCurrentBookIndex(0);
    const QString selectedMarkdown = library.currentBookMarkdown();
    QVERIFY(!selectedMarkdown.isEmpty());
    QSignalSpy currentBookChangedSpy(&library, &KoboLibrary::currentBookChanged);

    QTest::qWait(70);
    QCOMPARE(currentBookChangedSpy.count(), 0);
    QCOMPARE(library.currentBookIndex(), 0);
    QCOMPARE(library.currentBookMarkdown(), selectedMarkdown);
}

void KoboLibraryTest::closesAndRestoresRemovedDevice()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString mountPath = directory.filePath(QStringLiteral("KOBOeReader"));
    const QString databasePath = QDir(mountPath).filePath(QStringLiteral(".kobo/KoboReader.sqlite"));
    QVERIFY(QDir().mkpath(QFileInfo(databasePath).path()));
    QVERIFY(createDatabase(databasePath));

    QList<KoboVolume> volumes{{mountPath, QStringLiteral("Kobo Reader")}};
    KoboLibrary library([&volumes] { return volumes; }, 20, libraryStorePath(directory));
    library.refreshDevices();
    QCOMPARE(library.currentDeviceIndex(), 0);
    QCOMPARE(library.books().size(), 4);
    const int alphaIndex = bookIndexWithVolumeId(library.books(), QStringLiteral("book-alpha"));
    QVERIFY(alphaIndex >= 0);
    library.setCurrentBookIndex(alphaIndex);
    const QString markdown = library.currentBookMarkdown();
    QVERIFY(!markdown.isEmpty());

    volumes.clear();
    QTRY_COMPARE_WITH_TIMEOUT(library.statusText(),
                              QStringLiteral("Showing saved highlights from Kobo Reader. Connect the Kobo to update."),
                              5000);
    QCOMPARE(library.currentDeviceIndex(), 0);
    QCOMPARE(library.books().size(), 4);
    QCOMPARE(library.currentBookMarkdown(), markdown);
    QVERIFY(library.devices().at(0).toMap().value(QStringLiteral("displayName")).toString().endsWith(QStringLiteral(" (saved)")));

    volumes.append(KoboVolume{mountPath, QStringLiteral("Kobo Reader")});
    QTRY_COMPARE_WITH_TIMEOUT(library.currentDeviceIndex(), 0, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(library.books().size(), 4, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(library.statusText(),
                              QStringLiteral("Loaded 4 annotated books from Kobo Reader."),
                              5000);
}

void KoboLibraryTest::hidesUnreadableManualDatabase()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath));

    KoboLibrary library([] { return QList<KoboVolume>(); }, 20, libraryStorePath(directory));
    QVERIFY(library.addDatabase(databasePath));
    QCOMPARE(library.devices().size(), 1);
    const int alphaIndex = bookIndexWithVolumeId(library.books(), QStringLiteral("book-alpha"));
    QVERIFY(alphaIndex >= 0);
    library.setCurrentBookIndex(alphaIndex);
    const QString markdown = library.currentBookMarkdown();
    QVERIFY(!markdown.isEmpty());

    QVERIFY(QFile::remove(databasePath));
    QTRY_VERIFY_WITH_TIMEOUT(library.devices().size() == 1
                             && library.devices().at(0).toMap().value(QStringLiteral("displayName")).toString().endsWith(QStringLiteral(" (saved)")),
                             5000);
    QCOMPARE(library.currentDeviceIndex(), 0);
    QCOMPARE(library.books().size(), 4);
    QCOMPARE(library.currentBookMarkdown(), markdown);
    QVERIFY(library.statusText().contains(QStringLiteral("Showing saved highlights from ")));
}

void KoboLibraryTest::browsesSavedBooksAfterUnplug()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString mountPath = directory.filePath(QStringLiteral("KOBOeReader"));
    const QString databasePath = mountedDatabasePath(mountPath);
    QVERIFY(QDir().mkpath(QFileInfo(databasePath).path()));
    QVERIFY(createDatabase(databasePath));
    QVERIFY(writeSerialNumber(databasePath, QStringLiteral("N-BROWSE")));

    QList<KoboVolume> volumes{{mountPath, QStringLiteral("Kobo Reader")}};
    KoboLibrary library([&volumes] { return volumes; }, 20, libraryStorePath(directory));
    library.refreshDevices();
    QTRY_COMPARE_WITH_TIMEOUT(library.books().size(), 4, 5000);

    const int alphaIndex = bookIndexWithVolumeId(library.books(), QStringLiteral("book-alpha"));
    const int betaIndex = bookIndexWithVolumeId(library.books(), QStringLiteral("book-beta"));
    QVERIFY(alphaIndex >= 0);
    QVERIFY(betaIndex >= 0);
    library.setCurrentBookIndex(alphaIndex);
    const QString alphaMarkdown = library.currentBookMarkdown();
    const QString alphaObsidian = library.currentBookObsidianMarkdown();
    const QString alphaPlain = library.currentBookPlainText();
    library.setCurrentBookIndex(betaIndex);
    const QString betaMarkdown = library.currentBookMarkdown();
    const QString betaObsidian = library.currentBookObsidianMarkdown();
    const QString betaPlain = library.currentBookPlainText();
    QVERIFY(alphaMarkdown != betaMarkdown);

    volumes.clear();
    QTRY_COMPARE_WITH_TIMEOUT(library.statusText(),
                              QStringLiteral("Showing saved highlights from Kobo Reader. Connect the Kobo to update."),
                              5000);
    QCOMPARE(library.currentBookIndex(), betaIndex);
    QCOMPARE(library.currentBookMarkdown(), betaMarkdown);
    QCOMPARE(library.currentBookObsidianMarkdown(), betaObsidian);
    QCOMPARE(library.currentBookPlainText(), betaPlain);

    library.setCurrentBookIndex(bookIndexWithVolumeId(library.books(), QStringLiteral("book-alpha")));
    QCOMPARE(library.currentBookMarkdown(), alphaMarkdown);
    QCOMPARE(library.currentBookObsidianMarkdown(), alphaObsidian);
    QCOMPARE(library.currentBookPlainText(), alphaPlain);
}

void KoboLibraryTest::dropsHighlightRemovedOnDevice()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString mountPath = directory.filePath(QStringLiteral("KOBOeReader"));
    const QString databasePath = mountedDatabasePath(mountPath);
    QVERIFY(QDir().mkpath(QFileInfo(databasePath).path()));
    QVERIFY(createDatabase(databasePath));
    QVERIFY(writeSerialNumber(databasePath, QStringLiteral("N-DROP")));

    QList<KoboVolume> volumes{{mountPath, QStringLiteral("Kobo Reader")}};
    KoboLibrary library([&volumes] { return volumes; }, 20, libraryStorePath(directory));
    library.refreshDevices();
    QTRY_COMPARE_WITH_TIMEOUT(library.books().size(), 4, 5000);
    library.setCurrentBookIndex(bookIndexWithVolumeId(library.books(), QStringLiteral("book-alpha")));
    QVERIFY(library.currentBookMarkdown().contains(QStringLiteral("Later text")));

    volumes.clear();
    QTRY_VERIFY_WITH_TIMEOUT(library.statusText().contains(QStringLiteral("Showing saved highlights")), 5000);
    QVERIFY(library.currentBookMarkdown().contains(QStringLiteral("Later text")));
    QVERIFY(alterDatabase(databasePath, {QStringLiteral("DELETE FROM Bookmark WHERE BookmarkID = 'later-highlight'")}));

    volumes.append(KoboVolume{mountPath, QStringLiteral("Kobo Reader")});
    QTRY_VERIFY_WITH_TIMEOUT(library.statusText().contains(QStringLiteral("Loaded 4 annotated books")), 5000);
    const QVariantMap alpha = bookWithVolumeId(library.books(), QStringLiteral("book-alpha"));
    QCOMPARE(alpha.value(QStringLiteral("highlightCount")).toInt(), 1);
    library.setCurrentBookIndex(bookIndexWithVolumeId(library.books(), QStringLiteral("book-alpha")));
    const QString updated = library.currentBookMarkdown();
    QVERIFY(!updated.contains(QStringLiteral("Later text")));
    QVERIFY(updated.contains(QStringLiteral("Selected note text")));

    volumes.clear();
    QTRY_VERIFY_WITH_TIMEOUT(library.statusText().contains(QStringLiteral("Showing saved highlights")), 5000);
    QCOMPARE(library.currentBookMarkdown(), updated);
    QCOMPARE(bookWithVolumeId(library.books(), QStringLiteral("book-alpha")).value(QStringLiteral("highlightCount")).toInt(), 1);
}

void KoboLibraryTest::snapshotsStaySeparatePerSerial()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString mountA = directory.filePath(QStringLiteral("AlphaKobo"));
    const QString mountB = directory.filePath(QStringLiteral("BetaKobo"));
    const QString pathA = mountedDatabasePath(mountA);
    const QString pathB = mountedDatabasePath(mountB);
    QVERIFY(QDir().mkpath(QFileInfo(pathA).path()));
    QVERIFY(QDir().mkpath(QFileInfo(pathB).path()));
    QVERIFY(createDatabase(pathA));
    QVERIFY(createDatabase(pathB));
    QVERIFY(writeSerialNumber(pathA, QStringLiteral("N-SERIAL-A")));
    QVERIFY(writeSerialNumber(pathB, QStringLiteral("N-SERIAL-B")));
    QVERIFY(alterDatabase(pathB, {
        QStringLiteral("UPDATE content SET Title = 'Serial B Only' WHERE ContentID = 'book-alpha'"),
        QStringLiteral("DELETE FROM Bookmark WHERE VolumeID != 'book-alpha'"),
    }));

    QList<KoboVolume> volumes{{mountA, QStringLiteral("Alpha Kobo")}};
    KoboLibrary library([&volumes] { return volumes; }, 20, libraryStorePath(directory));
    library.refreshDevices();
    QTRY_COMPARE_WITH_TIMEOUT(library.books().size(), 4, 5000);
    library.setCurrentBookIndex(0);
    const QString alphaMarkdown = library.currentBookMarkdown();
    QVERIFY(!alphaMarkdown.isEmpty());

    volumes.append(KoboVolume{mountB, QStringLiteral("Beta Kobo")});
    QTRY_COMPARE_WITH_TIMEOUT(library.devices().size(), 2, 5000);
    QCOMPARE(library.currentDeviceIndex(), deviceIndexForPath(library.devices(), pathA));
    QCOMPARE(library.currentBookMarkdown(), alphaMarkdown);

    library.setCurrentDeviceIndex(deviceIndexForPath(library.devices(), pathB));
    QCOMPARE(library.books().size(), 1);
    QCOMPARE(library.books().at(0).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Serial B Only"));

    volumes.clear();
    QTRY_VERIFY_WITH_TIMEOUT(library.devices().size() == 2
                             && library.devices().at(0).toMap().value(QStringLiteral("displayName")).toString().endsWith(QStringLiteral(" (saved)"))
                             && library.devices().at(1).toMap().value(QStringLiteral("displayName")).toString().endsWith(QStringLiteral(" (saved)")),
                             5000);
    const int savedA = deviceIndexForKey(library.devices(), QStringLiteral("N-SERIAL-A"));
    const int savedB = deviceIndexForKey(library.devices(), QStringLiteral("N-SERIAL-B"));
    QVERIFY(savedA >= 0);
    QVERIFY(savedB >= 0);
    QVERIFY(savedA != savedB);
    library.setCurrentDeviceIndex(savedA);
    QCOMPARE(library.books().size(), 4);
    QCOMPARE(bookWithVolumeId(library.books(), QStringLiteral("book-alpha")).value(QStringLiteral("title")).toString(),
             QStringLiteral("Alpha"));
    library.setCurrentDeviceIndex(savedB);
    QCOMPARE(library.books().size(), 1);
    QCOMPARE(library.books().at(0).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Serial B Only"));
}

void KoboLibraryTest::syncLeavesDeviceDatabaseUnchanged()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath));

    QFile database(databasePath);
    QVERIFY(database.open(QIODevice::ReadOnly));
    const QByteArray before = database.readAll();
    database.close();
    QVERIFY(!before.isEmpty());

    {
        KoboLibrary library([] { return QList<KoboVolume>(); }, 60000, libraryStorePath(directory));
        QVERIFY(library.addDatabase(databasePath));
        QCOMPARE(library.books().size(), 4);
    }

    QVERIFY(database.open(QIODevice::ReadOnly));
    const QByteArray after = database.readAll();
    database.close();
    QCOMPARE(after, before);

    KoboLibrary saved([] { return QList<KoboVolume>(); }, 60000, libraryStorePath(directory));
    saved.refreshDevices();
    QCOMPARE(saved.devices().size(), 1);
    QCOMPARE(saved.books().size(), 4);
}

void KoboLibraryTest::serialFollowsDeviceAcrossMounts()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString mountA = directory.filePath(QStringLiteral("MountA"));
    const QString mountB = directory.filePath(QStringLiteral("MountB"));
    const QString pathA = mountedDatabasePath(mountA);
    const QString pathB = mountedDatabasePath(mountB);
    QVERIFY(QDir().mkpath(QFileInfo(pathA).path()));
    QVERIFY(QDir().mkpath(QFileInfo(pathB).path()));
    QVERIFY(createDatabase(pathA));
    QVERIFY(createDatabase(pathB));
    QVERIFY(writeSerialNumber(pathA, QStringLiteral("N-MOVED")));
    QVERIFY(writeSerialNumber(pathB, QStringLiteral("N-MOVED")));
    QVERIFY(alterDatabase(pathB, {
        QStringLiteral("UPDATE content SET Title = 'Moved Title' WHERE ContentID = 'book-alpha'"),
    }));

    QList<KoboVolume> volumes{{mountA, QStringLiteral("Kobo One")}};
    const QString storePath = libraryStorePath(directory);
    {
        KoboLibrary library([&volumes] { return volumes; }, 20, storePath);
        library.refreshDevices();
        QTRY_COMPARE_WITH_TIMEOUT(library.books().size(), 4, 5000);
        QCOMPARE(bookWithVolumeId(library.books(), QStringLiteral("book-alpha")).value(QStringLiteral("title")).toString(),
                 QStringLiteral("Alpha"));

        volumes.clear();
        QTRY_VERIFY_WITH_TIMEOUT(library.devices().size() == 1
                                 && library.devices().at(0).toMap().value(QStringLiteral("displayName")).toString().endsWith(QStringLiteral(" (saved)")),
                                 5000);

        volumes.append(KoboVolume{mountB, QStringLiteral("Kobo Two")});
        QTRY_COMPARE_WITH_TIMEOUT(library.devices().size(), 1, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(library.statusText(),
                                  QStringLiteral("Loaded 4 annotated books from Kobo Two."),
                                  5000);
        QCOMPARE(bookWithVolumeId(library.books(), QStringLiteral("book-alpha")).value(QStringLiteral("title")).toString(),
                 QStringLiteral("Moved Title"));
        QCOMPARE(library.devices().at(0).toMap().value(QStringLiteral("deviceKey")).toString(), QStringLiteral("N-MOVED"));

        volumes.clear();
        QTRY_VERIFY_WITH_TIMEOUT(library.devices().size() == 1
                                 && library.devices().at(0).toMap().value(QStringLiteral("displayName")).toString().endsWith(QStringLiteral(" (saved)")),
                                 5000);
        QCOMPARE(library.devices().at(0).toMap().value(QStringLiteral("deviceKey")).toString(), QStringLiteral("N-MOVED"));
        QCOMPARE(bookWithVolumeId(library.books(), QStringLiteral("book-alpha")).value(QStringLiteral("title")).toString(),
                 QStringLiteral("Moved Title"));
    }

    KoboLibrary restored([] { return QList<KoboVolume>(); }, 60000, storePath);
    restored.refreshDevices();
    QCOMPARE(restored.devices().size(), 1);
    QCOMPARE(restored.devices().at(0).toMap().value(QStringLiteral("deviceKey")).toString(), QStringLiteral("N-MOVED"));
    QCOMPARE(bookWithVolumeId(restored.books(), QStringLiteral("book-alpha")).value(QStringLiteral("title")).toString(),
             QStringLiteral("Moved Title"));
}

void KoboLibraryTest::pathIdentityStartsNewLibraryWithoutSerial()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString pathA = directory.filePath(QStringLiteral("loose-a/KoboReader.sqlite"));
    const QString pathB = directory.filePath(QStringLiteral("loose-b/KoboReader.sqlite"));
    QVERIFY(QDir().mkpath(QFileInfo(pathA).path()));
    QVERIFY(QDir().mkpath(QFileInfo(pathB).path()));
    QVERIFY(createDatabase(pathA));
    QVERIFY(createDatabase(pathB));
    QVERIFY(alterDatabase(pathB, {
        QStringLiteral("UPDATE content SET Title = 'Path B Only' WHERE ContentID = 'book-alpha'"),
    }));

    const QString storePath = libraryStorePath(directory);
    {
        KoboLibrary library([] { return QList<KoboVolume>(); }, 60000, storePath);
        QVERIFY(library.addDatabase(pathA));
        QVERIFY(library.addDatabase(pathB));
        QCOMPARE(library.devices().size(), 2);
        QVERIFY(library.devices().at(0).toMap().value(QStringLiteral("serial")).toString().isEmpty());
        QVERIFY(library.devices().at(1).toMap().value(QStringLiteral("serial")).toString().isEmpty());
        QVERIFY(library.devices().at(0).toMap().value(QStringLiteral("deviceKey")).toString()
                != library.devices().at(1).toMap().value(QStringLiteral("deviceKey")).toString());
    }

    KoboLibrary saved([] { return QList<KoboVolume>(); }, 60000, storePath);
    saved.refreshDevices();
    QCOMPARE(saved.devices().size(), 2);
    const int indexA = deviceIndexForPath(saved.devices(), pathA);
    const int indexB = deviceIndexForPath(saved.devices(), pathB);
    QVERIFY(indexA >= 0);
    QVERIFY(indexB >= 0);
    QVERIFY(indexA != indexB);
    saved.setCurrentDeviceIndex(indexA);
    QCOMPARE(bookWithVolumeId(saved.books(), QStringLiteral("book-alpha")).value(QStringLiteral("title")).toString(),
             QStringLiteral("Alpha"));
    saved.setCurrentDeviceIndex(indexB);
    QCOMPARE(bookWithVolumeId(saved.books(), QStringLiteral("book-alpha")).value(QStringLiteral("title")).toString(),
             QStringLiteral("Path B Only"));
}

void KoboLibraryTest::selectsLastOpenedSavedLibrary()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString mountA = directory.filePath(QStringLiteral("First"));
    const QString mountB = directory.filePath(QStringLiteral("Second"));
    const QString pathA = mountedDatabasePath(mountA);
    const QString pathB = mountedDatabasePath(mountB);
    QVERIFY(QDir().mkpath(QFileInfo(pathA).path()));
    QVERIFY(QDir().mkpath(QFileInfo(pathB).path()));
    QVERIFY(createDatabase(pathA));
    QVERIFY(createDatabase(pathB));
    QVERIFY(writeSerialNumber(pathA, QStringLiteral("N-LAST-A")));
    QVERIFY(writeSerialNumber(pathB, QStringLiteral("N-LAST-B")));
    QVERIFY(alterDatabase(pathB, {
        QStringLiteral("UPDATE content SET Title = 'Last Opened' WHERE ContentID = 'book-alpha'"),
    }));

    const QString storePath = libraryStorePath(directory);
    {
        QList<KoboVolume> volumes{
            {mountA, QStringLiteral("Alpha Kobo")},
            {mountB, QStringLiteral("Beta Kobo")},
        };
        KoboLibrary library([&volumes] { return volumes; }, 60000, storePath);
        library.refreshDevices();
        QCOMPARE(library.devices().size(), 2);
        library.setCurrentDeviceIndex(deviceIndexForKey(library.devices(), QStringLiteral("N-LAST-B")));
        QCOMPARE(bookWithVolumeId(library.books(), QStringLiteral("book-alpha")).value(QStringLiteral("title")).toString(),
                 QStringLiteral("Last Opened"));
    }

    KoboLibrary restored([] { return QList<KoboVolume>(); }, 60000, storePath);
    restored.refreshDevices();
    QCOMPARE(restored.devices().size(), 2);
    QCOMPARE(restored.currentDeviceIndex(), deviceIndexForKey(restored.devices(), QStringLiteral("N-LAST-B")));
    QCOMPARE(bookWithVolumeId(restored.books(), QStringLiteral("book-alpha")).value(QStringLiteral("title")).toString(),
             QStringLiteral("Last Opened"));
}

QTEST_GUILESS_MAIN(KoboLibraryTest)

#include "kobo_library_test.moc"
