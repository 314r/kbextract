#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

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
    void rebuildDevices(const QString &preferredDatabasePath = {});
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

    QVariantList m_devices;
    QVariantList m_manualDevices;
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
};
