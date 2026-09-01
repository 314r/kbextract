#include "kobo_library.h"

#include <algorithm>

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QStorageInfo>
#include <QUuid>

#include <utility>

namespace {

constexpr auto databaseRelativePath = ".kobo/KoboReader.sqlite";

struct ChapterMetadata {
    QString key;
    QString title;
    qlonglong order = 0;
    bool hasOrder = false;
};

struct ChapterAnnotations {
    QString title;
    QStringList annotations;
    qlonglong order = 0;
    bool hasOrder = false;
    int firstSeen = 0;
};

QString deviceDatabasePath(const QVariantMap &device)
{
    return device.value(QStringLiteral("databasePath")).toString();
}

QString normalizedText(QString text)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text.trimmed();
}

QString singleLineText(const QString &source)
{
    QStringList fragments;
    const QStringList lines = normalizedText(source).split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QString fragment = line.trimmed();
        if (!fragment.isEmpty())
            fragments.append(fragment);
    }
    return fragments.join(QLatin1Char(' '));
}

QString kepubBookmarkChapterId(QString contentId)
{
    const qsizetype suffixIndex = contentId.lastIndexOf(QLatin1Char('-'));
    const qsizetype pathIndex = contentId.lastIndexOf(QLatin1Char('!'));
    if (suffixIndex > pathIndex)
        contentId.truncate(suffixIndex);
    return contentId;
}

QString reflowedHighlightText(const QString &source)
{
    QStringList paragraphs;
    QStringList paragraphLines;

    const auto appendParagraph = [&paragraphs, &paragraphLines]() {
        if (paragraphLines.isEmpty())
            return;

        paragraphs.append(paragraphLines.join(QLatin1Char(' ')));
        paragraphLines.clear();
    };

    const QStringList lines = normalizedText(source).split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QString fragment = line.trimmed();
        if (fragment.isEmpty())
            appendParagraph();
        else
            paragraphLines.append(fragment);
    }
    appendParagraph();

    return paragraphs.join(QStringLiteral("\n\n"));
}

QString highlightBlock(const QString &text)
{
    QStringList lines = reflowedHighlightText(text).split(QLatin1Char('\n'));
    for (QString &line : lines)
        line.prepend(line.isEmpty() ? QStringLiteral(">") : QStringLiteral("> "));
    return lines.join(QLatin1Char('\n'));
}

QString noteParagraph(const QString &text)
{
    return normalizedText(text);
}

} // namespace

