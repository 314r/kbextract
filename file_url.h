#pragma once

#include <QObject>
#include <QUrl>

// QML URL values do not expose QUrl::toLocalFile(). Keep native path and
// percent-encoding handling in Qt, including Windows drive letters and UNC paths.
class FileUrl : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    Q_INVOKABLE QString localPath(const QUrl &url) const
    {
        return url.isLocalFile() ? url.toLocalFile() : QString();
    }
};
