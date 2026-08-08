#pragma once
#include <string>
#include <Windows.h>

enum class WallpaperType { Unknown, Image, Video, Web, Scene, Application };

struct RGBDevice {
    std::string name;
    int deviceId;
    bool connected;
};

struct DisplayInfo {
    std::string deviceName;
    int x, y;
    int width, height;
    bool isPrimary;
    float scaleFactor;
};

class WETypes {
public:
    static const char* WallpaperTypeToString(WallpaperType type) {
        switch (type) {
            case WallpaperType::Image: return "Image";
            case WallpaperType::Video: return "Video";
            case WallpaperType::Web: return "Web";
            case WallpaperType::Scene: return "Scene";
            case WallpaperType::Application: return "Application";
            default: return "Unknown";
        }
    }

    static WallpaperType StringToWallpaperType(const std::string& str) {
        if (str == "Image") return WallpaperType::Image;
        if (str == "Video") return WallpaperType::Video;
        if (str == "Web") return WallpaperType::Web;
        if (str == "Scene") return WallpaperType::Scene;
        if (str == "Application") return WallpaperType::Application;
        return WallpaperType::Unknown;
    }
};