KoboLibrary::KoboLibrary(QObject *parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("kbextract-kobo-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

KoboLibrary::~KoboLibrary()
{
    closeDatabase();
}

QVariantList KoboLibrary::devices() const
{
    return m_devices;
}

QVariantList KoboLibrary::books() const
{
    return m_books;
}

int KoboLibrary::currentDeviceIndex() const
{
    return m_currentDeviceIndex;
}

int KoboLibrary::currentBookIndex() const
{
    return m_currentBookIndex;
}

QString KoboLibrary::currentBookTitle() const
{
    return m_currentBookTitle;
}

QString KoboLibrary::currentBookAuthor() const
{
    return m_currentBookAuthor;
}

QString KoboLibrary::currentBookMarkdown() const
{
    return m_currentBookMarkdown;
}

QString KoboLibrary::annotationStatusText() const
{
    return m_annotationStatusText;
}

QString KoboLibrary::statusText() const
{
    return m_statusText;
}

void KoboLibrary::setCurrentDeviceIndex(int index)
{
    if (index < 0 || index >= m_devices.size()) {
        const bool indexChanged = m_currentDeviceIndex != -1;
        m_currentDeviceIndex = -1;
        m_currentLoadSucceeded = false;
        closeDatabase();
        setBooks({});
        setStatusText(tr("Connect a Kobo or choose KoboReader.sqlite."));
        if (indexChanged)
            emit currentDeviceIndexChanged();
        return;
    }

    const bool indexChanged = m_currentDeviceIndex != index;
    m_currentDeviceIndex = index;
    if (indexChanged)
        emit currentDeviceIndexChanged();

    loadCurrentDatabase();
}

void KoboLibrary::setCurrentBookIndex(int index)
{
    if (index < 0 || index >= m_books.size()) {
        clearCurrentBook();
        return;
    }

    const QVariantMap book = m_books.at(index).toMap();
    const QString title = book.value(QStringLiteral("title")).toString();
    const QString author = book.value(QStringLiteral("author")).toString();
    const QString volumeId = book.value(QStringLiteral("volumeId")).toString();

    if (!m_database.isOpen()) {
        setCurrentBookState(index, title, author, {}, tr("The Kobo database is not open."));
        return;
    }

    static const QString kepubChapterQueryText = QStringLiteral(R"SQL(
        SELECT ContentID, Title, VolumeIndex
        FROM content
        WHERE CAST(ContentType AS INTEGER) = 899
          AND BookID = :volume_id
        ORDER BY COALESCE(VolumeIndex, 0), ContentID COLLATE NOCASE
    )SQL");

    QSqlQuery kepubChapterQuery(m_database);
    kepubChapterQuery.setForwardOnly(true);
    if (!kepubChapterQuery.prepare(kepubChapterQueryText)) {
        setCurrentBookState(index, title, author, {},
                            tr("Could not prepare the chapter query: %1").arg(kepubChapterQuery.lastError().text()));
        return;
    }
    kepubChapterQuery.bindValue(QStringLiteral(":volume_id"), volumeId);
    if (!kepubChapterQuery.exec()) {
        setCurrentBookState(index, title, author, {},
                            tr("Could not read this book's chapters: %1").arg(kepubChapterQuery.lastError().text()));
        return;
    }

    QHash<QString, ChapterMetadata> kepubChapters;
    while (kepubChapterQuery.next()) {
        const QString contentId = normalizedText(kepubChapterQuery.value(0).toString());
        if (contentId.isEmpty())
            continue;

        ChapterMetadata metadata;
        metadata.key = kepubBookmarkChapterId(contentId);
        metadata.title = singleLineText(kepubChapterQuery.value(1).toString());
        metadata.hasOrder = !kepubChapterQuery.value(2).isNull();
        if (metadata.hasOrder)
            metadata.order = kepubChapterQuery.value(2).toLongLong();

        kepubChapters.insert(contentId, metadata);
        kepubChapters.insert(metadata.key, metadata);
    }

    static const QString queryText = QStringLiteral(R"SQL(
        SELECT
            bm.BookmarkID,
            bm.ContentID,
            bm.Text,
            bm.Annotation,
            chapter.Title,
            chapter.VolumeIndex
        FROM Bookmark bm
        LEFT JOIN content chapter ON chapter.ContentID = bm.ContentID
        WHERE bm.VolumeID = :volume_id
          AND LOWER(COALESCE(CAST(bm.Hidden AS TEXT), 'false')) IN ('false', '0')
          AND NOT (
              COALESCE(bm.StartContainerChildIndex, 0) = 0
              AND COALESCE(bm.StartOffset, 0) = 0
          )
          AND (
              NULLIF(TRIM(COALESCE(bm.Text, '')), '') IS NOT NULL
              OR NULLIF(TRIM(COALESCE(bm.Annotation, '')), '') IS NOT NULL
          )
        ORDER BY bm.ContentID COLLATE NOCASE,
                 COALESCE(bm.ChapterProgress, 0),
                 bm.BookmarkID COLLATE NOCASE
    )SQL");

    QSqlQuery query(m_database);
    query.setForwardOnly(true);
    if (!query.prepare(queryText)) {
        setCurrentBookState(index, title, author, {},
                            tr("Could not prepare the annotation query: %1").arg(query.lastError().text()));
        return;
    }
    query.bindValue(QStringLiteral(":volume_id"), volumeId);
    if (!query.exec()) {
        setCurrentBookState(index, title, author, {},
                            tr("Could not read this book's annotations: %1").arg(query.lastError().text()));
        return;
    }

    QList<ChapterAnnotations> chapters;
    QHash<QString, int> chapterIndexes;
    int anonymousChapterIndex = 0;
    while (query.next()) {
        const QString bookmarkId = normalizedText(query.value(0).toString());
        const QString contentId = normalizedText(query.value(1).toString());
        const QString highlightedText = normalizedText(query.value(2).toString());
        const QString noteText = normalizedText(query.value(3).toString());
        QStringList annotationParts;
        if (!highlightedText.isEmpty())
            annotationParts.append(highlightBlock(highlightedText));
        if (!noteText.isEmpty())
            annotationParts.append(noteParagraph(noteText));
        if (annotationParts.isEmpty())
            continue;

        QString chapterKey = contentId;
        QString chapterTitle = singleLineText(query.value(4).toString());
        bool hasChapterOrder = !query.value(5).isNull();
        qlonglong chapterOrder = hasChapterOrder ? query.value(5).toLongLong() : 0;

        const auto kepubChapter = kepubChapters.constFind(contentId);
        if (kepubChapter != kepubChapters.cend()) {
            chapterKey = kepubChapter->key;
            if (!kepubChapter->title.isEmpty())
                chapterTitle = kepubChapter->title;
            if (kepubChapter->hasOrder) {
                hasChapterOrder = true;
                chapterOrder = kepubChapter->order;
            }
        }

        QString groupKey;
        if (!chapterKey.isEmpty()) {
            groupKey = QStringLiteral("chapter:%1").arg(chapterKey);
        } else if (!bookmarkId.isEmpty()) {
            groupKey = QStringLiteral("bookmark:%1").arg(bookmarkId);
        } else {
            groupKey = QStringLiteral("anonymous:%1").arg(anonymousChapterIndex++);
        }

        int chapterIndex = chapterIndexes.value(groupKey, -1);
        if (chapterIndex < 0) {
            chapterIndex = chapters.size();
            chapterIndexes.insert(groupKey, chapterIndex);
            chapters.append(ChapterAnnotations{
                .title = chapterTitle,
                .annotations = {},
                .order = chapterOrder,
                .hasOrder = hasChapterOrder,
                .firstSeen = chapterIndex,
            });
        } else {
            if (chapters[chapterIndex].title.isEmpty() && !chapterTitle.isEmpty())
                chapters[chapterIndex].title = chapterTitle;
            if (!chapters[chapterIndex].hasOrder && hasChapterOrder) {
                chapters[chapterIndex].hasOrder = true;
                chapters[chapterIndex].order = chapterOrder;
            }
        }

        chapters[chapterIndex].annotations.append(annotationParts.join(QStringLiteral("\n\n")));
    }

    std::stable_sort(chapters.begin(), chapters.end(), [](const ChapterAnnotations &left, const ChapterAnnotations &right) {
        if (left.hasOrder && right.hasOrder && left.order != right.order)
            return left.order < right.order;
        if (left.hasOrder != right.hasOrder)
            return left.hasOrder;
        return left.firstSeen < right.firstSeen;
    });

    QStringList chapterSections;
    for (const ChapterAnnotations &chapter : std::as_const(chapters)) {
        const QString chapterTitle = chapter.title.isEmpty() ? tr("Untitled chapter") : chapter.title;
        chapterSections.append(QStringLiteral("## ") + chapterTitle
                               + QStringLiteral("\n\n")
                               + chapter.annotations.join(QStringLiteral("\n\n\n")));
    }

    const QString markdown = chapterSections.join(QStringLiteral("\n\n\n"));
    const QString annotationStatus = markdown.isEmpty()
        ? tr("No visible highlights or notes for this book.")
        : QString();
    setCurrentBookState(index, title, author, markdown, annotationStatus);
}

void KoboLibrary::refreshDevices()
{
    QString preferredPath;
    if (m_currentDeviceIndex >= 0 && m_currentDeviceIndex < m_devices.size())
        preferredPath = deviceDatabasePath(m_devices.at(m_currentDeviceIndex).toMap());

    rebuildDevices(preferredPath);
}

bool KoboLibrary::addDatabase(const QString &databasePath)
{
    const QString normalizedPath = normalizedDatabasePath(databasePath);
    const QFileInfo databaseInfo(normalizedPath);
    if (normalizedPath.isEmpty() || !databaseInfo.exists() || !databaseInfo.isFile() || !databaseInfo.isReadable()) {
        setStatusText(tr("The selected Kobo database is not a readable file."));
        return false;
    }

    for (int index = 0; index < m_devices.size(); ++index) {
        if (normalizedDatabasePath(deviceDatabasePath(m_devices.at(index).toMap())) == normalizedPath) {
            setCurrentDeviceIndex(index);
            return m_currentLoadSucceeded;
        }
    }

    bool isAlreadyManual = false;
    for (const QVariant &deviceValue : std::as_const(m_manualDevices)) {
        if (normalizedDatabasePath(deviceDatabasePath(deviceValue.toMap())) == normalizedPath) {
            isAlreadyManual = true;
            break;
        }
    }

    if (!isAlreadyManual)
        m_manualDevices.append(manualDevice(normalizedPath));

    rebuildDevices(normalizedPath);
    return m_currentLoadSucceeded;
}

void KoboLibrary::rebuildDevices(const QString &preferredDatabasePath)
{
    QVariantList devices;
    QSet<QString> seenPaths;

    const auto appendDevice = [this, &devices, &seenPaths](const QVariantMap &device) {
        const QString path = normalizedDatabasePath(deviceDatabasePath(device));
        if (path.isEmpty() || seenPaths.contains(path))
            return;
        seenPaths.insert(path);
        devices.append(device);
    };

    const QList<QStorageInfo> mountedVolumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : mountedVolumes) {
        if (!storage.isValid() || !storage.isReady())
            continue;

        const QString mountPath = QDir::cleanPath(storage.rootPath());
        const QString databasePath = normalizedDatabasePath(QDir(mountPath).filePath(QString::fromLatin1(databaseRelativePath)));
        const QFileInfo databaseInfo(databasePath);
        if (!databaseInfo.exists() || !databaseInfo.isFile() || !databaseInfo.isReadable())
            continue;

        QString displayName = storage.displayName().trimmed();
        if (displayName.isEmpty())
            displayName = QFileInfo(mountPath).fileName();
        if (displayName.isEmpty())
            displayName = tr("Kobo eReader");

        appendDevice({
            {QStringLiteral("displayName"), displayName},
            {QStringLiteral("mountPath"), mountPath},
            {QStringLiteral("databasePath"), databasePath},
            {QStringLiteral("manual"), false},
        });
    }

    for (const QVariant &deviceValue : std::as_const(m_manualDevices))
        appendDevice(deviceValue.toMap());

    m_devices = std::move(devices);
    emit devicesChanged();

    const QString normalizedPreferredPath = normalizedDatabasePath(preferredDatabasePath);
    int selectedIndex = -1;
    if (!normalizedPreferredPath.isEmpty()) {
        for (int index = 0; index < m_devices.size(); ++index) {
            if (normalizedDatabasePath(deviceDatabasePath(m_devices.at(index).toMap())) == normalizedPreferredPath) {
                selectedIndex = index;
                break;
            }
        }
    }
    if (selectedIndex < 0 && !m_devices.isEmpty())
        selectedIndex = 0;

    setCurrentDeviceIndex(selectedIndex);
}

bool KoboLibrary::loadCurrentDatabase()
{
    m_currentLoadSucceeded = false;
    setBooks({});
    closeDatabase();

    if (m_currentDeviceIndex < 0 || m_currentDeviceIndex >= m_devices.size()) {
        setStatusText(tr("Connect a Kobo or choose KoboReader.sqlite."));
        return false;
    }

    const QVariantMap device = m_devices.at(m_currentDeviceIndex).toMap();
    const QString databasePath = deviceDatabasePath(device);
    const QString displayName = device.value(QStringLiteral("displayName")).toString();

    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    m_database.setDatabaseName(databasePath);
    if (!m_database.open()) {
        setStatusText(tr("Could not open the Kobo database: %1").arg(m_database.lastError().text()));
        closeDatabase();
        return false;
    }

    static const QString queryText = QStringLiteral(R"SQL(
        SELECT
            bm.VolumeID AS volume_id,
            COALESCE(MAX(NULLIF(TRIM(c.Title), '')), bm.VolumeID) AS title,
            COALESCE(MAX(NULLIF(TRIM(c.Attribution), '')), '') AS author,
            SUM(CASE
                WHEN NULLIF(TRIM(COALESCE(bm.Text, '')), '') IS NOT NULL
                 AND NULLIF(TRIM(COALESCE(bm.Annotation, '')), '') IS NULL
                THEN 1 ELSE 0 END) AS highlight_count,
            SUM(CASE
                WHEN NULLIF(TRIM(COALESCE(bm.Annotation, '')), '') IS NOT NULL
                THEN 1 ELSE 0 END) AS note_count
        FROM Bookmark bm
        LEFT JOIN content c
               ON c.ContentID = bm.VolumeID
              AND c.BookID IS NULL
        WHERE bm.VolumeID IS NOT NULL
          AND TRIM(CAST(bm.VolumeID AS TEXT)) <> ''
          AND LOWER(COALESCE(CAST(bm.Hidden AS TEXT), 'false')) IN ('false', '0')
          AND NOT (
              COALESCE(bm.StartContainerChildIndex, 0) = 0
              AND COALESCE(bm.StartOffset, 0) = 0
          )
          AND (
              NULLIF(TRIM(COALESCE(bm.Text, '')), '') IS NOT NULL
              OR NULLIF(TRIM(COALESCE(bm.Annotation, '')), '') IS NOT NULL
          )
        GROUP BY bm.VolumeID
        HAVING highlight_count > 0 OR note_count > 0
        ORDER BY title COLLATE NOCASE, author COLLATE NOCASE
    )SQL");

    QSqlQuery query(m_database);
    query.setForwardOnly(true);
    if (!query.exec(queryText)) {
        setStatusText(tr("Could not read Kobo highlights: %1").arg(query.lastError().text()));
        return false;
    }

    QVariantList books;
    while (query.next()) {
        books.append(QVariantMap{
            {QStringLiteral("volumeId"), query.value(QStringLiteral("volume_id"))},
            {QStringLiteral("title"), query.value(QStringLiteral("title"))},
            {QStringLiteral("author"), query.value(QStringLiteral("author"))},
            {QStringLiteral("highlightCount"), query.value(QStringLiteral("highlight_count"))},
            {QStringLiteral("noteCount"), query.value(QStringLiteral("note_count"))},
        });
    }

    setBooks(std::move(books));
    m_currentLoadSucceeded = true;
    if (m_books.isEmpty()) {
        setStatusText(tr("No books with highlights or notes."));
    } else {
        setStatusText(tr("Loaded %1 annotated book%2 from %3.")
                          .arg(m_books.size())
                          .arg(m_books.size() == 1 ? QString() : QStringLiteral("s"))
                          .arg(displayName));
    }
    return true;
}

void KoboLibrary::closeDatabase()
{
    if (m_database.isValid())
        m_database.close();
    m_database = QSqlDatabase();
    if (QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);
}

void KoboLibrary::setBooks(QVariantList books)
{
    clearCurrentBook();
    if (m_books == books)
        return;
    m_books = std::move(books);
    emit booksChanged();
}

void KoboLibrary::clearCurrentBook()
{
    setCurrentBookState(-1, {}, {}, {}, {});
}

void KoboLibrary::setCurrentBookState(int index, const QString &title, const QString &author,
                                      const QString &markdown, const QString &statusText)
{
    if (m_currentBookIndex == index
        && m_currentBookTitle == title
        && m_currentBookAuthor == author
        && m_currentBookMarkdown == markdown
        && m_annotationStatusText == statusText) {
        return;
    }

    m_currentBookIndex = index;
    m_currentBookTitle = title;
    m_currentBookAuthor = author;
    m_currentBookMarkdown = markdown;
    m_annotationStatusText = statusText;
    emit currentBookChanged();
}

void KoboLibrary::setStatusText(const QString &statusText)
{
    if (m_statusText == statusText)
        return;
    m_statusText = statusText;
    emit statusTextChanged();
}

QString KoboLibrary::normalizedDatabasePath(const QString &databasePath) const
{
    if (databasePath.trimmed().isEmpty())
        return {};

    const QFileInfo databaseInfo(databasePath);
    const QString canonicalPath = databaseInfo.canonicalFilePath();
    return QDir::cleanPath(canonicalPath.isEmpty() ? databaseInfo.absoluteFilePath() : canonicalPath);
}

QVariantMap KoboLibrary::manualDevice(const QString &databasePath) const
{
    const QFileInfo databaseInfo(databasePath);
    QDir deviceDirectory = databaseInfo.dir();
    if (deviceDirectory.dirName() == QLatin1String(".kobo"))
        deviceDirectory.cdUp();

    QString displayName = deviceDirectory.dirName().trimmed();
    if (displayName.isEmpty())
        displayName = databaseInfo.completeBaseName();
    if (displayName.isEmpty())
        displayName = tr("Kobo database");

    return {
        {QStringLiteral("displayName"), tr("%1 (manual)").arg(displayName)},
        {QStringLiteral("mountPath"), deviceDirectory.absolutePath()},
        {QStringLiteral("databasePath"), normalizedDatabasePath(databasePath)},
        {QStringLiteral("manual"), true},
    };
}
