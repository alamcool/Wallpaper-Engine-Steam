#include "AudioVisualizer.h"
#include <cmath>
#include <algorithm>

AudioVisualizer& AudioVisualizer::GetInstance() {
    static AudioVisualizer instance;
    return instance;
}

bool AudioVisualizer::Initialize() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return false;

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&m_enumerator
    );
    if (FAILED(hr)) return false;

    hr = m_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
    if (FAILED(hr)) return false;

    hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
    if (FAILED(hr)) return false;

    WAVEFORMATEX* format = nullptr;
    hr = m_audioClient->GetMixFormat(&format);
    if (FAILED(hr)) return false;

    m_sampleRate = format->nSamplesPerSec;
    m_channels = format->nChannels;

    hr = m_audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        0, 0, format, nullptr
    );
    if (FAILED(hr)) {
        CoTaskMemFree(format);
        return false;
    }

    m_format = *format;
    CoTaskMemFree(format);

    hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_captureClient);
    if (FAILED(hr)) return false;

    m_captureEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_audioClient->SetEventHandle(m_captureEvent);

    m_fftSize = 1024;
    m_fftInput.resize(m_fftSize);
    m_fftOutput.resize(m_fftSize / 2);

    m_isRunning = true;
    m_captureThread = std::thread(&AudioVisualizer::CaptureLoop, this);

    return true;
}

void AudioVisualizer::CaptureLoop() {
    m_audioClient->Start();

    while (m_isRunning) {
        WaitForSingleObject(m_captureEvent, 100);

        UINT32 packetLength = 0;
        m_captureClient->GetNextPacketSize(&packetLength);

        while (packetLength != 0) {
            BYTE* data = nullptr;
            UINT32 numFrames = 0;
            DWORD flags = 0;

            m_captureClient->GetBuffer(&data, &numFrames, &flags, nullptr, nullptr);

            if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                ProcessAudioData((float*)data, numFrames);
            }

            m_captureClient->ReleaseBuffer(numFrames);
            m_captureClient->GetNextPacketSize(&packetLength);
        }
    }

    m_audioClient->Stop();
}

void AudioVisualizer::ProcessAudioData(const float* data, UINT32 frames) {
    for (UINT32 i = 0; i < frames && m_bufferPos < m_fftSize; i++) {
        float sample = 0.0f;
        for (UINT16 ch = 0; ch < m_channels; ch++) {
            sample += data[i * m_channels + ch];
        }
        sample /= m_channels;

        m_fftInput[m_bufferPos] = sample;
        m_bufferPos++;
    }

    if (m_bufferPos >= m_fftSize) {
        ComputeFFT();
        m_bufferPos = 0;
    }
}

void AudioVisualizer::ComputeFFT() {
    std::vector<std::complex<float>> complexInput(m_fftSize);
    for (int i = 0; i < m_fftSize; i++) {
        float window = 0.5f * (1.0f - cosf(2.0f * 3.14159265359f * i / (m_fftSize - 1)));
        complexInput[i] = m_fftInput[i] * window;
    }

    FFT(complexInput);

    for (int i = 0; i < m_fftSize / 2; i++) {
        float magnitude = std::abs(complexInput[i]) / (m_fftSize / 2);
        m_fftOutput[i] = m_fftOutput[i] * 0.7f + magnitude * 0.3f;
    }
}

void AudioVisualizer::FFT(std::vector<std::complex<float>>& data) {
    int n = (int)data.size();
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) std::swap(data[i], data[j]);
    }

    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * 3.14159265359f / len;
        std::complex<float> wlen(cosf(ang), sinf(ang));
        for (int i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (int j = 0; j < len / 2; j++) {
                std::complex<float> u = data[i + j];
                std::complex<float> v = data[i + j + len / 2] * w;
                data[i + j] = u + v;
                data[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

float AudioVisualizer::GetBassLevel() const {
    float sum = 0.0f;
    int bands = 8;
    for (int i = 0; i < bands; i++) {
        sum += m_fftOutput[i];
    }
    return std::min(sum / bands, 1.0f);
}

float AudioVisualizer::GetMidLevel() const {
    float sum = 0.0f;
    int start = 16;
    int end = 128;
    for (int i = start; i < end && i < (int)m_fftOutput.size(); i++) {
        sum += m_fftOutput[i];
    }
    return std::min(sum / (end - start), 1.0f);
}

float AudioVisualizer::GetTrebleLevel() const {
    float sum = 0.0f;
    int start = 128;
    int end = 512;
    for (int i = start; i < end && i < (int)m_fftOutput.size(); i++) {
        sum += m_fftOutput[i];
    }
    return std::min(sum / (end - start), 1.0f);
}

void AudioVisualizer::GetSpectrum(float* output, int bands) const {
    int bandSize = (m_fftSize / 2) / bands;
    for (int i = 0; i < bands; i++) {
        float sum = 0.0f;
        for (int j = 0; j < bandSize; j++) {
            int idx = i * bandSize + j;
            if (idx < (int)m_fftOutput.size()) {
                sum += m_fftOutput[idx];
            }
        }
        output[i] = std::min(sum / bandSize, 1.0f);
    }
}

void AudioVisualizer::Shutdown() {
    m_isRunning = false;
    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }
    if (m_captureEvent) CloseHandle(m_captureEvent);
    if (m_captureClient) { m_captureClient->Release(); m_captureClient = nullptr; }
    if (m_audioClient) { m_audioClient->Release(); m_audioClient = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
    if (m_enumerator) { m_enumerator->Release(); m_enumerator = nullptr; }
}
