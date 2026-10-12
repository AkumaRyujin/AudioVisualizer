#pragma once

#include <QObject>
#include <QList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <memory>
#include <atomic>
#include "AudioCapture.h"
#include "FftProcessor.h"

namespace AudioViewer {

class AudioSpectrumSource : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int barCount READ barCount WRITE setBarCount NOTIFY barCountChanged)
    Q_PROPERTY(float sensitivity READ sensitivity WRITE setSensitivity NOTIFY sensitivityChanged)
    Q_PROPERTY(float gravity READ gravity WRITE setGravity NOTIFY gravityChanged)
    Q_PROPERTY(float attackRate READ attackRate WRITE setAttackRate NOTIFY attackRateChanged)
    Q_PROPERTY(float spatialSmoothing READ spatialSmoothing WRITE setSpatialSmoothing NOTIFY spatialSmoothingChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(QString currentDevice READ currentDevice NOTIFY currentDeviceChanged)
    Q_PROPERTY(QList<float> spectrum READ spectrum NOTIFY spectrumUpdated)
    Q_PROPERTY(QList<float> peaks READ peaks NOTIFY peaksUpdated)
    Q_PROPERTY(int targetFps READ targetFps WRITE setTargetFps NOTIFY targetFpsChanged)

    // Modo Ahorro de Energía / Detección de Silencio
    Q_PROPERTY(bool autoSleepEnabled READ autoSleepEnabled WRITE setAutoSleepEnabled NOTIFY autoSleepEnabledChanged)
    Q_PROPERTY(bool isSleeping READ isSleeping NOTIFY isSleepingChanged)
    Q_PROPERTY(int sleepDelayMs READ sleepDelayMs WRITE setSleepDelayMs NOTIFY sleepDelayMsChanged)

public:
    explicit AudioSpectrumSource(QObject* parent = nullptr);
    ~AudioSpectrumSource() override;

    int barCount() const;
    void setBarCount(int count);

    float sensitivity() const;
    void setSensitivity(float sens);

    float gravity() const;
    void setGravity(float grav);

    float attackRate() const;
    void setAttackRate(float attack);

    float spatialSmoothing() const;
    void setSpatialSmoothing(float factor);

    bool isRunning() const;
    QString currentDevice() const;

    QList<float> spectrum() const;
    QList<float> peaks() const;

    int targetFps() const { return m_targetFps; }
    void setTargetFps(int fps);

    bool autoSleepEnabled() const { return m_autoSleepEnabled; }
    void setAutoSleepEnabled(bool enabled);

    bool isSleeping() const { return m_isSleeping.load(); }

    int sleepDelayMs() const { return m_sleepDelayMs; }
    void setSleepDelayMs(int ms);

    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void wakeUp();

signals:
    void barCountChanged();
    void sensitivityChanged();
    void gravityChanged();
    void attackRateChanged();
    void spatialSmoothingChanged();
    void isRunningChanged();
    void currentDeviceChanged();
    void spectrumUpdated(const QList<float>& spectrum);
    void peaksUpdated(const QList<float>& peaks);
    void targetFpsChanged();
    void autoSleepEnabledChanged();
    void isSleepingChanged();
    void sleepDelayMsChanged();

private slots:
    void onTick();

private:
    std::unique_ptr<AudioCapture> m_capture;
    std::unique_ptr<FftProcessor> m_processor;
    QTimer m_timer;

    int m_targetFps{60};
    QList<float> m_cachedSpectrum;
    QList<float> m_cachedPeaks;
    QString m_lastDevice;

    bool m_autoSleepEnabled{true};
    std::atomic<bool> m_isSleeping{false};
    int m_sleepDelayMs{2000};
    int m_silenceFrames{0};
};

} // namespace AudioViewer
