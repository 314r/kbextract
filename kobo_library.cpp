#include "kobo_library.h"

#include <algorithm>

#include <QDir>
#include <QFile>
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
    qlonglong spineStart = 0;
    bool hasSpineStart = false;
};

struct ChapterAnnotations {
    QString title;
    QStringList annotations;
    QStringList obsidianAnnotations;
    QStringList plainTextAnnotations;
    qlonglong order = 0;
    bool hasOrder = false;
    int firstSeen = 0;
};

QString deviceDatabasePath(const QVariantMap &device)
{
    return device.value(QStringLiteral("databasePath")).toString();
}

QString koboReaderConfPath(const QString &databasePath)
{
    const QDir databaseDirectory = QFileInfo(databasePath).dir();
    if (databaseDirectory.dirName() != QLatin1String(".kobo"))
        return {};
    return databaseDirectory.filePath(QStringLiteral("Kobo/Kobo eReader.conf"));
}

QString readSerialNumber(const QString &confPath)
{
    if (confPath.isEmpty())
        return {};

    QFile conf(confPath);
    if (!conf.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    while (!conf.atEnd()) {
        const QString line = QString::fromUtf8(conf.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')) || line.startsWith(QLatin1Char(';'))
            || line.startsWith(QLatin1Char('['))) {
            continue;
        }
        const int separator = line.indexOf(QLatin1Char('='));
        if (separator < 0 || line.left(separator).trimmed() != QLatin1String("SerialNumber"))
            continue;
        const QString serial = line.mid(separator + 1).trimmed();
        if (!serial.isEmpty())
            return serial;
    }
    return {};
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
        const QString fragment = line.simplified();
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

QString obsidianHighlightBlock(const QString &text)
{
    return QStringLiteral("> [!quote]\n") + highlightBlock(text);
}

QString noteParagraph(const QString &text)
{
    return normalizedText(text);
}

QList<KoboVolume> mountedKoboVolumes()
{
    QList<KoboVolume> volumes;
    const QList<QStorageInfo> mountedVolumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : mountedVolumes) {
        if (!storage.isValid() || !storage.isReady())
            continue;

        volumes.append(KoboVolume{
            .rootPath = storage.rootPath(),
            .displayName = storage.displayName(),
        });
    }
    return volumes;
}

} // namespace

KoboLibrary::KoboLibrary(QObject *parent)
    : KoboLibrary(mountedKoboVolumes, 2000, defaultAnnotationStorePath(), parent)
{
}

KoboLibrary::KoboLibrary(const QString &libraryPath, QObject *parent)
    : KoboLibrary(mountedKoboVolumes, 2000, libraryPath, parent)
{
}

KoboLibrary::KoboLibrary(KoboVolumeProvider volumeProvider, int refreshIntervalMs, QObject *parent)
    : KoboLibrary(std::move(volumeProvider), refreshIntervalMs, defaultAnnotationStorePath(), parent)
{
}

