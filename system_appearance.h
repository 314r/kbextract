#pragma once

#include <QFont>
#include <QObject>
#include <QVariantMap>

class QPalette;

class SystemAppearance : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap palette READ palette NOTIFY paletteChanged)
    Q_PROPERTY(QFont uiFont READ uiFont NOTIFY fontsChanged)
    Q_PROPERTY(QFont fixedFont READ fixedFont NOTIFY fontsChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY paletteChanged)

public:
    explicit SystemAppearance(QObject *parent = nullptr);

    QVariantMap palette() const;
    QFont uiFont() const;
    QFont fixedFont() const;
    bool dark() const;

    static QVariantMap paletteFrom(const QPalette &palette, bool dark);

    Q_INVOKABLE void reload();

signals:
    void paletteChanged();
    void fontsChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QVariantMap m_palette;
    QFont m_uiFont;
    QFont m_fixedFont;
    bool m_dark = false;
};
