#include <QtTest>

#include <QDir>
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
                "ContentType INTEGER, VolumeIndex INTEGER)"))
                && execute(database, QStringLiteral(
                    "CREATE TABLE Bookmark ("
                    "BookmarkID TEXT, VolumeID TEXT, ContentID TEXT, Text TEXT, Annotation TEXT, "
                    "Hidden TEXT, StartContainerChildIndex INTEGER, StartOffset INTEGER, ChapterProgress REAL)"))
                && execute(database, QStringLiteral(
                    "INSERT INTO content VALUES "
                    "('book-alpha', NULL, 'Alpha', 'Author A', 6, 0), "
                    "('book-beta', NULL, 'beta', 'Author B', 6, 0), "
                    "('book-empty', NULL, 'Empty', 'Author C', 6, 0), "
                    "('chapter-1', 'book-alpha', 'Chapter' || char(10) || 'One', NULL, 9, 1), "
                    "('chapter-2', 'book-alpha', 'Chapter Two', NULL, 9, 2), "
                    "('book-beta!opf!chapter-2-1', 'book-beta', 'Beta Chapter', NULL, 899, 2)"))
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

} // namespace

class KoboLibraryTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsAnnotatedBooks();
    void formatsSelectedBookAsMarkdown();
    void clearsSelectedBookOnReload();
    void rejectsMissingDatabase();
    void reportsMalformedSchema();
    void deduplicatesManualDatabase();
};

void KoboLibraryTest::loadsAnnotatedBooks()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath));

    KoboLibrary library;
    QVERIFY(library.addDatabase(databasePath));

    const QVariantList books = library.books();
    QCOMPARE(books.size(), 3);
    QCOMPARE(books.at(0).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Alpha"));
    QCOMPARE(books.at(1).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("beta"));
    QCOMPARE(books.at(2).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("missing-volume"));

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
    QVERIFY(library.statusText().contains(QStringLiteral("Loaded 3 annotated books")));
}

void KoboLibraryTest::formatsSelectedBookAsMarkdown()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath));

    KoboLibrary library;
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

void KoboLibraryTest::clearsSelectedBookOnReload()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("KoboReader.sqlite"));
    QVERIFY(createDatabase(databasePath));

    KoboLibrary library;
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

    KoboLibrary library;
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

    KoboLibrary library;
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

    KoboLibrary library;
    QVERIFY(library.addDatabase(databasePath));
    QVERIFY(library.addDatabase(databasePath));
    QCOMPARE(devicePathOccurrences(library.devices(), databasePath), 1);
}

QTEST_GUILESS_MAIN(KoboLibraryTest)

#include "kobo_library_test.moc"
