#include "annotation_store.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

static QVariant storedText(const QString &text)
{
    return QVariant(text.isNull() ? QStringLiteral("") : text);
}

QString defaultAnnotationStorePath()
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directory.isEmpty())
        return QStringLiteral("library.sqlite");
    return QDir(directory).filePath(QStringLiteral("library.sqlite"));
}

AnnotationStore::AnnotationStore(const QString &databasePath)
{
    const QString path = databasePath.trimmed();
    if (path.isEmpty()) {
        m_lastError = QStringLiteral("The highlight library path is empty.");
        return;
    }

    const QFileInfo databaseInfo(path);
    if (!QDir().mkpath(databaseInfo.absolutePath())) {
        m_lastError = QStringLiteral("Could not create the highlight library directory.");
        return;
    }

    m_connectionName = QStringLiteral("kbextract-library-%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setDatabaseName(databaseInfo.absoluteFilePath());
    if (!m_database.open()) {
        m_lastError = m_database.lastError().text();
        m_database = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return;
    }

    static const QStringList statements = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS libraries ("
            "device_key TEXT PRIMARY KEY,"
            "display_name TEXT NOT NULL,"
            "serial TEXT NOT NULL,"
            "database_path TEXT NOT NULL,"
            "synced_at TEXT NOT NULL,"
            "last_opened INTEGER NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS books ("
            "device_key TEXT NOT NULL,"
            "volume_id TEXT NOT NULL,"
            "title TEXT NOT NULL,"
            "author TEXT NOT NULL,"
            "highlight_count INTEGER NOT NULL,"
            "note_count INTEGER NOT NULL,"
            "PRIMARY KEY (device_key, volume_id))"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS annotations ("
            "device_key TEXT NOT NULL,"
            "volume_id TEXT NOT NULL,"
            "bookmark_id TEXT NOT NULL,"
            "chapter_key TEXT NOT NULL,"
            "chapter_title TEXT NOT NULL,"
            "chapter_order INTEGER,"
            "has_chapter_order INTEGER NOT NULL,"
            "highlight_text TEXT NOT NULL,"
            "note_text TEXT NOT NULL,"
            "sort_index INTEGER NOT NULL,"
            "PRIMARY KEY (device_key, volume_id, sort_index))"),
    };

    QSqlQuery query(m_database);
    for (const QString &statement : statements) {
        if (!query.exec(statement)) {
            m_lastError = query.lastError().text();
            m_database.close();
            m_database = QSqlDatabase();
            QSqlDatabase::removeDatabase(m_connectionName);
            m_connectionName.clear();
            return;
        }
    }
}

AnnotationStore::~AnnotationStore()
{
    const QString connectionName = m_connectionName;
    m_database.close();
    m_database = QSqlDatabase();
    if (!connectionName.isEmpty() && QSqlDatabase::contains(connectionName))
        QSqlDatabase::removeDatabase(connectionName);
}

bool AnnotationStore::isOpen() const
{
    return m_database.isOpen();
}

QString AnnotationStore::lastError() const
{
    return m_lastError;
}

bool AnnotationStore::fail(const QString &error) const
{
    m_lastError = error;
    return false;
}

bool AnnotationStore::rollback(const QString &error)
{
    m_database.rollback();
    return fail(error);
}

bool AnnotationStore::replaceLibrary(const StoredLibrary &library, const QList<StoredBook> &books)
{
    if (!m_database.isOpen())
        return fail(m_lastError.isEmpty() ? QStringLiteral("The highlight library is not open.") : m_lastError);
    if (library.deviceKey.isEmpty())
        return fail(QStringLiteral("The highlight library is missing a device key."));

    if (!m_database.transaction())
        return fail(m_database.lastError().text());

    QSqlQuery clearOpened(m_database);
    if (!clearOpened.exec(QStringLiteral("UPDATE libraries SET last_opened = 0")))
        return rollback(clearOpened.lastError().text());

    QSqlQuery deleteAnnotations(m_database);
    deleteAnnotations.prepare(QStringLiteral("DELETE FROM annotations WHERE device_key = ?"));
    deleteAnnotations.addBindValue(library.deviceKey);
    if (!deleteAnnotations.exec())
        return rollback(deleteAnnotations.lastError().text());

    QSqlQuery deleteBooks(m_database);
    deleteBooks.prepare(QStringLiteral("DELETE FROM books WHERE device_key = ?"));
    deleteBooks.addBindValue(library.deviceKey);
    if (!deleteBooks.exec())
        return rollback(deleteBooks.lastError().text());

    QSqlQuery upsert(m_database);
    upsert.prepare(QStringLiteral(
        "INSERT INTO libraries (device_key, display_name, serial, database_path, synced_at, last_opened) "
        "VALUES (?, ?, ?, ?, ?, 1) "
        "ON CONFLICT(device_key) DO UPDATE SET "
        "display_name = excluded.display_name, "
        "serial = excluded.serial, "
        "database_path = excluded.database_path, "
        "synced_at = excluded.synced_at, "
        "last_opened = 1"));
    upsert.addBindValue(storedText(library.deviceKey));
    upsert.addBindValue(storedText(library.displayName));
    upsert.addBindValue(storedText(library.serial));
    upsert.addBindValue(storedText(library.databasePath));
    upsert.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    if (!upsert.exec())
        return rollback(upsert.lastError().text());

    QSqlQuery insertBook(m_database);
    insertBook.prepare(QStringLiteral(
        "INSERT INTO books (device_key, volume_id, title, author, highlight_count, note_count) "
        "VALUES (?, ?, ?, ?, ?, ?)"));
    QSqlQuery insertAnnotation(m_database);
    insertAnnotation.prepare(QStringLiteral(
        "INSERT INTO annotations ("
        "device_key, volume_id, bookmark_id, chapter_key, chapter_title, chapter_order, "
        "has_chapter_order, highlight_text, note_text, sort_index) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

    for (const StoredBook &book : books) {
        insertBook.bindValue(0, storedText(library.deviceKey));
        insertBook.bindValue(1, storedText(book.volumeId));
        insertBook.bindValue(2, storedText(book.title));
        insertBook.bindValue(3, storedText(book.author));
        insertBook.bindValue(4, book.highlightCount);
        insertBook.bindValue(5, book.noteCount);
        if (!insertBook.exec())
            return rollback(insertBook.lastError().text());

        for (const StoredAnnotation &annotation : book.annotations) {
            insertAnnotation.bindValue(0, storedText(library.deviceKey));
            insertAnnotation.bindValue(1, storedText(book.volumeId));
            insertAnnotation.bindValue(2, storedText(annotation.bookmarkId));
            insertAnnotation.bindValue(3, storedText(annotation.chapterKey));
            insertAnnotation.bindValue(4, storedText(annotation.chapterTitle));
            if (annotation.hasChapterOrder)
                insertAnnotation.bindValue(5, annotation.chapterOrder);
            else
                insertAnnotation.bindValue(5, QVariant());
            insertAnnotation.bindValue(6, annotation.hasChapterOrder ? 1 : 0);
            insertAnnotation.bindValue(7, storedText(annotation.highlightText));
            insertAnnotation.bindValue(8, storedText(annotation.noteText));
            insertAnnotation.bindValue(9, annotation.sortIndex);
            if (!insertAnnotation.exec())
                return rollback(insertAnnotation.lastError().text());
        }
    }

    if (!m_database.commit())
        return rollback(m_database.lastError().text());
    m_lastError.clear();
    return true;
}

bool AnnotationStore::markLastOpened(const QString &deviceKey)
{
    if (!m_database.isOpen())
        return fail(m_lastError.isEmpty() ? QStringLiteral("The highlight library is not open.") : m_lastError);
    if (deviceKey.isEmpty())
        return fail(QStringLiteral("The highlight library is missing a device key."));

    if (!m_database.transaction())
        return fail(m_database.lastError().text());

    QSqlQuery clearOpened(m_database);
    if (!clearOpened.exec(QStringLiteral("UPDATE libraries SET last_opened = 0")))
        return rollback(clearOpened.lastError().text());

    QSqlQuery mark(m_database);
    mark.prepare(QStringLiteral("UPDATE libraries SET last_opened = 1 WHERE device_key = ?"));
    mark.addBindValue(deviceKey);
    if (!mark.exec())
        return rollback(mark.lastError().text());

    if (!m_database.commit())
        return rollback(m_database.lastError().text());
    m_lastError.clear();
    return true;
}

QList<StoredLibrary> AnnotationStore::libraries() const
{
    QList<StoredLibrary> libraries;
    if (!m_database.isOpen())
        return libraries;

    QSqlQuery query(m_database);
    query.setForwardOnly(true);
    if (!query.exec(QStringLiteral(
            "SELECT device_key, display_name, serial, database_path, synced_at, last_opened "
            "FROM libraries "
            "ORDER BY display_name COLLATE NOCASE, device_key COLLATE NOCASE"))) {
        m_lastError = query.lastError().text();
        return {};
    }

    while (query.next()) {
        StoredLibrary library;
        library.deviceKey = query.value(0).toString();
        library.displayName = query.value(1).toString();
        library.serial = query.value(2).toString();
        library.databasePath = query.value(3).toString();
        library.syncedAt = query.value(4).toString();
        library.lastOpened = query.value(5).toInt() != 0;
        libraries.append(library);
    }
    return libraries;
}

bool AnnotationStore::books(const QString &deviceKey, QList<StoredBook> *books) const
{
    if (!books)
        return fail(QStringLiteral("Missing book list."));
    books->clear();
    if (!m_database.isOpen())
        return fail(m_lastError.isEmpty() ? QStringLiteral("The highlight library is not open.") : m_lastError);

    QSqlQuery query(m_database);
    query.setForwardOnly(true);
    query.prepare(QStringLiteral(
        "SELECT volume_id, title, author, highlight_count, note_count "
        "FROM books WHERE device_key = ? "
        "ORDER BY title COLLATE NOCASE, author COLLATE NOCASE"));
    query.addBindValue(deviceKey);
    if (!query.exec())
        return fail(query.lastError().text());

    while (query.next()) {
        StoredBook book;
        book.volumeId = query.value(0).toString();
        book.title = query.value(1).toString();
        book.author = query.value(2).toString();
        book.highlightCount = query.value(3).toInt();
        book.noteCount = query.value(4).toInt();
        books->append(book);
    }
    m_lastError.clear();
    return true;
}

bool AnnotationStore::annotations(const QString &deviceKey, const QString &volumeId,
                                  QList<StoredAnnotation> *annotations) const
{
    if (!annotations)
        return fail(QStringLiteral("Missing annotation list."));
    annotations->clear();
    if (!m_database.isOpen())
        return fail(m_lastError.isEmpty() ? QStringLiteral("The highlight library is not open.") : m_lastError);

    QSqlQuery query(m_database);
    query.setForwardOnly(true);
    query.prepare(QStringLiteral(
        "SELECT bookmark_id, highlight_text, note_text, chapter_key, chapter_title, "
        "chapter_order, has_chapter_order, sort_index "
        "FROM annotations WHERE device_key = ? AND volume_id = ? "
        "ORDER BY sort_index"));
    query.addBindValue(deviceKey);
    query.addBindValue(volumeId);
    if (!query.exec())
        return fail(query.lastError().text());

    while (query.next()) {
        StoredAnnotation annotation;
        annotation.bookmarkId = query.value(0).toString();
        annotation.highlightText = query.value(1).toString();
        annotation.noteText = query.value(2).toString();
        annotation.chapterKey = query.value(3).toString();
        annotation.chapterTitle = query.value(4).toString();
        annotation.hasChapterOrder = query.value(6).toInt() != 0;
        if (annotation.hasChapterOrder)
            annotation.chapterOrder = query.value(5).toLongLong();
        annotation.sortIndex = query.value(7).toInt();
        annotations->append(annotation);
    }
    m_lastError.clear();
    return true;
}

QString AnnotationStore::lastOpenedDeviceKey() const
{
    if (!m_database.isOpen())
        return {};

    QSqlQuery query(m_database);
    query.setForwardOnly(true);
    if (!query.exec(QStringLiteral(
            "SELECT device_key FROM libraries WHERE last_opened != 0 "
            "ORDER BY synced_at DESC LIMIT 1"))) {
        m_lastError = query.lastError().text();
        return {};
    }
    return query.next() ? query.value(0).toString() : QString();
}
