#include "AudioCapture.h"
#include <iostream>
#include <cstring>

namespace AudioViewer {

AudioCapture::AudioCapture(uint32_t sampleRate)
    : m_sampleRate(sampleRate)
{
}

AudioCapture::~AudioCapture()
{
    stop();
}

void AudioCapture::setAudioCallback(AudioDataCallback callback)
{
    m_audioCallback = std::move(callback);
}

std::string AudioCapture::currentDevice() const
{
    return m_monitorSource.empty() ? "Sin dispositivo" : m_monitorSource;
}

bool AudioCapture::start()
{
    if (m_running.load()) {
        return true;
    }

    m_mainloop = pa_threaded_mainloop_new();
    if (!m_mainloop) {
        std::cerr << "[AudioCapture] Error: No se pudo crear pa_threaded_mainloop\n";
        return false;
    }

    pa_threaded_mainloop_lock(m_mainloop);

    m_context = pa_context_new(pa_threaded_mainloop_get_api(m_mainloop), "AudioViewer Capture");
    if (!m_context) {
        std::cerr << "[AudioCapture] Error: No se pudo crear pa_context\n";
        pa_threaded_mainloop_unlock(m_mainloop);
        pa_threaded_mainloop_free(m_mainloop);
        m_mainloop = nullptr;
        return false;
    }

    pa_context_set_state_callback(m_context, contextStateCallback, this);

    if (pa_context_connect(m_context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        std::cerr << "[AudioCapture] Error: Fallo en pa_context_connect: " 
                  << pa_strerror(pa_context_errno(m_context)) << "\n";
        pa_context_unref(m_context);
        m_context = nullptr;
        pa_threaded_mainloop_unlock(m_mainloop);
        pa_threaded_mainloop_free(m_mainloop);
        m_mainloop = nullptr;
        return false;
    }

    if (pa_threaded_mainloop_start(m_mainloop) < 0) {
        std::cerr << "[AudioCapture] Error: No se pudo iniciar pa_threaded_mainloop\n";
        pa_context_disconnect(m_context);
        pa_context_unref(m_context);
        m_context = nullptr;
        pa_threaded_mainloop_unlock(m_mainloop);
        pa_threaded_mainloop_free(m_mainloop);
        m_mainloop = nullptr;
        return false;
    }

    m_running.store(true);
    pa_threaded_mainloop_unlock(m_mainloop);
    return true;
}

void AudioCapture::stop()
{
    if (!m_running.exchange(false)) {
        return;
    }

    if (m_mainloop) {
        pa_threaded_mainloop_stop(m_mainloop);
        pa_threaded_mainloop_lock(m_mainloop);

        disconnectStream();

        if (m_context) {
            pa_context_disconnect(m_context);
            pa_context_unref(m_context);
            m_context = nullptr;
        }

        pa_threaded_mainloop_unlock(m_mainloop);
        pa_threaded_mainloop_free(m_mainloop);
        m_mainloop = nullptr;
    }
}

void AudioCapture::contextStateCallback(pa_context* c, void* userdata)
{
    auto* self = static_cast<AudioCapture*>(userdata);
    self->onContextState(c);
}

void AudioCapture::onContextState(pa_context* c)
{
    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_READY: {
            pa_context_set_subscribe_callback(c, contextSubscribeCallback, this);
            pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SERVER, nullptr, nullptr);
            pa_context_get_server_info(c, serverInfoCallback, this);
            break;
        }
        case PA_CONTEXT_FAILED: {
            std::cerr << "[AudioCapture] Contexto de audio falló: "
                      << pa_strerror(pa_context_errno(c)) << "\n";
            break;
        }
        case PA_CONTEXT_TERMINATED:
            break;
        default:
            break;
    }
}

void AudioCapture::serverInfoCallback(pa_context* /*c*/, const pa_server_info* i, void* userdata)
{
    auto* self = static_cast<AudioCapture*>(userdata);
    self->onServerInfo(i);
}

void AudioCapture::onServerInfo(const pa_server_info* i)
{
    if (!i || !i->default_sink_name) {
        return;
    }

    std::string newSink = i->default_sink_name;
    std::string newMonitor = newSink + ".monitor";

    if (newMonitor != m_monitorSource) {
        m_defaultSink = newSink;
        m_monitorSource = newMonitor;
        connectStream(m_monitorSource);
    }
}

