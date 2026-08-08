#include "WorkshopClient.h"
#include "Config.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

WorkshopClient& WorkshopClient::GetInstance() {
    static WorkshopClient instance;
    return instance;
}

void WorkshopClient::Initialize() {
    m_steamPath = FindSteamPath();
    m_workshopPath = m_steamPath + "\\steamapps\\workshop\\content\\431960\\";
    ScanInstalledWallpapers();
}

std::string WorkshopClient::FindSteamPath() {
    HKEY hKey;
    char value[MAX_PATH];
    DWORD size = MAX_PATH;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "SteamPath", nullptr, nullptr, (LPBYTE)value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(value);
        }
        RegCloseKey(hKey);
    }

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "InstallPath", nullptr, nullptr, (LPBYTE)value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(value);
        }
        RegCloseKey(hKey);
    }

    return "C:\\Program Files (x86)\\Steam";
}

void WorkshopClient::ScanInstalledWallpapers() {
    m_subscribedItems.clear();

    if (!fs::exists(m_workshopPath)) return;

    for (const auto& entry : fs::directory_iterator(m_workshopPath)) {
        if (!entry.is_directory()) continue;

        WorkshopItem item;
        item.id = entry.path().filename().string();
        item.path = entry.path().string();

        fs::path previewPath = entry.path() / "preview.jpg";
        if (!fs::exists(previewPath)) {
            previewPath = entry.path() / "preview.png";
        }
        item.previewPath = fs::exists(previewPath) ? previewPath.string() : "";

        fs::path infoPath = entry.path() / "info.json";
        if (fs::exists(infoPath)) {
            LoadMetadata(infoPath.string(), item);
        }

        m_subscribedItems.push_back(item);
    }
}

void WorkshopClient::LoadMetadata(const std::string& path, WorkshopItem& item) {
    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    size_t titlePos = content.find("\"title\"");
    if (titlePos != std::string::npos) {
        size_t start = content.find('"', titlePos + 7);
        size_t end = content.find('"', start + 1);
        if (start != std::string::npos && end != std::string::npos) {
            item.title = content.substr(start + 1, end - start - 1);
        }
    }

    size_t typePos = content.find("\"type\"");
    if (typePos != std::string::npos) {
        size_t start = content.find('"', typePos + 6);
        size_t end = content.find('"', start + 1);
        if (start != std::string::npos && end != std::string::npos) {
            item.type = content.substr(start + 1, end - start - 1);
        }
    }
}

void WorkshopClient::SyncSubscriptions() {
    ScanInstalledWallpapers();
}

std::vector<WorkshopItem> WorkshopClient::GetSubscribedItems() const {
    return m_subscribedItems;
}

WorkshopItem WorkshopClient::GetItemById(const std::string& id) const {
    for (const auto& item : m_subscribedItems) {
        if (item.id == id) return item;
    }
    return WorkshopItem{};
}

std::vector<WorkshopItem> WorkshopClient::GetItemsByCategory(const std::string& category) const {
    std::vector<WorkshopItem> result;
    for (const auto& item : m_subscribedItems) {
        if (item.type == category) {
            result.push_back(item);
        }
    }
    return result;
}

int WorkshopClient::GetItemCount() const {
    return (int)m_subscribedItems.size();
}

void WorkshopClient::Shutdown() {
    m_subscribedItems.clear();
}
