#include "ConfigManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QThread>
#include <iostream>

namespace AudioViewer {

static const QStringList FACTORY_PRESET_NAMES = {
    QStringLiteral("Cyberpunk Neon"),
    QStringLiteral("Synthwave 80s"),
    QStringLiteral("Minimal Line"),
    QStringLiteral("Fire & Embers"),
    QStringLiteral("Deep Ocean"),
    QStringLiteral("Vertical Tower")
};

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
{
    ensurePresetsDirectoryExists();
}

void ConfigManager::setBackgroundModeRequested(bool req)
{
    if (m_bgMode != req) {
        m_bgMode = req;
        emit backgroundModeRequestedChanged();
    }
}

QString ConfigManager::configFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return dir + "/AudioViewer/config.json";
}

QString ConfigManager::presetsDirectoryPath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return dir + "/AudioViewer/presets";
}

void ConfigManager::ensurePresetsDirectoryExists() const
{
    QDir dir(presetsDirectoryPath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

bool ConfigManager::hasSavedConfig() const
{
    return QFile::exists(configFilePath());
}

qint64 ConfigManager::configLastModifiedMs() const
{
    QString path = configFilePath();
    QFileInfo info(path);
    if (!info.exists()) {
        return 0;
    }
    return info.lastModified().toMSecsSinceEpoch();
}

bool ConfigManager::saveConfig(const QJsonObject& config)
{
    QString path = configFilePath();
    QFileInfo info(path);
    QDir dir = info.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::cerr << "[ConfigManager] Error al abrir archivo para guardar: " 
                  << path.toStdString() << "\n";
        return false;
    }

    QJsonDocument doc(config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QJsonObject ConfigManager::loadConfig()
{
    QString path = configFilePath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return QJsonObject();
    }

    return doc.object();
}

QStringList ConfigManager::availablePresets() const
{
    QStringList result = FACTORY_PRESET_NAMES;

    QDir dir(presetsDirectoryPath());
    if (dir.exists()) {
        QStringList filters;
        filters << "*.json";
        QFileInfoList list = dir.entryInfoList(filters, QDir::Files, QDir::Name);
        for (const QFileInfo& fi : list) {
            QString name = fi.baseName();
            if (!result.contains(name)) {
                result.append(name);
            }
        }
    }

    return result;
}

bool ConfigManager::isFactoryPreset(const QString& name) const
{
    return FACTORY_PRESET_NAMES.contains(name);
}

QJsonObject ConfigManager::getFactoryPreset(const QString& name) const
{
    QJsonObject cfg;
    QJsonArray colors;

    if (name == QStringLiteral("Cyberpunk Neon")) {
        cfg["originAxis"] = 0; // X
        cfg["axisOffset"] = -300.0;
        cfg["frequencyLayout"] = 0; // CenterOut
        cfg["mirrorEnabled"] = true;
        cfg["invertDirection"] = false;
        cfg["showAxes"] = false;
        cfg["autoFillScreen"] = true;
        cfg["autoMaxThreshold"] = true;
        cfg["barSpacing"] = 1.0;
        cfg["barWidth"] = 16.0;
        cfg["maxBarLength"] = 260.0;
        cfg["tiltPitch"] = 45.0;
        cfg["tiltYaw"] = 0.0;
        cfg["perspectiveDepth"] = 450.0;
        cfg["residualEnabled"] = true;
        cfg["residualDecay"] = 0.92;
        cfg["residualOpacity"] = 0.50;
        cfg["residualOffset"] = 5.0;
        cfg["transparency"] = 1.0;
        cfg["colorMode"] = 2; // Both
        cfg["colorPreset"] = QStringLiteral("Personalizado");
        colors = {"#00f0ff", "#7000ff", "#ff0077", "#ff003c", "#ffe600"};
        cfg["customColors"] = colors;
        cfg["barCount"] = 64;
        cfg["sensitivity"] = 1.2;
        cfg["gravity"] = 0.88;
        cfg["minThreshold"] = 0.05;
        cfg["maxThreshold"] = 1.0;
        cfg["showThresholdLine"] = false;
    } else if (name == QStringLiteral("Synthwave 80s")) {
        cfg["originAxis"] = 0;
        cfg["axisOffset"] = -320.0;
        cfg["frequencyLayout"] = 0;
        cfg["mirrorEnabled"] = false;
        cfg["invertDirection"] = false;
        cfg["showAxes"] = false;
        cfg["autoFillScreen"] = true;
        cfg["autoMaxThreshold"] = true;
        cfg["barSpacing"] = 2.0;
        cfg["barWidth"] = 18.0;
        cfg["maxBarLength"] = 240.0;
        cfg["tiltPitch"] = 35.0;
        cfg["tiltYaw"] = 0.0;
        cfg["perspectiveDepth"] = 380.0;
        cfg["residualEnabled"] = true;
        cfg["residualDecay"] = 0.90;
        cfg["residualOpacity"] = 0.40;
        cfg["residualOffset"] = 4.0;
        cfg["transparency"] = 1.0;
        cfg["colorMode"] = 1; // GradientAcrossBars
        cfg["colorPreset"] = QStringLiteral("Personalizado");
        colors = {"#ff7700", "#ff007f", "#9900ff", "#00d4ff"};
        cfg["customColors"] = colors;
        cfg["barCount"] = 64;
        cfg["sensitivity"] = 1.1;
        cfg["gravity"] = 0.86;
        cfg["minThreshold"] = 0.05;
        cfg["maxThreshold"] = 1.0;
        cfg["showThresholdLine"] = false;
    } else if (name == QStringLiteral("Minimal Line")) {
        cfg["originAxis"] = 0;
        cfg["axisOffset"] = -350.0;
        cfg["frequencyLayout"] = 1; // Linear
        cfg["mirrorEnabled"] = false;
        cfg["invertDirection"] = false;
        cfg["showAxes"] = false;
        cfg["autoFillScreen"] = true;
        cfg["autoMaxThreshold"] = false;
        cfg["barSpacing"] = 4.0;
        cfg["barWidth"] = 6.0;
        cfg["maxBarLength"] = 180.0;
        cfg["tiltPitch"] = 0.0;
        cfg["tiltYaw"] = 0.0;
        cfg["perspectiveDepth"] = 0.0;
        cfg["residualEnabled"] = false;
        cfg["residualDecay"] = 0.85;
        cfg["residualOpacity"] = 0.20;
        cfg["residualOffset"] = 2.0;
        cfg["transparency"] = 0.90;
        cfg["colorMode"] = 0; // GradientAlongHeight
        cfg["colorPreset"] = QStringLiteral("Personalizado");
        colors = {"#ffffff", "#88ccff", "#00aaff"};
        cfg["customColors"] = colors;
        cfg["barCount"] = 64;
        cfg["sensitivity"] = 1.0;
        cfg["gravity"] = 0.90;
        cfg["minThreshold"] = 0.02;
        cfg["maxThreshold"] = 0.90;
        cfg["showThresholdLine"] = false;
    } else if (name == QStringLiteral("Fire & Embers")) {
        cfg["originAxis"] = 0;
        cfg["axisOffset"] = -300.0;
        cfg["frequencyLayout"] = 0;
        cfg["mirrorEnabled"] = true;
        cfg["invertDirection"] = false;
        cfg["showAxes"] = false;
        cfg["autoFillScreen"] = true;
        cfg["autoMaxThreshold"] = true;
        cfg["barSpacing"] = 1.0;
        cfg["barWidth"] = 16.0;
        cfg["maxBarLength"] = 250.0;
        cfg["tiltPitch"] = 50.0;
        cfg["tiltYaw"] = 0.0;
        cfg["perspectiveDepth"] = 420.0;
        cfg["residualEnabled"] = true;
        cfg["residualDecay"] = 0.93;
        cfg["residualOpacity"] = 0.60;
        cfg["residualOffset"] = 6.0;
        cfg["transparency"] = 1.0;
        cfg["colorMode"] = 0; // GradientAlongHeight
        cfg["colorPreset"] = QStringLiteral("Personalizado");
        colors = {"#ff2200", "#ff6600", "#ffaa00", "#ffee55", "#ffffff"};
        cfg["customColors"] = colors;
        cfg["barCount"] = 64;
        cfg["sensitivity"] = 1.3;
        cfg["gravity"] = 0.89;
        cfg["minThreshold"] = 0.05;
        cfg["maxThreshold"] = 1.0;
        cfg["showThresholdLine"] = false;
    } else if (name == QStringLiteral("Deep Ocean")) {
        cfg["originAxis"] = 0;
        cfg["axisOffset"] = -260.0;
        cfg["frequencyLayout"] = 0;
        cfg["mirrorEnabled"] = true;
        cfg["invertDirection"] = false;
        cfg["showAxes"] = false;
        cfg["autoFillScreen"] = true;
        cfg["autoMaxThreshold"] = true;
        cfg["barSpacing"] = 0.0;
        cfg["barWidth"] = 16.0;
        cfg["maxBarLength"] = 240.0;
        cfg["tiltPitch"] = 30.0;
        cfg["tiltYaw"] = 0.0;
        cfg["perspectiveDepth"] = 500.0;
        cfg["residualEnabled"] = true;
        cfg["residualDecay"] = 0.89;
        cfg["residualOpacity"] = 0.35;
        cfg["residualOffset"] = 4.0;
        cfg["transparency"] = 0.95;
        cfg["colorMode"] = 2; // Both
        cfg["colorPreset"] = QStringLiteral("Personalizado");
        colors = {"#001f3f", "#0074d9", "#7fdbff", "#39cccc", "#01ff70"};
        cfg["customColors"] = colors;
        cfg["barCount"] = 64;
        cfg["sensitivity"] = 1.15;
        cfg["gravity"] = 0.85;
        cfg["minThreshold"] = 0.05;
        cfg["maxThreshold"] = 1.0;
        cfg["showThresholdLine"] = false;
    } else if (name == QStringLiteral("Vertical Tower")) {
        cfg["originAxis"] = 1; // Y
        cfg["axisOffset"] = -400.0;
        cfg["frequencyLayout"] = 0;
        cfg["mirrorEnabled"] = true;
        cfg["invertDirection"] = false;
        cfg["showAxes"] = false;
        cfg["autoFillScreen"] = true;
        cfg["autoMaxThreshold"] = true;
        cfg["barSpacing"] = 1.0;
        cfg["barWidth"] = 14.0;
        cfg["maxBarLength"] = 240.0;
        cfg["tiltPitch"] = 0.0;
        cfg["tiltYaw"] = 45.0;
        cfg["perspectiveDepth"] = 400.0;
        cfg["residualEnabled"] = true;
        cfg["residualDecay"] = 0.91;
        cfg["residualOpacity"] = 0.45;
        cfg["residualOffset"] = 4.0;
        cfg["transparency"] = 1.0;
        cfg["colorMode"] = 2; // Both
        cfg["colorPreset"] = QStringLiteral("Personalizado");
        colors = {"#00ffaa", "#00b4d8", "#7209b7", "#f72585"};
        cfg["customColors"] = colors;
        cfg["barCount"] = 64;
        cfg["sensitivity"] = 1.2;
        cfg["gravity"] = 0.88;
        cfg["minThreshold"] = 0.05;
        cfg["maxThreshold"] = 1.0;
    }

    if (!cfg.isEmpty()) {
        if (!cfg.contains("reachPercent")) cfg["reachPercent"] = 1.0;
        if (!cfg.contains("attackRate")) cfg["attackRate"] = 0.45;
        if (!cfg.contains("spatialSmoothing")) cfg["spatialSmoothing"] = 0.32;
        if (cfg.value("gravity").toDouble(0.88) < 0.90) cfg["gravity"] = 0.92;
    }

    return cfg;
}

QJsonObject ConfigManager::loadPreset(const QString& name)
{
    if (isFactoryPreset(name)) {
        return getFactoryPreset(name);
    }

    QString filePath = QStringLiteral("%1/%2.json").arg(presetsDirectoryPath(), name);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "[ConfigManager] No se pudo abrir preset: " << filePath.toStdString() << "\n";
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return QJsonObject();
    }

    return doc.object();
}

bool ConfigManager::savePreset(const QString& name, const QJsonObject& config)
{
    if (name.trimmed().isEmpty()) {
        return false;
    }

    ensurePresetsDirectoryExists();
    QString filePath = QStringLiteral("%1/%2.json").arg(presetsDirectoryPath(), name.trimmed());
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        std::cerr << "[ConfigManager] No se pudo guardar preset: " << filePath.toStdString() << "\n";
        return false;
    }

    QJsonDocument doc(config);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    emit presetsChanged();
    return true;
}

