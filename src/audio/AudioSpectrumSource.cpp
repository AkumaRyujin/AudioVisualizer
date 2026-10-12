#include "AudioSpectrumSource.h"
#include <cmath>

namespace AudioViewer {

AudioSpectrumSource::AudioSpectrumSource(QObject* parent)
    : QObject(parent)
    , m_capture(std::make_unique<AudioCapture>(48000))
    , m_processor(std::make_unique<FftProcessor>(2048, 48000, 64))
{
    m_capture->setAudioCallback([this](const float* data, size_t count) {
        if (m_processor) {
            m_processor->pushSamples(data, count);
        }

        if (m_autoSleepEnabled && m_isSleeping.load()) {
            for (size_t i = 0; i < count; ++i) {
                if (std::abs(data[i]) > 0.005f) {
                    QMetaObject::invokeMethod(this, &AudioSpectrumSource::wakeUp, Qt::QueuedConnection);
                    break;
                }
            }
        }
    });

    connect(&m_timer, &QTimer::timeout, this, &AudioSpectrumSource::onTick);
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(1000 / m_targetFps);
}

AudioSpectrumSource::~AudioSpectrumSource()
{
    stop();
}

int AudioSpectrumSource::barCount() const
{
    return static_cast<int>(m_processor->getBarCount());
}

void AudioSpectrumSource::setBarCount(int count)
{
    if (count > 0 && count != barCount()) {
        m_processor->setBarCount(static_cast<size_t>(count));
        emit barCountChanged();
    }
}

float AudioSpectrumSource::sensitivity() const
{
    return m_processor->getSensitivity();
}

void AudioSpectrumSource::setSensitivity(float sens)
{
    if (sens != sensitivity()) {
        m_processor->setSensitivity(sens);
        emit sensitivityChanged();
    }
}

float AudioSpectrumSource::gravity() const
{
    return m_processor->getGravity();
}

void AudioSpectrumSource::setGravity(float grav)
{
    if (grav != gravity()) {
        m_processor->setGravity(grav);
        emit gravityChanged();
    }
}

float AudioSpectrumSource::attackRate() const
{
    return m_processor->getAttackRate();
}

void AudioSpectrumSource::setAttackRate(float attack)
{
    if (std::abs(attack - attackRate()) > 0.001f) {
        m_processor->setAttackRate(attack);
        emit attackRateChanged();
    }
}

float AudioSpectrumSource::spatialSmoothing() const
{
    return m_processor->getSpatialSmoothing();
}

void AudioSpectrumSource::setSpatialSmoothing(float factor)
{
    if (std::abs(factor - spatialSmoothing()) > 0.001f) {
        m_processor->setSpatialSmoothing(factor);
        emit spatialSmoothingChanged();
    }
}

bool AudioSpectrumSource::isRunning() const
{
    return m_capture && m_capture->isRunning();
}

QString AudioSpectrumSource::currentDevice() const
{
    if (!m_capture) {
        return QStringLiteral("Inactivo");
    }
    return QString::fromStdString(m_capture->currentDevice());
}

QList<float> AudioSpectrumSource::spectrum() const
{
    return m_cachedSpectrum;
}

QList<float> AudioSpectrumSource::peaks() const
{
    return m_cachedPeaks;
}

void AudioSpectrumSource::setTargetFps(int fps)
{
    if (fps >= 15 && fps <= 240 && fps != m_targetFps) {
        m_targetFps = fps;
        if (!m_isSleeping.load()) {
            m_timer.setInterval(1000 / m_targetFps);
        }
        emit targetFpsChanged();
    }
}

void AudioSpectrumSource::setAutoSleepEnabled(bool enabled)
{
    if (m_autoSleepEnabled != enabled) {
        m_autoSleepEnabled = enabled;
        if (!enabled && m_isSleeping.load()) {
            wakeUp();
        }
        emit autoSleepEnabledChanged();
    }
}

void AudioSpectrumSource::setSleepDelayMs(int ms)
{
    if (ms >= 500 && ms <= 15000 && ms != m_sleepDelayMs) {
        m_sleepDelayMs = ms;
        emit sleepDelayMsChanged();
    }
}

void AudioSpectrumSource::wakeUp()
{
    if (m_isSleeping.load()) {
        m_isSleeping.store(false);
        m_silenceFrames = 0;
        m_timer.setInterval(1000 / m_targetFps);
        emit isSleepingChanged();
    }
}

bool AudioSpectrumSource::start()
{
    if (isRunning()) {
        return true;
    }

    if (m_capture->start()) {
        m_timer.start();
        emit isRunningChanged();
        return true;
    }

    return false;
}

void AudioSpectrumSource::stop()
{
    m_timer.stop();
    if (m_capture) {
        m_capture->stop();
    }
    emit isRunningChanged();
}

void AudioSpectrumSource::reset()
{
    if (m_processor) {
        m_processor->reset();
    }
    m_cachedSpectrum.clear();
    m_cachedPeaks.clear();
    emit spectrumUpdated(m_cachedSpectrum);
    emit peaksUpdated(m_cachedPeaks);
}

void AudioSpectrumSource::onTick()
{
    if (!m_capture || !m_capture->isRunning()) {
        return;
    }

    m_processor->process();

    const auto& bars = m_processor->getBars();
    const auto& peaks = m_processor->getPeakBars();

    // Comprobar la amplitud máxima para detectar silencio
    float maxVal = 0.0f;
    for (float v : bars) {
        if (v > maxVal) maxVal = v;
    }

    if (m_autoSleepEnabled && maxVal < 0.005f) {
        m_silenceFrames++;
        int thresholdFrames = (m_sleepDelayMs * m_targetFps) / 1000;
        if (m_silenceFrames >= thresholdFrames) {
            if (!m_isSleeping.load()) {
                m_isSleeping.store(true);
                // Enviar último frame en cero para limpiar el lienzo
                m_cachedSpectrum.fill(0.0f, static_cast<qsizetype>(bars.size()));
                m_cachedPeaks.fill(0.0f, static_cast<qsizetype>(peaks.size()));
                emit spectrumUpdated(m_cachedSpectrum);
                emit peaksUpdated(m_cachedPeaks);
                emit isSleepingChanged();
                // Reducir la tasa de sondeo a 100ms mientras duerme
                m_timer.setInterval(100);
            }
            return;
        }
    } else {
        if (m_isSleeping.load()) {
            m_isSleeping.store(false);
            m_timer.setInterval(1000 / m_targetFps);
            emit isSleepingChanged();
        }
        m_silenceFrames = 0;
    }

    if (m_cachedSpectrum.size() != static_cast<qsizetype>(bars.size())) {
        m_cachedSpectrum.resize(static_cast<qsizetype>(bars.size()));
    }
    std::copy(bars.begin(), bars.end(), m_cachedSpectrum.begin());

    if (m_cachedPeaks.size() != static_cast<qsizetype>(peaks.size())) {
        m_cachedPeaks.resize(static_cast<qsizetype>(peaks.size()));
    }
    std::copy(peaks.begin(), peaks.end(), m_cachedPeaks.begin());

    emit spectrumUpdated(m_cachedSpectrum);
    emit peaksUpdated(m_cachedPeaks);

    QString dev = currentDevice();
    if (dev != m_lastDevice) {
        m_lastDevice = dev;
        emit currentDeviceChanged();
    }
}

} // namespace AudioViewer
