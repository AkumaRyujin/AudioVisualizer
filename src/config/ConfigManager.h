#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

namespace AudioViewer {

class ConfigManager : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool backgroundModeRequested READ backgroundModeRequested WRITE setBackgroundModeRequested NOTIFY backgroundModeRequestedChanged)
    Q_PROPERTY(QString configFilePath READ configFilePath CONSTANT)
    Q_PROPERTY(QString presetsDirectoryPath READ presetsDirectoryPath CONSTANT)
    Q_PROPERTY(QStringList availablePresets READ availablePresets NOTIFY presetsChanged)
    Q_PROPERTY(bool isDesktopDaemonRunning READ isDesktopDaemonRunning NOTIFY desktopDaemonStateChanged)

public:
    explicit ConfigManager(QObject* parent = nullptr);

    bool backgroundModeRequested() const { return m_bgMode; }
    void setBackgroundModeRequested(bool req);

    QString configFilePath() const;
    QString presetsDirectoryPath() const;

    Q_INVOKABLE bool saveConfig(const QJsonObject& config);
    Q_INVOKABLE QJsonObject loadConfig();
    Q_INVOKABLE bool hasSavedConfig() const;
    Q_INVOKABLE qint64 configLastModifiedMs() const;

    // Gestión de Presets
    QStringList availablePresets() const;
    Q_INVOKABLE bool isFactoryPreset(const QString& name) const;
    Q_INVOKABLE QJsonObject loadPreset(const QString& name);
    Q_INVOKABLE bool savePreset(const QString& name, const QJsonObject& config);
    Q_INVOKABLE bool deletePreset(const QString& name);
    Q_INVOKABLE bool exportPreset(const QString& name, const QString& destinationFilePath);
    Q_INVOKABLE bool importPreset(const QString& sourceFilePath);

    // Control del demonio / elemento en el escritorio (Quickshell layer-shell)
    Q_INVOKABLE bool isDesktopDaemonRunning() const;
    Q_INVOKABLE bool startDesktopDaemon();
    Q_INVOKABLE bool stopDesktopDaemon();
    Q_INVOKABLE bool restartDesktopDaemon();

signals:
    void backgroundModeRequestedChanged();
    void presetsChanged();
    void desktopDaemonStateChanged();

private:
    QJsonObject getFactoryPreset(const QString& name) const;
    void ensurePresetsDirectoryExists() const;

    bool m_bgMode{false};
};

} // namespace AudioViewer
