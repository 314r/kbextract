#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantMap>

class OmarchyTheme : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap palette READ palette NOTIFY paletteChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY paletteChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)

public:
    explicit OmarchyTheme(QObject *parent = nullptr);
    explicit OmarchyTheme(const QString &stateRoot, QObject *parent = nullptr);

    QVariantMap palette() const;
    bool dark() const;
    QString name() const;

    Q_INVOKABLE void reload();

signals:
    void paletteChanged();
    void nameChanged();

private:
    QVariantMap m_palette;
    bool m_dark = true;
    QString m_name;
    QString m_stateRoot;
    QTimer m_refreshTimer;
};
