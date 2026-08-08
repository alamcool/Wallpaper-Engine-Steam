#pragma once
#include <Windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <complex>
#include <vector>
#include <thread>
#include <atomic>

class AudioVisualizer {
private:
    IMMDeviceEnumerator* m_enumerator = nullptr;
    IMMDevice* m_device = nullptr;
    IAudioClient* m_audioClient = nullptr;
    IAudioCaptureClient* m_captureClient = nullptr;
    HANDLE m_captureEvent = nullptr;
    WAVEFORMATEX m_format = {};
    UINT32 m_sampleRate = 44100;
    UINT16 m_channels = 2;
    int m_fftSize = 1024;
    int m_bufferPos = 0;
    std::vector<float> m_fftInput;
    std::vector<float> m_fftOutput;
    std::thread m_captureThread;
    std::atomic<bool> m_isRunning = false;

    AudioVisualizer() = default;

    void CaptureLoop();
    void ProcessAudioData(const float* data, UINT32 frames);
    void ComputeFFT();
    void FFT(std::vector<std::complex<float>>& data);

public:
    static AudioVisualizer& GetInstance();

    bool Initialize();
    void Shutdown();

    float GetBassLevel() const;
    float GetMidLevel() const;
    float GetTrebleLevel() const;
    void GetSpectrum(float* output, int bands) const;

    int GetFFTSize() const { return m_fftSize; }
    bool IsRunning() const { return m_isRunning; }
};