KoboLibrary::KoboLibrary(KoboVolumeProvider volumeProvider, int refreshIntervalMs, const QString &libraryPath,
                         QObject *parent)
    : QObject(parent)
    , m_volumeProvider(std::move(volumeProvider))
    , m_connectionName(QStringLiteral("kbextract-kobo-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
    , m_annotationStore(libraryPath.isEmpty() ? defaultAnnotationStorePath() : libraryPath)
{
    connect(&m_deviceRefreshTimer, &QTimer::timeout, this, [this] {
        rebuildDevices();
    });
    m_deviceRefreshTimer.setInterval(qMax(1, refreshIntervalMs));
    m_deviceRefreshTimer.start();
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

QString KoboLibrary::currentBookObsidianMarkdown() const
{
    return m_currentBookObsidianMarkdown;
}

QString KoboLibrary::currentBookPlainText() const
{
    return m_currentBookPlainText;
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
    m_pendingDatabasePath.clear();

    if (index < 0 || index >= m_devices.size()) {
        clearDeviceSelection(tr("Connect a Kobo or choose KoboReader.sqlite."));
        return;
    }

    const QString oldPath = m_currentDeviceIndex >= 0 && m_currentDeviceIndex < m_devices.size()
        ? deviceDatabasePath(m_devices.at(m_currentDeviceIndex).toMap())
        : QString();
    const QString newPath = deviceDatabasePath(m_devices.at(index).toMap());
    const bool indexChanged = m_currentDeviceIndex != index;
    m_currentDeviceIndex = index;
    if (indexChanged)
        emit currentDeviceIndexChanged();

    if (normalizedDatabasePath(oldPath) != normalizedDatabasePath(newPath) || !m_database.isOpen())
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

    QList<StoredAnnotation> annotations;
    QString error;
    if (m_database.isOpen()) {
        if (!loadResolvedAnnotations(m_database, volumeId, &annotations, &error)) {
            setCurrentBookState(index, title, author, {}, {}, {}, error);
            return;
        }
    } else if (m_currentDeviceIndex >= 0 && m_currentDeviceIndex < m_devices.size()
               && m_devices.at(m_currentDeviceIndex).toMap().value(QStringLiteral("saved")).toBool()) {
        const QString deviceKey = m_devices.at(m_currentDeviceIndex).toMap().value(QStringLiteral("deviceKey")).toString();
        if (!m_annotationStore.annotations(deviceKey, volumeId, &annotations)) {
            setCurrentBookState(index, title, author, {}, {}, {},
                                tr("Could not read the saved highlight library: %1").arg(m_annotationStore.lastError()));
            return;
        }
    } else {
        setCurrentBookState(index, title, author, {}, {}, {}, tr("The Kobo database is not open."));
        return;
    }

    QString markdown;
    QString obsidianMarkdown;
    QString plainText;
    formatAnnotations(annotations, &markdown, &obsidianMarkdown, &plainText);
    const QString annotationStatus = markdown.isEmpty()
        ? tr("No visible highlights or notes for this book.")
        : QString();
    setCurrentBookState(index, title, author, markdown, obsidianMarkdown, plainText, annotationStatus);
}

bool KoboLibrary::loadResolvedAnnotations(QSqlDatabase &database, const QString &volumeId,
                                         QList<StoredAnnotation> *annotations, QString *error)
{
    annotations->clear();

    static const QString kepubChapterQueryText = QStringLiteral(R"SQL(
        SELECT
            navigation.ContentID,
            navigation.ChapterIDBookmarked,
            navigation.Title,
            navigation.VolumeIndex,
            target.VolumeIndex
        FROM content navigation
        LEFT JOIN content target
               ON target.ContentID = navigation.ChapterIDBookmarked
        WHERE CAST(navigation.ContentType AS INTEGER) = 899
          AND navigation.BookID = :volume_id
        ORDER BY CASE WHEN target.VolumeIndex IS NULL THEN 1 ELSE 0 END,
                 target.VolumeIndex,
                 COALESCE(navigation.VolumeIndex, 0),
                 navigation.ContentID COLLATE NOCASE
    )SQL");

    QSqlQuery kepubChapterQuery(database);
    kepubChapterQuery.setForwardOnly(true);
    if (!kepubChapterQuery.prepare(kepubChapterQueryText)) {
        if (error)
            *error = tr("Could not prepare the chapter query: %1").arg(kepubChapterQuery.lastError().text());
        return false;
    }
    kepubChapterQuery.bindValue(QStringLiteral(":volume_id"), volumeId);
    if (!kepubChapterQuery.exec()) {
        if (error)
            *error = tr("Could not read this book's chapters: %1").arg(kepubChapterQuery.lastError().text());
        return false;
    }

    QHash<QString, ChapterMetadata> kepubChapters;
    QList<ChapterMetadata> kepubChapterRanges;
    while (kepubChapterQuery.next()) {
        const QString contentId = normalizedText(kepubChapterQuery.value(0).toString());
        if (contentId.isEmpty())
            continue;

        const QString targetContentId = normalizedText(kepubChapterQuery.value(1).toString());
        ChapterMetadata metadata;
        metadata.key = targetContentId.isEmpty() ? kepubBookmarkChapterId(contentId) : targetContentId;
        metadata.title = singleLineText(kepubChapterQuery.value(2).toString());
        metadata.hasSpineStart = !kepubChapterQuery.value(4).isNull();
        if (metadata.hasSpineStart)
            metadata.spineStart = kepubChapterQuery.value(4).toLongLong();

        metadata.hasOrder = metadata.hasSpineStart || !kepubChapterQuery.value(3).isNull();
        if (metadata.hasSpineStart)
            metadata.order = metadata.spineStart;
        else if (metadata.hasOrder)
            metadata.order = kepubChapterQuery.value(3).toLongLong();

        kepubChapters.insert(contentId, metadata);
        kepubChapters.insert(kepubBookmarkChapterId(contentId), metadata);
        if (!metadata.key.isEmpty())
            kepubChapters.insert(metadata.key, metadata);
        if (metadata.hasSpineStart)
            kepubChapterRanges.append(metadata);
    }

    std::stable_sort(kepubChapterRanges.begin(), kepubChapterRanges.end(),
                     [](const ChapterMetadata &left, const ChapterMetadata &right) {
        return left.spineStart < right.spineStart;
    });

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
        ORDER BY CASE WHEN chapter.VolumeIndex IS NULL THEN 1 ELSE 0 END,
                 chapter.VolumeIndex,
                 COALESCE(bm.ChapterProgress, 0),
                 bm.ContentID COLLATE NOCASE,
                 bm.BookmarkID COLLATE NOCASE
    )SQL");

    QSqlQuery query(database);
    query.setForwardOnly(true);
    if (!query.prepare(queryText)) {
        if (error)
            *error = tr("Could not prepare the annotation query: %1").arg(query.lastError().text());
        return false;
    }
    query.bindValue(QStringLiteral(":volume_id"), volumeId);
    if (!query.exec()) {
        if (error)
            *error = tr("Could not read this book's annotations: %1").arg(query.lastError().text());
        return false;
    }

    while (query.next()) {
        const QString bookmarkId = normalizedText(query.value(0).toString());
        const QString contentId = normalizedText(query.value(1).toString());
        const QString highlightedText = normalizedText(query.value(2).toString());
        const QString noteText = normalizedText(query.value(3).toString());
        if (highlightedText.isEmpty() && noteText.isEmpty())
            continue;

        QString chapterKey = contentId;
        QString chapterTitle = singleLineText(query.value(4).toString());
        const bool hasContentOrder = !query.value(5).isNull();
        const qlonglong contentOrder = hasContentOrder ? query.value(5).toLongLong() : 0;
        bool hasChapterOrder = hasContentOrder;
        qlonglong chapterOrder = contentOrder;

        const ChapterMetadata *resolvedChapter = nullptr;
        const auto exactChapter = kepubChapters.constFind(contentId);
        if (exactChapter != kepubChapters.cend()) {
            resolvedChapter = &exactChapter.value();
        } else if (hasContentOrder) {
            for (const ChapterMetadata &candidate : std::as_const(kepubChapterRanges)) {
                if (candidate.spineStart > contentOrder)
                    break;
                resolvedChapter = &candidate;
            }
        }

        if (resolvedChapter) {
            chapterKey = resolvedChapter->key;
            if (!resolvedChapter->title.isEmpty())
                chapterTitle = resolvedChapter->title;
            if (resolvedChapter->hasOrder) {
                hasChapterOrder = true;
                chapterOrder = resolvedChapter->order;
            }
        }

        annotations->append(StoredAnnotation{
            .bookmarkId = bookmarkId,
            .highlightText = highlightedText,
            .noteText = noteText,
            .chapterKey = chapterKey,
            .chapterTitle = chapterTitle,
            .chapterOrder = chapterOrder,
            .hasChapterOrder = hasChapterOrder,
            .sortIndex = static_cast<int>(annotations->size()),
        });
    }
    return true;
}

void KoboLibrary::formatAnnotations(const QList<StoredAnnotation> &annotations, QString *markdown,
                                    QString *obsidianMarkdown, QString *plainText) const
{
    QList<ChapterAnnotations> chapters;
    QHash<QString, int> chapterIndexes;
    int anonymousChapterIndex = 0;
    for (const StoredAnnotation &annotation : annotations) {
        QStringList annotationParts;
        QStringList obsidianAnnotationParts;
        QStringList plainTextAnnotationParts;
        if (!annotation.highlightText.isEmpty()) {
            annotationParts.append(highlightBlock(annotation.highlightText));
            obsidianAnnotationParts.append(obsidianHighlightBlock(annotation.highlightText));
            plainTextAnnotationParts.append(reflowedHighlightText(annotation.highlightText));
        }
        if (!annotation.noteText.isEmpty()) {
            annotationParts.append(noteParagraph(annotation.noteText));
            obsidianAnnotationParts.append(noteParagraph(annotation.noteText));
            plainTextAnnotationParts.append(noteParagraph(annotation.noteText));
        }
        if (annotationParts.isEmpty())
            continue;

        QString groupKey;
        if (!annotation.chapterKey.isEmpty()) {
            groupKey = QStringLiteral("chapter:%1").arg(annotation.chapterKey);
        } else if (!annotation.bookmarkId.isEmpty()) {
            groupKey = QStringLiteral("bookmark:%1").arg(annotation.bookmarkId);
        } else {
            groupKey = QStringLiteral("anonymous:%1").arg(anonymousChapterIndex++);
        }

        int chapterIndex = chapterIndexes.value(groupKey, -1);
        if (chapterIndex < 0) {
            chapterIndex = chapters.size();
            chapterIndexes.insert(groupKey, chapterIndex);
            chapters.append(ChapterAnnotations{
                .title = annotation.chapterTitle,
                .annotations = {},
                .obsidianAnnotations = {},
                .plainTextAnnotations = {},
                .order = annotation.chapterOrder,
                .hasOrder = annotation.hasChapterOrder,
                .firstSeen = chapterIndex,
            });
        } else {
            if (chapters[chapterIndex].title.isEmpty() && !annotation.chapterTitle.isEmpty())
                chapters[chapterIndex].title = annotation.chapterTitle;
            if (!chapters[chapterIndex].hasOrder && annotation.hasChapterOrder) {
                chapters[chapterIndex].hasOrder = true;
                chapters[chapterIndex].order = annotation.chapterOrder;
            }
        }

        chapters[chapterIndex].annotations.append(annotationParts.join(QStringLiteral("\n\n")));
        chapters[chapterIndex].obsidianAnnotations.append(obsidianAnnotationParts.join(QStringLiteral("\n\n")));
        chapters[chapterIndex].plainTextAnnotations.append(plainTextAnnotationParts.join(QStringLiteral("\n\n")));
    }

    std::stable_sort(chapters.begin(), chapters.end(), [](const ChapterAnnotations &left, const ChapterAnnotations &right) {
        if (left.hasOrder && right.hasOrder && left.order != right.order)
            return left.order < right.order;
        if (left.hasOrder != right.hasOrder)
            return left.hasOrder;
        return left.firstSeen < right.firstSeen;
    });

    QStringList chapterSections;
    QStringList obsidianChapterSections;
    QStringList plainTextChapterSections;
    for (const ChapterAnnotations &chapter : std::as_const(chapters)) {
        const QString chapterTitle = chapter.title.isEmpty() ? tr("Untitled chapter") : chapter.title;
        const QString heading = QStringLiteral("## ") + chapterTitle + QStringLiteral("\n\n");
        chapterSections.append(heading + chapter.annotations.join(QStringLiteral("\n\n\n")));
        obsidianChapterSections.append(heading + chapter.obsidianAnnotations.join(QStringLiteral("\n\n\n")));
        plainTextChapterSections.append(chapterTitle + QStringLiteral("\n\n")
                                        + chapter.plainTextAnnotations.join(QStringLiteral("\n\n\n")));
    }

    if (markdown)
        *markdown = chapterSections.join(QStringLiteral("\n\n\n"));
    if (obsidianMarkdown)
        *obsidianMarkdown = obsidianChapterSections.join(QStringLiteral("\n\n\n"));
    if (plainText)
        *plainText = plainTextChapterSections.join(QStringLiteral("\n\n\n"));
}

void KoboLibrary::refreshDevices()
{
    QString preferredPath;
    if (m_currentDeviceIndex >= 0 && m_currentDeviceIndex < m_devices.size())
        preferredPath = deviceDatabasePath(m_devices.at(m_currentDeviceIndex).toMap());
    else
        preferredPath = m_pendingDatabasePath;

    rebuildDevices(preferredPath, true);
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

    rebuildDevices(normalizedPath, true);
    return m_currentLoadSucceeded;
}

QVariantList KoboLibrary::discoveredDevices() const
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

    const QList<KoboVolume> mountedVolumes = m_volumeProvider ? m_volumeProvider() : QList<KoboVolume>();
    for (const KoboVolume &storage : mountedVolumes) {
        const QString mountPath = QDir::cleanPath(storage.rootPath);
        const QString databasePath = normalizedDatabasePath(QDir(mountPath).filePath(QString::fromLatin1(databaseRelativePath)));
        const QFileInfo databaseInfo(databasePath);
        if (!databaseInfo.exists() || !databaseInfo.isFile() || !databaseInfo.isReadable())
            continue;

        QString displayName = storage.displayName.trimmed();
        if (displayName.isEmpty())
            displayName = QFileInfo(mountPath).fileName();
        if (displayName.isEmpty())
            displayName = tr("Kobo eReader");

        appendDevice(deviceRecord(displayName, mountPath, databasePath, false));
    }

    std::sort(devices.begin(), devices.end(), [](const QVariant &left, const QVariant &right) {
        const QVariantMap leftDevice = left.toMap();
        const QVariantMap rightDevice = right.toMap();
        const int nameOrder = QString::localeAwareCompare(
            leftDevice.value(QStringLiteral("displayName")).toString(),
            rightDevice.value(QStringLiteral("displayName")).toString());
        if (nameOrder != 0)
            return nameOrder < 0;
        return deviceDatabasePath(leftDevice) < deviceDatabasePath(rightDevice);
    });

    for (const QVariant &deviceValue : std::as_const(m_manualDevices)) {
        const QVariantMap device = deviceValue.toMap();
        const QFileInfo databaseInfo(deviceDatabasePath(device));
        if (databaseInfo.exists() && databaseInfo.isFile() && databaseInfo.isReadable())
            appendDevice(device);
    }

    QVariantList savedDevices;
    const QList<StoredLibrary> libraries = m_annotationStore.libraries();
    for (const StoredLibrary &library : libraries) {
        const QString savedPath = normalizedDatabasePath(library.databasePath);
        bool covered = false;
        for (const QVariant &deviceValue : devices) {
            const QVariantMap device = deviceValue.toMap();
            const bool sameKey = device.value(QStringLiteral("deviceKey")).toString() == library.deviceKey;
            const bool sameSerial = !library.serial.isEmpty()
                && device.value(QStringLiteral("serial")).toString() == library.serial;
            const bool samePath = !savedPath.isEmpty()
                && normalizedDatabasePath(deviceDatabasePath(device)) == savedPath;
            if (sameKey || sameSerial || samePath) {
                covered = true;
                break;
            }
        }
        if (!covered)
            savedDevices.append(savedDevice(library));
    }

    std::sort(savedDevices.begin(), savedDevices.end(), [](const QVariant &left, const QVariant &right) {
        const QVariantMap leftDevice = left.toMap();
        const QVariantMap rightDevice = right.toMap();
        const int nameOrder = QString::localeAwareCompare(
            leftDevice.value(QStringLiteral("displayName")).toString(),
            rightDevice.value(QStringLiteral("displayName")).toString());
        if (nameOrder != 0)
            return nameOrder < 0;
        return leftDevice.value(QStringLiteral("deviceKey")).toString()
            < rightDevice.value(QStringLiteral("deviceKey")).toString();
    });
    for (const QVariant &saved : savedDevices)
        devices.append(saved);

    return devices;
}

void KoboLibrary::rebuildDevices(const QString &preferredDatabasePath, bool forceReload)
{
    const QVariantMap oldDevice = m_currentDeviceIndex >= 0 && m_currentDeviceIndex < m_devices.size()
        ? m_devices.at(m_currentDeviceIndex).toMap()
        : QVariantMap();
    const QString oldPath = normalizedDatabasePath(deviceDatabasePath(oldDevice));
    const QString oldKey = oldDevice.value(QStringLiteral("deviceKey")).toString();
    const bool oldSaved = oldDevice.value(QStringLiteral("saved")).toBool();
    const QString requestedPath = normalizedDatabasePath(preferredDatabasePath);
    const QVariantList devices = discoveredDevices();

    if (devices == m_devices) {
        if (forceReload && m_currentDeviceIndex >= 0) {
            const QVariantMap current = m_devices.at(m_currentDeviceIndex).toMap();
            snapshotOtherLiveDevices(current.value(QStringLiteral("saved")).toBool()
                                         ? QString()
                                         : deviceDatabasePath(current));
            loadCurrentDatabase();
        } else if (forceReload && m_currentDeviceIndex < 0 && m_pendingDatabasePath.isEmpty()) {
            setStatusText(tr("Connect a Kobo or choose KoboReader.sqlite."));
        }
        return;
    }

    m_devices = devices;
    emit devicesChanged();

    QString targetPath = requestedPath;
    if (targetPath.isEmpty() && !m_pendingDatabasePath.isEmpty())
        targetPath = normalizedDatabasePath(m_pendingDatabasePath);
    if (targetPath.isEmpty())
        targetPath = oldPath;

    int selectedIndex = -1;
    if (!targetPath.isEmpty()) {
        for (int index = 0; index < m_devices.size(); ++index) {
            if (normalizedDatabasePath(deviceDatabasePath(m_devices.at(index).toMap())) == targetPath) {
                selectedIndex = index;
                break;
            }
        }
    }

    if (selectedIndex < 0 && !oldKey.isEmpty()) {
        for (int index = 0; index < m_devices.size(); ++index) {
            if (m_devices.at(index).toMap().value(QStringLiteral("deviceKey")).toString() == oldKey) {
                selectedIndex = index;
                break;
            }
        }
    }

    if (selectedIndex < 0 && oldPath.isEmpty() && oldKey.isEmpty() && m_pendingDatabasePath.isEmpty() && !m_devices.isEmpty()) {
        const bool anyLive = std::any_of(m_devices.cbegin(), m_devices.cend(), [](const QVariant &deviceValue) {
            return !deviceValue.toMap().value(QStringLiteral("saved")).toBool();
        });
        if (!anyLive) {
            const QString lastOpenedKey = m_annotationStore.lastOpenedDeviceKey();
            if (!lastOpenedKey.isEmpty()) {
                for (int index = 0; index < m_devices.size(); ++index) {
                    if (m_devices.at(index).toMap().value(QStringLiteral("deviceKey")).toString() == lastOpenedKey) {
                        selectedIndex = index;
                        break;
                    }
                }
            }
        }
        if (selectedIndex < 0)
            selectedIndex = 0;
    }

    if (selectedIndex < 0) {
        if (!oldPath.isEmpty())
            m_pendingDatabasePath = oldPath;
        clearDeviceSelection(tr("The selected Kobo is no longer connected."));
        return;
    }

    const QVariantMap selected = m_devices.at(selectedIndex).toMap();
    const QString selectedPath = normalizedDatabasePath(deviceDatabasePath(selected));
    const QString selectedKey = selected.value(QStringLiteral("deviceKey")).toString();
    const bool selectedSaved = selected.value(QStringLiteral("saved")).toBool();
    if (selectedPath == normalizedDatabasePath(m_pendingDatabasePath))
        m_pendingDatabasePath.clear();

    const bool indexChanged = m_currentDeviceIndex != selectedIndex;
    m_currentDeviceIndex = selectedIndex;
    if (indexChanged)
        emit currentDeviceIndexChanged();

    // The live book list was just synced. Keep it, and the open text, when the
    // volume disappears and the same device's snapshot takes its place.
    if (selectedSaved && !oldSaved && selectedKey == oldKey && !selectedKey.isEmpty()) {
        snapshotOtherLiveDevices({});
        closeDatabase();
        m_annotationStore.markLastOpened(selectedKey);
        setStatusText(savedStatusText(selected.value(QStringLiteral("libraryName")).toString()));
        return;
    }

    if (forceReload || oldPath != selectedPath || oldKey != selectedKey || selectedSaved != oldSaved || !m_database.isOpen()) {
        snapshotOtherLiveDevices(selectedSaved ? QString() : selectedPath);
        loadCurrentDatabase();
    } else {
        // A newly connected device is snapshotted here without reloading the
        // open book or moving the selection.
        snapshotOtherLiveDevices(selectedPath);
        if (!selectedKey.isEmpty())
            m_annotationStore.markLastOpened(selectedKey);
    }
}

void KoboLibrary::clearDeviceSelection(const QString &statusText)
{
    const bool indexChanged = m_currentDeviceIndex != -1;
    m_currentDeviceIndex = -1;
    m_currentLoadSucceeded = false;
    closeDatabase();
    setBooks({});
    setStatusText(statusText);
    if (indexChanged)
        emit currentDeviceIndexChanged();
}

StoredLibrary KoboLibrary::libraryRecordForDevice(const QVariantMap &device) const
{
    const QString displayName = device.value(QStringLiteral("libraryName")).toString().isEmpty()
        ? device.value(QStringLiteral("displayName")).toString()
        : device.value(QStringLiteral("libraryName")).toString();
    StoredLibrary snapshot;
    snapshot.deviceKey = device.value(QStringLiteral("deviceKey")).toString();
    snapshot.displayName = displayName;
    snapshot.serial = device.value(QStringLiteral("serial")).toString();
    snapshot.databasePath = normalizedDatabasePath(deviceDatabasePath(device));
    if (snapshot.deviceKey.isEmpty())
        snapshot.deviceKey = snapshot.serial.isEmpty() ? snapshot.databasePath : snapshot.serial;
    return snapshot;
}

bool KoboLibrary::readAnnotatedBooks(QSqlDatabase &database, QVariantList *books, QList<StoredBook> *storedBooks,
                                     QString *error)
{
    books->clear();
    storedBooks->clear();

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

    QSqlQuery query(database);
    query.setForwardOnly(true);
    if (!query.exec(queryText)) {
        if (error)
            *error = tr("Could not read Kobo highlights: %1").arg(query.lastError().text());
        return false;
    }

    while (query.next()) {
        books->append(QVariantMap{
            {QStringLiteral("volumeId"), query.value(QStringLiteral("volume_id"))},
            {QStringLiteral("title"), query.value(QStringLiteral("title"))},
            {QStringLiteral("author"), query.value(QStringLiteral("author"))},
            {QStringLiteral("highlightCount"), query.value(QStringLiteral("highlight_count"))},
            {QStringLiteral("noteCount"), query.value(QStringLiteral("note_count"))},
        });
    }

    storedBooks->reserve(books->size());
    for (const QVariant &bookValue : std::as_const(*books)) {
        const QVariantMap book = bookValue.toMap();
        StoredBook storedBook;
        storedBook.volumeId = book.value(QStringLiteral("volumeId")).toString();
        storedBook.title = book.value(QStringLiteral("title")).toString();
        storedBook.author = book.value(QStringLiteral("author")).toString();
        storedBook.highlightCount = book.value(QStringLiteral("highlightCount")).toInt();
        storedBook.noteCount = book.value(QStringLiteral("noteCount")).toInt();
        if (!loadResolvedAnnotations(database, storedBook.volumeId, &storedBook.annotations, error))
            return false;
        storedBooks->append(std::move(storedBook));
    }
    return true;
}

bool KoboLibrary::snapshotDevice(const QVariantMap &device, QString *error)
{
    const QString connectionName = m_connectionName + QStringLiteral("-snapshot-")
        + QUuid::createUuid().toString(QUuid::Id128);
    bool saved = false;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        database.setDatabaseName(deviceDatabasePath(device));
        if (!database.open()) {
            if (error)
                *error = tr("Could not open the Kobo database: %1").arg(database.lastError().text());
        } else {
            QVariantList books;
            QList<StoredBook> storedBooks;
            if (!readAnnotatedBooks(database, &books, &storedBooks, error)) {
                // error already set
            } else if (!m_annotationStore.replaceLibrary(libraryRecordForDevice(device), storedBooks)) {
                if (error)
                    *error = tr("Could not save the highlight library: %1").arg(m_annotationStore.lastError());
            } else {
                saved = true;
            }
            database.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);
    return saved;
}

bool KoboLibrary::snapshotOtherLiveDevices(const QString &currentDatabasePath)
{
    const QString skipPath = normalizedDatabasePath(currentDatabasePath);
    for (const QVariant &deviceValue : std::as_const(m_devices)) {
        const QVariantMap device = deviceValue.toMap();
        if (device.value(QStringLiteral("saved")).toBool())
            continue;
        const QString databasePath = normalizedDatabasePath(deviceDatabasePath(device));
        if (!skipPath.isEmpty() && databasePath == skipPath)
            continue;
        QString error;
        if (!snapshotDevice(device, &error)) {
            setStatusText(error);
            return false;
        }
    }
    return true;
}

bool KoboLibrary::loadCurrentDatabase()
{
    if (m_currentDeviceIndex < 0 || m_currentDeviceIndex >= m_devices.size()) {
        m_currentLoadSucceeded = false;
        setBooks({});
        closeDatabase();
        setStatusText(tr("Connect a Kobo or choose KoboReader.sqlite."));
        return false;
    }

    const QVariantMap device = m_devices.at(m_currentDeviceIndex).toMap();
    if (device.value(QStringLiteral("saved")).toBool())
        return showSavedLibrary(device);

    m_currentLoadSucceeded = false;
    setBooks({});
    closeDatabase();

    const QString databasePath = deviceDatabasePath(device);
    const QString displayName = device.value(QStringLiteral("libraryName")).toString().isEmpty()
        ? device.value(QStringLiteral("displayName")).toString()
        : device.value(QStringLiteral("libraryName")).toString();

    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    m_database.setDatabaseName(databasePath);
    if (!m_database.open()) {
        setStatusText(tr("Could not open the Kobo database: %1").arg(m_database.lastError().text()));
        closeDatabase();
        return false;
    }

    QVariantList books;
    QList<StoredBook> storedBooks;
    QString readError;
    if (!readAnnotatedBooks(m_database, &books, &storedBooks, &readError)) {
        setStatusText(readError);
        return false;
    }

    // Highlight sets are small, so the snapshot is written on this thread at the
    // end of a successful read. Other connected devices were snapshotted first,
    // on their own read-only connections, so this write keeps last_opened here.
    if (!m_annotationStore.replaceLibrary(libraryRecordForDevice(device), storedBooks)) {
        setBooks(std::move(books));
        setStatusText(tr("Could not save the highlight library: %1").arg(m_annotationStore.lastError()));
        return false;
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
    setCurrentBookState(-1, {}, {}, {}, {}, {}, {});
}

void KoboLibrary::setCurrentBookState(int index, const QString &title, const QString &author,
                                      const QString &markdown, const QString &obsidianMarkdown,
                                      const QString &plainText, const QString &statusText)
{
    if (m_currentBookIndex == index
        && m_currentBookTitle == title
        && m_currentBookAuthor == author
        && m_currentBookMarkdown == markdown
        && m_currentBookObsidianMarkdown == obsidianMarkdown
        && m_currentBookPlainText == plainText
        && m_annotationStatusText == statusText) {
        return;
    }

    m_currentBookIndex = index;
    m_currentBookTitle = title;
    m_currentBookAuthor = author;
    m_currentBookMarkdown = markdown;
    m_currentBookObsidianMarkdown = obsidianMarkdown;
    m_currentBookPlainText = plainText;
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

    return deviceRecord(tr("%1 (manual)").arg(displayName), deviceDirectory.absolutePath(), databasePath, true);
}

QVariantMap KoboLibrary::deviceRecord(const QString &displayName, const QString &mountPath, const QString &databasePath,
                                      bool manual) const
{
    const QString normalizedPath = normalizedDatabasePath(databasePath);
    const QString serial = readSerialNumber(koboReaderConfPath(normalizedPath));
    return {
        {QStringLiteral("displayName"), displayName},
        {QStringLiteral("libraryName"), displayName},
        {QStringLiteral("mountPath"), mountPath},
        {QStringLiteral("databasePath"), normalizedPath},
        {QStringLiteral("manual"), manual},
        {QStringLiteral("saved"), false},
        {QStringLiteral("serial"), serial},
        {QStringLiteral("deviceKey"), serial.isEmpty() ? normalizedPath : serial},
    };
}

QVariantMap KoboLibrary::savedDevice(const StoredLibrary &library) const
{
    const QString libraryName = library.displayName.isEmpty() ? tr("Kobo eReader") : library.displayName;
    QDir mountDirectory = QFileInfo(library.databasePath).dir();
    if (mountDirectory.dirName() == QLatin1String(".kobo"))
        mountDirectory.cdUp();
    return {
        {QStringLiteral("displayName"), tr("%1 (saved)").arg(libraryName)},
        {QStringLiteral("libraryName"), libraryName},
        {QStringLiteral("mountPath"), mountDirectory.absolutePath()},
        {QStringLiteral("databasePath"), library.databasePath},
        {QStringLiteral("manual"), false},
        {QStringLiteral("saved"), true},
        {QStringLiteral("serial"), library.serial},
        {QStringLiteral("deviceKey"), library.deviceKey},
    };
}

QString KoboLibrary::savedStatusText(const QString &libraryName) const
{
    const QString name = libraryName.isEmpty() ? tr("Kobo eReader") : libraryName;
    return tr("Showing saved highlights from %1. Connect the Kobo to update.").arg(name);
}

bool KoboLibrary::showSavedLibrary(const QVariantMap &device)
{
    m_currentLoadSucceeded = false;
    closeDatabase();

    const QString deviceKey = device.value(QStringLiteral("deviceKey")).toString();
    QList<StoredBook> storedBooks;
    if (!m_annotationStore.books(deviceKey, &storedBooks)) {
        setBooks({});
        setStatusText(tr("Could not read the saved highlight library: %1").arg(m_annotationStore.lastError()));
        return false;
    }

    QVariantList books;
    for (const StoredBook &book : storedBooks) {
        books.append(QVariantMap{
            {QStringLiteral("volumeId"), book.volumeId},
            {QStringLiteral("title"), book.title},
            {QStringLiteral("author"), book.author},
            {QStringLiteral("highlightCount"), book.highlightCount},
            {QStringLiteral("noteCount"), book.noteCount},
        });
    }

    setBooks(std::move(books));
    m_annotationStore.markLastOpened(deviceKey);
    m_currentLoadSucceeded = true;
    setStatusText(savedStatusText(device.value(QStringLiteral("libraryName")).toString()));
    return true;
}
