#pragma once

#include <QList>
#include <QSqlDatabase>
#include <QString>

struct StoredAnnotation {
    QString bookmarkId;
    QString highlightText;
    QString noteText;
    QString chapterKey;
    QString chapterTitle;
    qlonglong chapterOrder = 0;
    bool hasChapterOrder = false;
    int sortIndex = 0;
};

struct StoredBook {
    QString volumeId;
    QString title;
    QString author;
    int highlightCount = 0;
    int noteCount = 0;
    QList<StoredAnnotation> annotations;
};

struct StoredLibrary {
    QString deviceKey;
    QString displayName;
    QString serial;
    QString databasePath;
    QString syncedAt;
    bool lastOpened = false;
};

QString defaultAnnotationStorePath();

class AnnotationStore
{
public:
    explicit AnnotationStore(const QString &databasePath);
    ~AnnotationStore();

    AnnotationStore(const AnnotationStore &) = delete;
    AnnotationStore &operator=(const AnnotationStore &) = delete;

    bool isOpen() const;
    QString lastError() const;

    bool replaceLibrary(const StoredLibrary &library, const QList<StoredBook> &books);
    bool markLastOpened(const QString &deviceKey);
    QList<StoredLibrary> libraries() const;
    bool books(const QString &deviceKey, QList<StoredBook> *books) const;
    bool annotations(const QString &deviceKey, const QString &volumeId, QList<StoredAnnotation> *annotations) const;
    QString lastOpenedDeviceKey() const;

private:
    bool fail(const QString &error) const;
    bool rollback(const QString &error);

    QString m_connectionName;
    QSqlDatabase m_database;
    mutable QString m_lastError;
};
