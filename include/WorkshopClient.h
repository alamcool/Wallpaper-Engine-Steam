#pragma once
#include <string>
#include <vector>

struct WorkshopItem {
    std::string id;
    std::string title;
    std::string type;
    std::string path;
    std::string previewPath;
    std::string description;
    int fileSize = 0;
    bool subscribed = false;
};

class WorkshopClient {
private:
    std::string m_steamPath;
    std::string m_workshopPath;
    std::vector<WorkshopItem> m_subscribedItems;

    WorkshopClient() = default;

    std::string FindSteamPath();
    void ScanInstalledWallpapers();
    void LoadMetadata(const std::string& path, WorkshopItem& item);

public:
    static WorkshopClient& GetInstance();

    void Initialize();
    void SyncSubscriptions();
    void Shutdown();

    std::vector<WorkshopItem> GetSubscribedItems() const;
    WorkshopItem GetItemById(const std::string& id) const;
    std::vector<WorkshopItem> GetItemsByCategory(const std::string& category) const;
    int GetItemCount() const;

    const std::string& GetWorkshopPath() const { return m_workshopPath; }
};
