#pragma once

#include <functional>

#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "annotation_store.h"

struct KoboVolume {
    QString rootPath;
    QString displayName;
};

using KoboVolumeProvider = std::function<QList<KoboVolume>()>;

class KoboLibrary : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList books READ books NOTIFY booksChanged)
    Q_PROPERTY(int currentDeviceIndex READ currentDeviceIndex WRITE setCurrentDeviceIndex NOTIFY currentDeviceIndexChanged)
    Q_PROPERTY(int currentBookIndex READ currentBookIndex WRITE setCurrentBookIndex NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookTitle READ currentBookTitle NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookAuthor READ currentBookAuthor NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookMarkdown READ currentBookMarkdown NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookObsidianMarkdown READ currentBookObsidianMarkdown NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookPlainText READ currentBookPlainText NOTIFY currentBookChanged)
    Q_PROPERTY(QString annotationStatusText READ annotationStatusText NOTIFY currentBookChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit KoboLibrary(QObject *parent = nullptr);
    explicit KoboLibrary(const QString &libraryPath, QObject *parent = nullptr);
    KoboLibrary(KoboVolumeProvider volumeProvider, int refreshIntervalMs, QObject *parent = nullptr);
    KoboLibrary(KoboVolumeProvider volumeProvider, int refreshIntervalMs, const QString &libraryPath,
                QObject *parent = nullptr);
    ~KoboLibrary() override;

    QVariantList devices() const;
    QVariantList books() const;
    int currentDeviceIndex() const;
    int currentBookIndex() const;
    QString currentBookTitle() const;
    QString currentBookAuthor() const;
    QString currentBookMarkdown() const;
    QString currentBookObsidianMarkdown() const;
    QString currentBookPlainText() const;
    QString annotationStatusText() const;
    QString statusText() const;

    void setCurrentDeviceIndex(int index);
    void setCurrentBookIndex(int index);

    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE bool addDatabase(const QString &databasePath);

signals:
    void devicesChanged();
    void booksChanged();
    void currentDeviceIndexChanged();
    void currentBookChanged();
    void statusTextChanged();

private:
    void rebuildDevices(const QString &preferredDatabasePath = {}, bool forceReload = false);
    QVariantList discoveredDevices() const;
    void clearDeviceSelection(const QString &statusText);
    bool loadCurrentDatabase();
    void closeDatabase();
    void setBooks(QVariantList books);
    void clearCurrentBook();
    void setCurrentBookState(int index, const QString &title, const QString &author,
                             const QString &markdown, const QString &obsidianMarkdown,
                             const QString &plainText, const QString &statusText);
    void setStatusText(const QString &statusText);
    QString normalizedDatabasePath(const QString &databasePath) const;
    QVariantMap manualDevice(const QString &databasePath) const;
    QVariantMap deviceRecord(const QString &displayName, const QString &mountPath, const QString &databasePath,
                             bool manual) const;
    QVariantMap savedDevice(const StoredLibrary &library) const;
    QString savedStatusText(const QString &libraryName) const;
    bool showSavedLibrary(const QVariantMap &device);
    bool loadResolvedAnnotations(const QString &volumeId, QList<StoredAnnotation> *annotations, QString *error);
    void formatAnnotations(const QList<StoredAnnotation> &annotations, QString *markdown, QString *obsidianMarkdown,
                           QString *plainText) const;

    QVariantList m_devices;
    QVariantList m_manualDevices;
    KoboVolumeProvider m_volumeProvider;
    QTimer m_deviceRefreshTimer;
    QString m_pendingDatabasePath;
    QVariantList m_books;
    int m_currentDeviceIndex = -1;
    int m_currentBookIndex = -1;
    QString m_currentBookTitle;
    QString m_currentBookAuthor;
    QString m_currentBookMarkdown;
    QString m_currentBookObsidianMarkdown;
    QString m_currentBookPlainText;
    QString m_annotationStatusText;
    QString m_statusText;
    QString m_connectionName;
    QSqlDatabase m_database;
    bool m_currentLoadSucceeded = false;
    AnnotationStore m_annotationStore;
};
