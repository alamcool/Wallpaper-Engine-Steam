#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <atomic>

enum class WallpaperType {
    Unknown,
    Image,
    Video,
    Web,
    Scene,
    Application
};

struct MonitorInfo {
    std::string deviceName;
    RECT rect;
    int width;
    int height;
};

class WallpaperEngine {
private:
    HINSTANCE m_hInstance = nullptr;
    HWND m_hwnd = nullptr;
    std::vector<MonitorInfo> m_monitors;
    std::string m_currentWallpaperPath;
    WallpaperType m_wallpaperType = WallpaperType::Unknown;
    bool m_isRunning = false;
    bool m_isPlaying = false;
    bool m_wallpaperChanged = false;
    bool m_pauseOnFullscreen = true;
    bool m_autoResume = true;
    int m_displayMode = 0;

    WallpaperEngine() = default;

    void RegisterWindowClass();
    void CreateWallpaperWindow();
    void LoadWallpaper(const std::string& path);
    bool ShouldPauseForFullscreen() const;

public:
    static WallpaperEngine& GetInstance();

    void Initialize(HINSTANCE hInstance, int displayMode);
    void SetActiveWallpaper(const std::string& path);
    void Run();
    void Shutdown();

    void PausePlayback();
    void ResumePlayback();
    void SetPlaybackState(bool playing) { m_isPlaying = playing; }
    void SetPauseOnFullscreen(bool pause) { m_pauseOnFullscreen = pause; }

    HWND GetHWND() const { return m_hwnd; }
    bool IsPlaying() const { return m_isPlaying; }
    WallpaperType GetWallpaperType() const { return m_wallpaperType; }
    const std::string& GetCurrentWallpaperPath() const { return m_currentWallpaperPath; }
    const std::vector<MonitorInfo>& GetMonitors() const { return m_monitors; }
};
