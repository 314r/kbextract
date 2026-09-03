#pragma once

#include <QObject>
#include <QString>

class ClipboardHelper : public QObject
{
    Q_OBJECT

public:
    explicit ClipboardHelper(QObject *parent = nullptr);

    Q_INVOKABLE bool copyText(const QString &text) const;
};