bool ConfigManager::deletePreset(const QString& name)
{
    if (isFactoryPreset(name)) {
        // Los presets de fábrica no se eliminan
        return false;
    }

    QString filePath = QStringLiteral("%1/%2.json").arg(presetsDirectoryPath(), name);
    bool ok = QFile::remove(filePath);
    if (ok) {
        emit presetsChanged();
    }
    return ok;
}

bool ConfigManager::exportPreset(const QString& name, const QString& destinationFilePath)
{
    QJsonObject obj = loadPreset(name);
    if (obj.isEmpty()) return false;

    QFile file(destinationFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QJsonDocument doc(obj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool ConfigManager::importPreset(const QString& sourceFilePath)
{
    QFile file(sourceFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QFileInfo fi(sourceFilePath);
    QString name = fi.baseName();
    return savePreset(name, doc.object());
}

bool ConfigManager::isDesktopDaemonRunning() const
{
    QProcess p;
    p.start("sh", {"-c", "pgrep -f 'AudioViewer.*-b' || pgrep -f 'AudioViewer.*--background' || pgrep -f 'audioviewer-bg'"});
    p.waitForFinished(500);
    return (p.exitCode() == 0);
}

bool ConfigManager::startDesktopDaemon()
{
    if (isDesktopDaemonRunning()) {
        return true;
    }

    QString appBin = QDir::homePath() + "/.local/bin/audioviewer-bg";
    if (!QFile::exists(appBin)) {
        appBin = "/home/sergio/Projects/AudioViewer/build/AudioViewer";
    }

    bool ok = QProcess::startDetached(appBin, {"-b"});
    QThread::msleep(300);
    emit desktopDaemonStateChanged();
    return ok;
}

bool ConfigManager::stopDesktopDaemon()
{
    QProcess::execute("sh", {"-c", "pkill -f 'AudioViewer.*-b' || pkill -f 'AudioViewer.*--background' || pkill -f 'audioviewer-bg'"});
    QThread::msleep(200);
    emit desktopDaemonStateChanged();
    return true;
}

bool ConfigManager::restartDesktopDaemon()
{
    stopDesktopDaemon();
    QThread::msleep(300);
    return startDesktopDaemon();
}

} // namespace AudioViewer
