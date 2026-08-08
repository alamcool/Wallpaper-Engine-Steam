#include "MultiMonitor.h"
#include "Config.h"

MultiMonitor& MultiMonitor::GetInstance() {
    static MultiMonitor instance;
    return instance;
}

void MultiMonitor::DetectMonitors() {
    m_monitors.clear();

    EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM lParam) -> BOOL {
        auto* self = (MultiMonitor*)lParam;

        MONITORINFOEXW info;
        info.cbSize = sizeof(info);
        GetMonitorInfoW(hMon, &info);

        MonitorConfig mc;
        wcstombs(mc.deviceName, info.szDevice, sizeof(mc.deviceName));
        mc.x = info.rcMonitor.left;
        mc.y = info.rcMonitor.top;
        mc.width = info.rcMonitor.right - info.rcMonitor.left;
        mc.height = info.rcMonitor.bottom - info.rcMonitor.top;
        mc.isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
        mc.scaleFactor = self->GetMonitorScaleFactor(hMon);
        mc.wallpaperPath = "";
        mc.wallpaperMode = WallpaperMode::Stretch;
        mc.enabled = true;

        self->m_monitors.push_back(mc);
        return TRUE;
    }, (LPARAM)this);
}

float MultiMonitor::GetMonitorScaleFactor(HMONITOR hMonitor) {
    UINT dpiX = 96, dpiY = 96;
    HMODULE hShcore = LoadLibraryA("Shcore.dll");
    if (hShcore) {
        typedef HRESULT(WINAPI* GetDpiForMonitorFunc)(HMONITOR, int, UINT*, UINT*);
        auto pGetDpiForMonitor = (GetDpiForMonitorFunc)GetProcAddress(hShcore, "GetDpiForMonitor");
        if (pGetDpiForMonitor) {
            pGetDpiForMonitor(hMonitor, 0, &dpiX, &dpiY);
        }
        FreeLibrary(hShcore);
    }
    return (float)dpiX / 96.0f;
}

int MultiMonitor::GetMonitorCount() const {
    return (int)m_monitors.size();
}

MonitorConfig MultiMonitor::GetMonitor(int index) const {
    if (index < 0 || index >= (int)m_monitors.size()) return MonitorConfig{};
    return m_monitors[index];
}

void MultiMonitor::SetWallpaperForMonitor(int index, const std::string& path, WallpaperMode mode) {
    if (index < 0 || index >= (int)m_monitors.size()) return;
    m_monitors[index].wallpaperPath = path;
    m_monitors[index].wallpaperMode = mode;
}

void MultiMonitor::SetPanoramicWallpaper(const std::string& path) {
    for (auto& mon : m_monitors) {
        mon.wallpaperPath = path;
        mon.wallpaperMode = WallpaperMode::Span;
    }
}

void MultiMonitor::SaveProfile(const std::string& name) {
    MonitorProfile profile;
    profile.name = name;
    profile.monitors = m_monitors;
    m_profiles[name] = profile;
}

void MultiMonitor::LoadProfile(const std::string& name) {
    auto it = m_profiles.find(name);
    if (it != m_profiles.end()) {
        m_monitors = it->second.monitors;
    }
}

std::vector<std::string> MultiMonitor::GetProfileNames() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : m_profiles) {
        names.push_back(name);
    }
    return names;
}

void MultiMonitor::ApplyWallpapers() {
    for (const auto& mon : m_monitors) {
        if (!mon.enabled || mon.wallpaperPath.empty()) continue;

        switch (mon.wallpaperMode) {
            case WallpaperMode::Stretch:
                ApplyStretch(mon);
                break;
            case WallpaperMode::Fit:
                ApplyFit(mon);
                break;
            case WallpaperMode::Fill:
                ApplyFill(mon);
                break;
            case WallpaperMode::Center:
                ApplyCenter(mon);
                break;
            case WallpaperMode::Span:
                ApplySpan(mon);
                break;
        }
    }
}

void MultiMonitor::ApplyStretch(const MonitorConfig& mon) {
    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void*)mon.wallpaperPath.c_str(),
        SPIF_UPDATEINIFILE);
}

void MultiMonitor::ApplyFit(const MonitorConfig& mon) {
    (void)mon;
}

void MultiMonitor::ApplyFill(const MonitorConfig& mon) {
    (void)mon;
}

void MultiMonitor::ApplyCenter(const MonitorConfig& mon) {
    (void)mon;
}

void MultiMonitor::ApplySpan(const MonitorConfig& mon) {
    (void)mon;
}