void AudioCapture::contextSubscribeCallback(pa_context* /*c*/, pa_subscription_event_type_t t, uint32_t idx, void* userdata)
{
    auto* self = static_cast<AudioCapture*>(userdata);
    self->onSubscribeEvent(t, idx);
}

void AudioCapture::onSubscribeEvent(pa_subscription_event_type_t t, uint32_t /*idx*/)
{
    if ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SERVER) {
        pa_context_get_server_info(m_context, serverInfoCallback, this);
    }
}

bool AudioCapture::connectStream(const std::string& sourceName)
{
    disconnectStream();

    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.channels = 2;
    ss.rate = m_sampleRate;

    m_stream = pa_stream_new(m_context, "AudioViewer Monitor", &ss, nullptr);
    if (!m_stream) {
        std::cerr << "[AudioCapture] Error: No se pudo crear pa_stream\n";
        return false;
    }

    pa_stream_set_state_callback(m_stream, streamStateCallback, this);
    pa_stream_set_read_callback(m_stream, streamReadCallback, this);

    pa_buffer_attr attr;
    attr.maxlength = static_cast<uint32_t>(-1);
    attr.tlength = static_cast<uint32_t>(-1);
    attr.prebuf = static_cast<uint32_t>(-1);
    attr.minreq = static_cast<uint32_t>(-1);
    // Latencia objetivo de ~20ms
    attr.fragsize = pa_usec_to_bytes(20000, &ss);

    pa_stream_flags_t flags = static_cast<pa_stream_flags_t>(
        PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_DONT_MOVE
    );

    int ret = pa_stream_connect_record(m_stream, sourceName.c_str(), &attr, flags);
    if (ret < 0) {
        std::cerr << "[AudioCapture] Error al conectar grabación de stream (" 
                  << sourceName << "): " << pa_strerror(pa_context_errno(m_context)) << "\n";
        pa_stream_unref(m_stream);
        m_stream = nullptr;
        return false;
    }

    return true;
}

void AudioCapture::disconnectStream()
{
    if (m_stream) {
        pa_stream_set_state_callback(m_stream, nullptr, nullptr);
        pa_stream_set_read_callback(m_stream, nullptr, nullptr);
        pa_stream_disconnect(m_stream);
        pa_stream_unref(m_stream);
        m_stream = nullptr;
    }
}

void AudioCapture::streamStateCallback(pa_stream* s, void* userdata)
{
    auto* self = static_cast<AudioCapture*>(userdata);
    self->onStreamState(s);
}

void AudioCapture::onStreamState(pa_stream* s)
{
    switch (pa_stream_get_state(s)) {
        case PA_STREAM_READY:
            break;
        case PA_STREAM_FAILED:
            std::cerr << "[AudioCapture] Stream falló: "
                      << pa_strerror(pa_context_errno(m_context)) << "\n";
            break;
        case PA_STREAM_TERMINATED:
            break;
        default:
            break;
    }
}

void AudioCapture::streamReadCallback(pa_stream* s, size_t length, void* userdata)
{
    auto* self = static_cast<AudioCapture*>(userdata);
    self->onStreamRead(s, length);
}

void AudioCapture::onStreamRead(pa_stream* s, size_t /*length*/)
{
    const void* data = nullptr;
    size_t nbytes = 0;

    while (pa_stream_peek(s, &data, &nbytes) == 0 && nbytes > 0) {
        if (!data) {
            pa_stream_drop(s);
            continue;
        }

        const float* floatData = reinterpret_cast<const float*>(data);
        size_t frames = nbytes / (sizeof(float) * 2);

        if (frames > 0 && m_audioCallback) {
            if (m_monoConversionBuffer.size() < frames) {
                m_monoConversionBuffer.resize(frames);
            }

            for (size_t i = 0; i < frames; ++i) {
                m_monoConversionBuffer[i] = (floatData[2 * i] + floatData[2 * i + 1]) * 0.5f;
            }

            m_audioCallback(m_monoConversionBuffer.data(), frames);
        }

        pa_stream_drop(s);
    }
}

} // namespace AudioViewer
