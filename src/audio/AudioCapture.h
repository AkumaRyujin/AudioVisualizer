#pragma once

#include <pulse/pulseaudio.h>
#include <functional>
#include <string>
#include <atomic>
#include <vector>

namespace AudioViewer {

class AudioCapture {
public:
    using AudioDataCallback = std::function<void(const float* monoSamples, size_t sampleCount)>;

    AudioCapture(uint32_t sampleRate = 48000);
    ~AudioCapture();

    bool start();
    void stop();

    bool isRunning() const { return m_running.load(); }
    std::string currentDevice() const;

    void setAudioCallback(AudioDataCallback callback);

private:
    static void contextStateCallback(pa_context* c, void* userdata);
    static void streamStateCallback(pa_stream* s, void* userdata);
    static void streamReadCallback(pa_stream* s, size_t length, void* userdata);
    static void serverInfoCallback(pa_context* c, const pa_server_info* i, void* userdata);
    static void contextSubscribeCallback(pa_context* c, pa_subscription_event_type_t t, uint32_t idx, void* userdata);

    void onContextState(pa_context* c);
    void onStreamState(pa_stream* s);
    void onStreamRead(pa_stream* s, size_t length);
    void onServerInfo(const pa_server_info* i);
    void onSubscribeEvent(pa_subscription_event_type_t t, uint32_t idx);

    bool connectStream(const std::string& sourceName);
    void disconnectStream();

    uint32_t m_sampleRate;
    std::atomic<bool> m_running{false};
    AudioDataCallback m_audioCallback;

    pa_threaded_mainloop* m_mainloop{nullptr};
    pa_context* m_context{nullptr};
    pa_stream* m_stream{nullptr};

    std::string m_defaultSink;
    std::string m_monitorSource;
    std::vector<float> m_monoConversionBuffer;
};

} // namespace AudioViewer
