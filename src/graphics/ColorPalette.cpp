#include "ColorPalette.h"
#include <algorithm>
#include <cmath>
#include <QProcess>
#include <QRegularExpression>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileSystemWatcher>

namespace AudioViewer {

ColorPalette::ColorPalette(QObject* parent)
    : QObject(parent)
    , m_themeBackgroundColor(QColor("#0a0a12"))
    , m_themeSurfaceColor(QColor("#12121e"))
    , m_themeAccentColor(QColor("#00e5ff"))
    , m_themeTextColor(QColor("#ffffff"))
    , m_themeMutedColor(QColor("#7a5868"))
{
    loadPreset("Cyberpunk");
    reloadThemeColors();

    m_themeWatcher = new QFileSystemWatcher(this);
    QString omarchyThemeFile = QDir::homePath() + "/.local/state/omarchy/current/theme/colors.toml";
    QString omarchyThemeDir = QDir::homePath() + "/.local/state/omarchy/current/theme";
    if (QFile::exists(omarchyThemeFile)) {
        m_themeWatcher->addPath(omarchyThemeFile);
    }
    if (QDir(omarchyThemeDir).exists()) {
        m_themeWatcher->addPath(omarchyThemeDir);
    }

    auto onThemeFileChanged = [this, omarchyThemeFile]() {
        reloadThemeColors();
        if (m_themeWatcher && QFile::exists(omarchyThemeFile) && !m_themeWatcher->files().contains(omarchyThemeFile)) {
            m_themeWatcher->addPath(omarchyThemeFile);
        }
    };
    connect(m_themeWatcher, &QFileSystemWatcher::fileChanged, this, onThemeFileChanged);
    connect(m_themeWatcher, &QFileSystemWatcher::directoryChanged, this, onThemeFileChanged);
}

bool ColorPalette::isHyprlandSession() const
{
    if (!qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")) {
        return true;
    }
    QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    if (desktop.contains("Hyprland", Qt::CaseInsensitive)) {
        return true;
    }
    return false;
}

bool ColorPalette::isEyedropperAvailable() const
{
    return !QStandardPaths::findExecutable("hyprpicker").isEmpty();
}

void ColorPalette::setColors(const QList<QColor>& colors)
{
    m_colors = colors;
    clampList();
    emit colorsChanged();
    emit countChanged();
}

void ColorPalette::clampList()
{
    if (m_colors.isEmpty()) {
        m_colors.append(QColor("#00e5ff"));
    } else if (m_colors.size() > 64) {
        m_colors = m_colors.mid(0, 64);
    }
}

QColor ColorPalette::sample(float t) const
{
    if (m_colors.isEmpty()) {
        return QColor("#00e5ff");
    }
    if (m_colors.size() == 1) {
        return m_colors.first();
    }

    t = std::clamp(t, 0.0f, 1.0f);
    float scaled = t * static_cast<float>(m_colors.size() - 1);
    int idx0 = static_cast<int>(std::floor(scaled));
    int idx1 = std::min(idx0 + 1, static_cast<int>(m_colors.size() - 1));
    float frac = scaled - static_cast<float>(idx0);

    const QColor& c0 = m_colors[idx0];
    const QColor& c1 = m_colors[idx1];

    int r = static_cast<int>(c0.red() + (c1.red() - c0.red()) * frac);
    int g = static_cast<int>(c0.green() + (c1.green() - c0.green()) * frac);
    int b = static_cast<int>(c0.blue() + (c1.blue() - c0.blue()) * frac);
    int a = static_cast<int>(c0.alpha() + (c1.alpha() - c0.alpha()) * frac);

    return QColor(r, g, b, a);
}

QStringList ColorPalette::availablePresets() const
{
    return {
        "Cyberpunk",
        "Synthwave",
        "Fire",
        "Matrix",
        "Ice",
        "Neon Sunset",
        "Emerald",
        "Monochrome"
    };
}

void ColorPalette::loadPreset(const QString& name)
{
    QList<QColor> list;

    if (name.compare("Synthwave", Qt::CaseInsensitive) == 0) {
        list = {
            QColor("#241734"),
            QColor("#2e2157"),
            QColor("#fd3a69"),
            QColor("#fecd1a"),
            QColor("#ff2a8d")
        };
    } else if (name.compare("Fire", Qt::CaseInsensitive) == 0) {
        list = {
            QColor("#3d0000"),
            QColor("#950101"),
            QColor("#ff0000"),
            QColor("#ff6000"),
            QColor("#ffea00"),
            QColor("#ffffff")
        };
    } else if (name.compare("Matrix", Qt::CaseInsensitive) == 0) {
        list = {
            QColor("#001a00"),
            QColor("#005b00"),
            QColor("#00bb00"),
            QColor("#00ff66"),
            QColor("#ccffdd")
        };
    } else if (name.compare("Ice", Qt::CaseInsensitive) == 0) {
        list = {
            QColor("#001f3f"),
            QColor("#0074d9"),
            QColor("#7fdbff"),
            QColor("#39cccc"),
            QColor("#e0f7fa")
        };
    } else if (name.compare("Neon Sunset", Qt::CaseInsensitive) == 0) {
        list = {
            QColor("#2b1055"),
            QColor("#7597de"),
            QColor("#f35588"),
            QColor("#ff8e71"),
            QColor("#ffe05d")
        };
    } else if (name.compare("Emerald", Qt::CaseInsensitive) == 0) {
        list = {
            QColor("#064420"),
            QColor("#007965"),
            QColor("#00af91"),
            QColor("#f4eeff")
        };
    } else if (name.compare("Monochrome", Qt::CaseInsensitive) == 0) {
        list = {
            QColor("#111111"),
            QColor("#555555"),
            QColor("#999999"),
            QColor("#dddddd"),
            QColor("#ffffff")
        };
    } else { // Cyberpunk por defecto
        list = {
            QColor("#0d0221"),
            QColor("#0f084b"),
            QColor("#26408b"),
            QColor("#a626d3"),
            QColor("#f205e2"),
            QColor("#00f0ff")
        };
    }

    setColors(list);
}

bool ColorPalette::addColor(const QColor& color)
{
    if (m_colors.size() >= 64 || !color.isValid()) {
        return false;
    }
    m_colors.append(color);
    emit colorsChanged();
    emit countChanged();
    return true;
}

bool ColorPalette::setColorAt(int index, const QColor& color)
{
    if (index >= 0 && index < m_colors.size() && color.isValid()) {
        m_colors[index] = color;
        emit colorsChanged();
        return true;
    }
    return false;
}

bool ColorPalette::removeColorAt(int index)
{
    if (index >= 0 && index < m_colors.size() && m_colors.size() > 1) {
        m_colors.removeAt(index);
        emit colorsChanged();
        emit countChanged();
        return true;
    }
    return false;
}

void ColorPalette::clear(const QColor& fallback)
{
    m_colors.clear();
    m_colors.append(fallback);
    emit colorsChanged();
    emit countChanged();
}

QList<QColor> ColorPalette::extractHyprlandColors() const
{
    QList<QColor> result;
    auto addUnique = [&result](const QString& hex) {
        QColor col(hex);
        if (col.isValid() && !result.contains(col)) {
            result.append(col);
        }
    };

    // 1. Extraer colores del tema de Omarchy si está disponible
    QString home = QDir::homePath();
    QString themeFilePath = home + "/.local/state/omarchy/current/theme/colors.toml";
    QFile themeFile(themeFilePath);
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray content = themeFile.readAll();
        themeFile.close();

        QRegularExpression re(R"(([a-zA-Z0-9_-]+)\s*=\s*["'](#[0-9a-fA-F]{6})["'])");
        QRegularExpressionMatchIterator it = re.globalMatch(QString::fromUtf8(content));

