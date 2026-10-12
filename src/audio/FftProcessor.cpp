#include "FftProcessor.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <numbers>

namespace AudioViewer {

FftProcessor::FftProcessor(size_t fftSize, uint32_t sampleRate, size_t barCount)
    : m_fftSize(fftSize)
    , m_sampleRate(sampleRate)
    , m_barCount(barCount)
{
    m_sampleRingBuffer.resize(m_fftSize * 4, 0.0f);
    setupFft();
    updateWindow();
    recalculateFrequencyBands();

    m_rawBars.resize(m_barCount, 0.0f);
    m_smoothBars.resize(m_barCount, 0.0f);
    m_peakBars.resize(m_barCount, 0.0f);
}

FftProcessor::~FftProcessor()
{
    cleanupFft();
}

void FftProcessor::setupFft()
{
    cleanupFft();

    m_fftIn = fftwf_alloc_real(m_fftSize);
    m_fftOut = fftwf_alloc_complex(m_fftSize / 2 + 1);
    m_fftPlan = fftwf_plan_dft_r2c_1d(
        static_cast<int>(m_fftSize),
        m_fftIn,
        m_fftOut,
        FFTW_ESTIMATE
    );
    m_magnitudes.resize(m_fftSize / 2 + 1, 0.0f);
}

void FftProcessor::cleanupFft()
{
    if (m_fftPlan) {
        fftwf_destroy_plan(m_fftPlan);
        m_fftPlan = nullptr;
    }
    if (m_fftIn) {
        fftwf_free(m_fftIn);
        m_fftIn = nullptr;
    }
    if (m_fftOut) {
        fftwf_free(m_fftOut);
        m_fftOut = nullptr;
    }
}

void FftProcessor::updateWindow()
{
    m_window.resize(m_fftSize);
    constexpr double twoPi = 2.0 * std::numbers::pi;
    for (size_t i = 0; i < m_fftSize; ++i) {
        // Ventana Hann
        m_window[i] = static_cast<float>(0.5 * (1.0 - std::cos(twoPi * i / (m_fftSize - 1))));
    }
}

void FftProcessor::recalculateFrequencyBands()
{
    m_bands.resize(m_barCount);

    const float logMin = std::log10(m_minFreq);
    const float logMax = std::log10(m_maxFreq);
    const float logStep = (logMax - logMin) / static_cast<float>(m_barCount);
    const float maxNyquistBin = static_cast<float>(m_fftSize / 2);
    const float freqPerBin = static_cast<float>(m_sampleRate) / static_cast<float>(m_fftSize);

    for (size_t i = 0; i < m_barCount; ++i) {
        float fLow = std::pow(10.0f, logMin + static_cast<float>(i) * logStep);
        float fHigh = std::pow(10.0f, logMin + static_cast<float>(i + 1) * logStep);

        size_t binStart = static_cast<size_t>(std::floor(fLow / freqPerBin));
        size_t binEnd = static_cast<size_t>(std::ceil(fHigh / freqPerBin));

        // Evitar componente DC (bin 0)
        binStart = std::max<size_t>(1, binStart);
        if (binEnd <= binStart) {
            binEnd = binStart + 1;
        }
        binEnd = std::min<size_t>(binEnd, static_cast<size_t>(maxNyquistBin));

        // Curva de ecualización para compensar la caída natural de altas frecuencias (+dB por octava)
        float centerFreq = std::sqrt(fLow * fHigh);
        float eqWeight = std::pow(centerFreq / m_minFreq, 0.42f);

        m_bands[i] = BandRange{binStart, binEnd, eqWeight};
    }
}

void FftProcessor::setBarCount(size_t count)
{
    if (count == 0 || count == m_barCount) {
        return;
    }
    m_barCount = count;
    m_rawBars.assign(m_barCount, 0.0f);
    m_smoothBars.assign(m_barCount, 0.0f);
    m_peakBars.assign(m_barCount, 0.0f);
    recalculateFrequencyBands();
}

void FftProcessor::setFrequencies(float minHz, float maxHz)
{
    if (minHz <= 0.0f || maxHz <= minHz) {
        return;
    }
    m_minFreq = minHz;
    m_maxFreq = maxHz;
    recalculateFrequencyBands();
}

void FftProcessor::pushSamples(const float* samples, size_t count)
{
    if (!samples || count == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_bufferMutex);
    const size_t bufSize = m_sampleRingBuffer.size();

    // Ingesta eficiente por bloques mediante memcpy
    size_t firstPart = std::min(count, bufSize - m_ringWritePos);
    std::memcpy(&m_sampleRingBuffer[m_ringWritePos], samples, firstPart * sizeof(float));
    size_t secondPart = count - firstPart;
    if (secondPart > 0) {
        std::memcpy(&m_sampleRingBuffer[0], samples + firstPart, secondPart * sizeof(float));
    }
    m_ringWritePos = (m_ringWritePos + count) % bufSize;
    m_samplesAvailable = std::min(m_samplesAvailable + count, bufSize);
}

void FftProcessor::process()
{
    if (!m_fftPlan || !m_fftIn || !m_fftOut) {
        return;
    }

    // Extraer las últimas m_fftSize muestras del buffer circular
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        if (m_samplesAvailable < m_fftSize) {
            return;
        }

        const size_t bufSize = m_sampleRingBuffer.size();
        size_t readPos = (m_ringWritePos + bufSize - m_fftSize) % bufSize;

        for (size_t i = 0; i < m_fftSize; ++i) {
            m_fftIn[i] = m_sampleRingBuffer[readPos] * m_window[i];
            readPos = (readPos + 1) % bufSize;
        }
    }

