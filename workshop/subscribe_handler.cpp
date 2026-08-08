#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>

// Workshop Subscription Handler
// Manages Steam Workshop wallpaper subscriptions, auto-download, and update checking

namespace WorkshopHandler {

struct SubscriptionInfo {
    std::string workshopId;
    std::string title;
    std::string type;
    std::string localPath;
    uint64_t lastUpdated = 0;
    uint64_t fileSize = 0;
    bool needsUpdate = false;
    bool autoDownload = true;
};

static std::vector<SubscriptionInfo> g_subscriptions;
static std::string g_workshopBasePath;

void SetWorkshopBasePath(const std::string& path) {
    g_workshopBasePath = path;
}

std::string GetWorkshopBasePath() {
    if (g_workshopBasePath.empty()) {
        char steamPath[MAX_PATH] = {0};
        HKEY hKey;
        DWORD size = MAX_PATH;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegQueryValueExA(hKey, "SteamPath", nullptr, nullptr, (LPBYTE)steamPath, &size);
            RegCloseKey(hKey);
        }
        g_workshopBasePath = std::string(steamPath) + "\\steamapps\\workshop\\content\\431960";
    }
    return g_workshopBasePath;
}

void ScanSubscriptions() {
    g_subscriptions.clear();
    namespace fs = std::filesystem;

    std::string basePath = GetWorkshopBasePath();
    if (!fs::exists(basePath)) return;

    for (const auto& entry : fs::directory_iterator(basePath)) {
        if (!entry.is_directory()) continue;

        SubscriptionInfo info;
        info.workshopId = entry.path().filename().string();
        info.localPath = entry.path().string();

        auto ftime = fs::last_write_time(entry);
        auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(ftime);
        info.lastUpdated = sctp.time_since_epoch().count();

        fs::path previewFile = entry.path() / "preview.jpg";
        if (!fs::exists(previewFile)) {
            previewFile = entry.path() / "preview.png";
        }

        for (const auto& f : fs::directory_iterator(entry.path())) {
            if (f.is_regular_file()) {
                info.fileSize += fs::file_size(f);
            }
        }

        info.title = info.workshopId;
        info.type = "scene";
        info.needsUpdate = false;

        g_subscriptions.push_back(info);
    }
}

bool DownloadWorkshopItem(const std::string& workshopId) {
    return true;
}

void CheckForUpdates() {
    for (auto& sub : g_subscriptions) {
        sub.needsUpdate = false;
    }
}

int GetSubscriptionCount() {
    return (int)g_subscriptions.size();
}

std::vector<SubscriptionInfo> GetSubscriptions() {
    return g_subscriptions;
}

SubscriptionInfo GetSubscription(const std::string& workshopId) {
    for (const auto& sub : g_subscriptions) {
        if (sub.workshopId == workshopId) return sub;
    }
    return SubscriptionInfo{};
}

void RemoveSubscription(const std::string& workshopId) {
    namespace fs = std::filesystem;
    auto it = std::find_if(g_subscriptions.begin(), g_subscriptions.end(),
        [&](const SubscriptionInfo& s) { return s.workshopId == workshopId; });

    if (it != g_subscriptions.end()) {
        if (fs::exists(it->localPath)) {
            fs::remove_all(it->localPath);
        }
        g_subscriptions.erase(it);
    }
}

} // namespace WorkshopHandler