        QMap<QString, QString> keyMap;
        while (it.hasNext()) {
            auto match = it.next();
            keyMap.insert(match.captured(1).toLower(), match.captured(2).toLower());
        }

        const QStringList priority = {
            "accent", "cyan", "blue", "bright_blue",
            "magenta", "bright_magenta", "red", "bright_red",
            "orange", "yellow", "green", "bright_green"
        };
        for (const QString& k : priority) {
            if (keyMap.contains(k)) {
                addUnique(keyMap.value(k));
            }
        }
    }

    // 2. Extraer borde activo de Hyprland via hyprctl
    QProcess pActive;
    pActive.start("hyprctl", {"getoption", "general:col.active_border"});
    if (pActive.waitForFinished(400) && pActive.exitCode() == 0) {
        QString out = QString::fromUtf8(pActive.readAllStandardOutput());
        QRegularExpression hexRe(R"(([0-9a-fA-F]{8}))");
        QRegularExpressionMatchIterator hit = hexRe.globalMatch(out);
        while (hit.hasNext()) {
            QString m = hit.next().captured(1);
            QString hex = "#" + m.mid(2).toLower();
            QColor col(hex);
            if (col.isValid()) {
                if (result.contains(col)) {
                    result.removeAll(col);
                }
                result.prepend(col);
            }
        }
    }

    // 3. Extraer borde inactivo de Hyprland via hyprctl
    QProcess pInactive;
    pInactive.start("hyprctl", {"getoption", "general:col.inactive_border"});
    if (pInactive.waitForFinished(400) && pInactive.exitCode() == 0) {
        QString out = QString::fromUtf8(pInactive.readAllStandardOutput());
        QRegularExpression hexRe(R"(([0-9a-fA-F]{8}))");
        QRegularExpressionMatchIterator hit = hexRe.globalMatch(out);
        while (hit.hasNext()) {
            QString m = hit.next().captured(1);
            QString hex = "#" + m.mid(2).toLower();
            addUnique(hex);
        }
    }

    // Fallback en caso de que no haya colores detectados
    if (result.isEmpty()) {
        result = {
            QColor("#00adb8"),
            QColor("#0075b8"),
            QColor("#d947d9"),
            QColor("#de0046"),
            QColor("#00b871")
        };
    }

    return result;
}

