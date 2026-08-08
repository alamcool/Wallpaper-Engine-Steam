#pragma once
#include <string>

struct ConfigSettings {
    bool m_runOnStartup = true;
    bool m_pauseOnFullscreen = true;
    bool m_rgbChroma = false;
    bool m_rgbICue = false;
    bool m_rgbHue = false;
    bool m_hdrEnabled = false;
    int m_qualityLevel = 2;
    std::string m_defaultWallpaperPath;
    int m_displayMode = 0;
    int m_maxFPS = 60;
    int m_cacheSizeMB = 2048;
    int m_gpuUsageLimit = 30;
    bool m_pauseOnBattery = false;
};

class Config : public ConfigSettings {
private:
    std::string m_configPath;
    std::string m_displayModeStr;

    Config() = default;

public:
    static Config& GetInstance();

    void Load(const std::string& path);
    void Save(const std::string& path);

    int GetDisplayMode() const { return m_displayMode; }
    const std::string& GetDefaultWallpaperPath() const { return m_defaultWallpaperPath; }
    void SetDefaultWallpaperPath(const std::string& path) { m_defaultWallpaperPath = path; }
};
