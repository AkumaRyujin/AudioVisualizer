#pragma once

#include <vector>
#include <mutex>
#include <cstdint>
#include <fftw3.h>

namespace AudioViewer {

class FftProcessor {
public:
    FftProcessor(size_t fftSize = 2048, uint32_t sampleRate = 48000, size_t barCount = 64);
    ~FftProcessor();

    // Ingesta de muestras desde el hilo de audio (hilo seguro)
    void pushSamples(const float* samples, size_t count);

    // Ejecuta el análisis FFT y calcula los valores normalizados [0.0, 1.0]
    void process();

    // Obtener los valores procesados
    const std::vector<float>& getBars() const { return m_smoothBars; }
    const std::vector<float>& getPeakBars() const { return m_peakBars; }

    // Parámetros configurables
    void setBarCount(size_t count);
    size_t getBarCount() const { return m_barCount; }

    void setSensitivity(float sens) { m_sensitivity = sens; }
    float getSensitivity() const { return m_sensitivity; }

    void setGravity(float grav) { m_gravity = grav; }
    float getGravity() const { return m_gravity; }

    void setAttackRate(float attack) { m_attackRate = attack; }
    float getAttackRate() const { return m_attackRate; }

    void setSpatialSmoothing(float factor) { m_spatialSmoothing = factor; }
    float getSpatialSmoothing() const { return m_spatialSmoothing; }

    void setFrequencies(float minHz, float maxHz);
    float getMinFrequency() const { return m_minFreq; }
    float getMaxFrequency() const { return m_maxFreq; }

    void reset();

private:
    void setupFft();
    void cleanupFft();
    void updateWindow();
    void recalculateFrequencyBands();

    size_t m_fftSize;
    uint32_t m_sampleRate;
    size_t m_barCount;

    float m_sensitivity{1.0f};
    float m_gravity{0.92f}; // Factor de decaimiento orgánico
    float m_attackRate{0.45f}; // Factor de subida suave y musical
    float m_spatialSmoothing{0.32f}; // Suavizado de coherencia entre barras adyacentes
    float m_minFreq{20.0f};
    float m_maxFreq{20000.0f};

    // Buffer circular de audio entrante
    std::vector<float> m_sampleRingBuffer;
    size_t m_ringWritePos{0};
    size_t m_samplesAvailable{0};
    std::mutex m_bufferMutex;

    // Estructuras FFTW
    float* m_fftIn{nullptr};
    fftwf_complex* m_fftOut{nullptr};
    fftwf_plan m_fftPlan{nullptr};

    std::vector<float> m_window;
    std::vector<float> m_magnitudes;

    // Rango de índices FFT para cada barra
    struct BandRange {
        size_t startBin;
        size_t endBin;
        float eqWeight;
    };
    std::vector<BandRange> m_bands;

    // Barras procesadas con suavizado
    std::vector<float> m_rawBars;
    std::vector<float> m_smoothBars;
    std::vector<float> m_peakBars;
};

} // namespace AudioViewer