bool ColorPalette::applyHyprlandTheme()
{
    QList<QColor> colors = extractHyprlandColors();
    if (!colors.isEmpty()) {
        setColors(colors);
        return true;
    }
    return false;
}

void ColorPalette::pickColorWithEyedropper(int targetIndex)
{
    QProcess* proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, targetIndex](int exitCode, QProcess::ExitStatus exitStatus) {
        proc->deleteLater();
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
            if (!out.isEmpty()) {
                if (!out.startsWith('#')) {
                    out.prepend('#');
                }
                QColor col(out);
                if (col.isValid()) {
                    if (targetIndex >= 0 && targetIndex < m_colors.size()) {
                        m_colors[targetIndex] = col;
                        emit colorsChanged();
                    } else if (m_colors.size() < 64) {
                        m_colors.append(col);
                        clampList();
                        emit colorsChanged();
                        emit countChanged();
                    }
                    emit eyedropperColorPicked(targetIndex, col);
                }
            }
        }
    });
    proc->start("hyprpicker", {"-f", "hex", "-b"});
}

void ColorPalette::setColorsFromHex(const QStringList& hexList)
{
    if (hexList.isEmpty()) return;
    QList<QColor> list;
    for (const QString& h : hexList) {
        QColor c(h);
        if (c.isValid()) {
            list.append(c);
        }
    }
    if (!list.isEmpty()) {
        setColors(list);
    }
}

QStringList ColorPalette::colorsAsHex() const
{
    QStringList res;
    for (const QColor& c : m_colors) {
        res.append(c.name(QColor::HexRgb));
    }
    return res;
}