    // Ejecutar FFT
    fftwf_execute(m_fftPlan);

    // Magnitudes espectrales normalizadas
    const size_t numBins = m_fftSize / 2 + 1;
    const float normFactor = 2.0f / static_cast<float>(m_fftSize);

    for (size_t k = 0; k < numBins; ++k) {
        float re = m_fftOut[k][0];
        float im = m_fftOut[k][1];
        m_magnitudes[k] = std::sqrt(re * re + im * im) * normFactor;
    }

    // Etapa 1: Agregación de bins con mezcla armónica de energía promedio y pico
    std::vector<float> rawEnergies(m_barCount, 0.0f);
    for (size_t b = 0; b < m_barCount; ++b) {
        const auto& band = m_bands[b];
        float sum = 0.0f;
        float maxV = 0.0f;
        size_t binCount = band.endBin - band.startBin;

        for (size_t k = band.startBin; k < band.endBin; ++k) {
            float mag = m_magnitudes[k];
            sum += mag;
            if (mag > maxV) maxV = mag;
        }

        float avg = (binCount > 0) ? (sum / static_cast<float>(binCount)) : 0.0f;
        // Combinación: 60% energía del cuerpo de la banda + 40% pico transitorio
        float bandEnergy = (binCount > 1) ? (0.60f * avg + 0.40f * maxV) : maxV;

        float weighted = bandEnergy * band.eqWeight * m_sensitivity * 12.0f;
        float value = std::pow(weighted, 0.70f);
        rawEnergies[b] = std::clamp(value, 0.0f, 1.0f);
    }

    // Etapa 2: Suavizado espacial entre barras adyacentes (filtro binomial de coherencia)
    // Evita saltos inconexos entre barras vecinas que rompen las ondas y curvas
    std::vector<float> filteredEnergies(m_barCount, 0.0f);
    if (m_spatialSmoothing > 0.01f && m_barCount > 2) {
        float neighborWeight = m_spatialSmoothing * 0.5f;
        float centerWeight = 1.0f - m_spatialSmoothing;

        for (size_t b = 0; b < m_barCount; ++b) {
            float left = (b > 0) ? rawEnergies[b - 1] : rawEnergies[b];
            float right = (b + 1 < m_barCount) ? rawEnergies[b + 1] : rawEnergies[b];
            filteredEnergies[b] = centerWeight * rawEnergies[b] + neighborWeight * (left + right);
        }
    } else {
        filteredEnergies = rawEnergies;
    }

    // Etapa 3: Balística temporal orgánica (subida suave y controlada + caída con gravedad)
    for (size_t b = 0; b < m_barCount; ++b) {
        float target = filteredEnergies[b];
        m_rawBars[b] = target;

        if (target > m_smoothBars[b]) {
            // Ataque controlado: responde con pegada en 2-3 fotogramas sin saltos instantáneos
            float delta = target - m_smoothBars[b];
            float riseStep = std::min(delta * m_attackRate, 0.22f);
            m_smoothBars[b] += riseStep;
        } else {
            // Caída suave y constante mediante gravedad exponencial
            m_smoothBars[b] = m_smoothBars[b] * m_gravity;
            if (m_smoothBars[b] < 0.001f) {
                m_smoothBars[b] = 0.0f;
            }
        }

        // Seguimiento amortiguado de picos
        if (m_smoothBars[b] >= m_peakBars[b]) {
            m_peakBars[b] = m_smoothBars[b];
        } else {
            m_peakBars[b] = std::max(0.0f, m_peakBars[b] - 0.008f);
        }
    }
}

void FftProcessor::reset()
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    std::fill(m_sampleRingBuffer.begin(), m_sampleRingBuffer.end(), 0.0f);
    m_samplesAvailable = 0;
    m_ringWritePos = 0;

    std::fill(m_rawBars.begin(), m_rawBars.end(), 0.0f);
    std::fill(m_smoothBars.begin(), m_smoothBars.end(), 0.0f);
    std::fill(m_peakBars.begin(), m_peakBars.end(), 0.0f);
}

} // namespace AudioViewer
