#pragma once

#include <QObject>
#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>
#include <QFileSystemWatcher>
#include <QtQml/qqmlregistration.h>

namespace AudioViewer {

class ColorPalette : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<QColor> colors READ colors WRITE setColors NOTIFY colorsChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool isHyprlandSession READ isHyprlandSession CONSTANT)
    Q_PROPERTY(bool isEyedropperAvailable READ isEyedropperAvailable CONSTANT)

    Q_PROPERTY(QColor themeBackgroundColor READ themeBackgroundColor NOTIFY themeColorsChanged)
    Q_PROPERTY(QColor themeSurfaceColor READ themeSurfaceColor NOTIFY themeColorsChanged)
    Q_PROPERTY(QColor themeAccentColor READ themeAccentColor NOTIFY themeColorsChanged)
    Q_PROPERTY(QColor themeTextColor READ themeTextColor NOTIFY themeColorsChanged)
    Q_PROPERTY(QColor themeMutedColor READ themeMutedColor NOTIFY themeColorsChanged)

public:
    explicit ColorPalette(QObject* parent = nullptr);

    QList<QColor> colors() const { return m_colors; }
    void setColors(const QList<QColor>& colors);

    int count() const { return m_colors.size(); }
    bool isHyprlandSession() const;
    bool isEyedropperAvailable() const;

    QColor themeBackgroundColor() const { return m_themeBackgroundColor; }
    QColor themeSurfaceColor() const { return m_themeSurfaceColor; }
    QColor themeAccentColor() const { return m_themeAccentColor; }
    QColor themeTextColor() const { return m_themeTextColor; }
    QColor themeMutedColor() const { return m_themeMutedColor; }

    // Interpola entre 0.0 y 1.0 dentro de la lista de 1 a 64 colores
    QColor sample(float t) const;

    // Presets integrados
    Q_INVOKABLE void loadPreset(const QString& name);
    Q_INVOKABLE QStringList availablePresets() const;

    // Métodos para agregar / editar / remover colores individualmente (hasta 64)
    Q_INVOKABLE bool addColor(const QColor& color);
    Q_INVOKABLE bool setColorAt(int index, const QColor& color);
    Q_INVOKABLE bool removeColorAt(int index);
    Q_INVOKABLE void clear(const QColor& fallback = QColor("#00e5ff"));

    // Integración con Hyprland y Cuenta Gotas (Eyedropper)
    Q_INVOKABLE bool applyHyprlandTheme();
    Q_INVOKABLE QList<QColor> extractHyprlandColors() const;
    Q_INVOKABLE void pickColorWithEyedropper(int targetIndex = -1);
    Q_INVOKABLE void reloadThemeColors();

    // Serialización Hex
    Q_INVOKABLE void setColorsFromHex(const QStringList& hexList);
    Q_INVOKABLE QStringList colorsAsHex() const;

signals:
    void colorsChanged();
    void countChanged();
    void eyedropperColorPicked(int index, const QColor& color);
    void themeColorsChanged();

private:
    void clampList();

    QList<QColor> m_colors;

    QColor m_themeBackgroundColor;
    QColor m_themeSurfaceColor;
    QColor m_themeAccentColor;
    QColor m_themeTextColor;
    QColor m_themeMutedColor;
    QFileSystemWatcher* m_themeWatcher{nullptr};
};

} // namespace AudioViewer