void ColorPalette::reloadThemeColors()
{
    QColor newBg("#0a0a12");
    QColor newSurface("#12121e");
    QColor newAccent("#00e5ff");
    QColor newText("#ffffff");
    QColor newMuted("#7a5868");

    bool foundTheme = false;

    // 1. Extraer colores del tema Omarchy / Hyprland si está disponible
    QString home = QDir::homePath();
    QString themeFilePath = home + "/.local/state/omarchy/current/theme/colors.toml";
    QFile themeFile(themeFilePath);
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray content = themeFile.readAll();
        themeFile.close();

        QRegularExpression re(R"(([a-zA-Z0-9_-]+)\s*=\s*["']([^"']+)["'])");
        QRegularExpressionMatchIterator it = re.globalMatch(QString::fromUtf8(content));

        QMap<QString, QString> keyMap;
        while (it.hasNext()) {
            auto match = it.next();
            keyMap.insert(match.captured(1).toLower(), match.captured(2).trimmed());
        }

        // Color de fondo de ventana principal
        if (keyMap.contains("darker_background")) {
            QColor c(keyMap.value("darker_background"));
            if (c.isValid()) { newBg = c; foundTheme = true; }
        } else if (keyMap.contains("dark_background")) {
            QColor c(keyMap.value("dark_background"));
            if (c.isValid()) { newBg = c; foundTheme = true; }
        } else if (keyMap.contains("background")) {
            QColor c(keyMap.value("background"));
            if (c.isValid()) { newBg = c; foundTheme = true; }
        }

        // Superficie / paneles / diálogos
        if (keyMap.contains("dark_background")) {
            QColor c(keyMap.value("dark_background"));
            if (c.isValid()) newSurface = c;
        } else if (keyMap.contains("background")) {
            QColor c(keyMap.value("background"));
            if (c.isValid()) newSurface = c;
        }

        // Color de acento
        if (keyMap.contains("accent")) {
            QColor c(keyMap.value("accent"));
            if (c.isValid()) newAccent = c;
        }

        // Color de texto
        if (keyMap.contains("foreground")) {
            QColor c(keyMap.value("foreground"));
            if (c.isValid()) newText = c;
        }

        // Color atenuado (muted)
        if (keyMap.contains("muted")) {
            QColor c(keyMap.value("muted"));
            if (c.isValid()) newMuted = c;
        }
    }

    // 2. Extraer Pywal si Omarchy no está configurado
    if (!foundTheme) {
        QString walFilePath = home + "/.cache/wal/colors.json";
        QFile walFile(walFilePath);
        if (walFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonDocument doc = QJsonDocument::fromJson(walFile.readAll());
            walFile.close();
            if (doc.isObject()) {
                QJsonObject rootObj = doc.object();
                QJsonObject special = rootObj.value("special").toObject();
                QJsonObject colors = rootObj.value("colors").toObject();

                if (special.contains("background")) {
                    QColor c(special.value("background").toString());
                    if (c.isValid()) {
                        newBg = c;
                        newSurface = c.lighter(115);
                        foundTheme = true;
                    }
                }
                if (special.contains("foreground")) {
                    QColor c(special.value("foreground").toString());
                    if (c.isValid()) newText = c;
                }
                if (colors.contains("color4")) {
                    QColor c(colors.value("color4").toString());
                    if (c.isValid()) newAccent = c;
                } else if (colors.contains("color1")) {
                    QColor c(colors.value("color1").toString());
                    if (c.isValid()) newAccent = c;
                }
            }
        }
    }

    // 3. Si estamos en sesión Hyprland, verificar si hyprctl active_border provee el acento
    if (isHyprlandSession()) {
        QProcess pActive;
        pActive.start("hyprctl", {"getoption", "general:col.active_border"});
        if (pActive.waitForFinished(200) && pActive.exitCode() == 0) {
            QString out = QString::fromUtf8(pActive.readAllStandardOutput());
            QRegularExpression hexRe(R"(([0-9a-fA-F]{8}))");
            auto match = hexRe.match(out);
            if (match.hasMatch()) {
                QString m = match.captured(1);
                // Hyprland gradient hex formato es AARRGGBB
                QString hex = "#" + m.mid(2).toLower();
                QColor c(hex);
                if (c.isValid()) {
                    newAccent = c;
                }
            }
        }
    }

    bool changed = (m_themeBackgroundColor != newBg ||
                    m_themeSurfaceColor != newSurface ||
                    m_themeAccentColor != newAccent ||
                    m_themeTextColor != newText ||
                    m_themeMutedColor != newMuted);

    m_themeBackgroundColor = newBg;
    m_themeSurfaceColor = newSurface;
    m_themeAccentColor = newAccent;
    m_themeTextColor = newText;
    m_themeMutedColor = newMuted;

    if (changed) {
        emit themeColorsChanged();
    }
}

} // namespace AudioViewer
