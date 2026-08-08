#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <regex>

// Workshop Wallpaper Metadata Parser
// Parses wallpaper metadata from Workshop item directories

namespace WallpaperMetadata {

struct Metadata {
    std::string title;
    std::string description;
    std::string type;
    std::vector<std::string> tags;
    std::vector<std::string> categories;
    int width = 0;
    int height = 0;
    int framerate = 0;
    float rating = 0.0f;
    int subscriptionCount = 0;
    uint64_t publishDate = 0;
    uint64_t updateDate = 0;
    std::string author;
    std::string previewPath;
    std::string contentPath;
    std::vector<std::string> supportedResolutions;
};

std::string ExtractJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + searchKey.length());
    if (pos == std::string::npos) return "";

    pos++;
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

    if (pos >= json.length()) return "";

    if (json[pos] == '"') {
        size_t start = pos + 1;
        size_t end = json.find('"', start);
        if (end == std::string::npos) return "";
        return json.substr(start, end - start);
    } else {
        size_t start = pos;
        size_t end = json.find_first_of(",}\n", start);
        if (end == std::string::npos) end = json.length();
        return json.substr(start, end - start);
    }
}

std::vector<std::string> ExtractJsonArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return result;

    pos = json.find('[', pos);
    if (pos == std::string::npos) return result;

    size_t end = json.find(']', pos);
    if (end == std::string::npos) return result;

    std::string arrayContent = json.substr(pos + 1, end - pos - 1);

    size_t start = 0;
    while (start < arrayContent.length()) {
        size_t qStart = arrayContent.find('"', start);
        if (qStart == std::string::npos) break;
        size_t qEnd = arrayContent.find('"', qStart + 1);
        if (qEnd == std::string::npos) break;
        result.push_back(arrayContent.substr(qStart + 1, qEnd - qStart - 1));
        start = qEnd + 1;
    }

    return result;
}

Metadata ParseMetadataFile(const std::string& path) {
    Metadata meta;
    namespace fs = std::filesystem;

    if (!fs::exists(path)) return meta;

    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    meta.title = ExtractJsonValue(content, "title");
    meta.description = ExtractJsonValue(content, "description");
    meta.type = ExtractJsonValue(content, "type");
    meta.author = ExtractJsonValue(content, "author");
    meta.tags = ExtractJsonArray(content, "tags");
    meta.categories = ExtractJsonArray(content, "categories");

    std::string widthStr = ExtractJsonValue(content, "width");
    std::string heightStr = ExtractJsonValue(content, "height");
    if (!widthStr.empty()) meta.width = std::stoi(widthStr);
    if (!heightStr.empty()) meta.height = std::stoi(heightStr);

    std::string fpsStr = ExtractJsonValue(content, "framerate");
    if (!fpsStr.empty()) meta.framerate = std::stoi(fpsStr);

    std::string ratingStr = ExtractJsonValue(content, "rating");
    if (!ratingStr.empty()) meta.rating = std::stof(ratingStr);

    std::string subStr = ExtractJsonValue(content, "subscriptions");
    if (!subStr.empty()) meta.subscriptionCount = std::stoi(subStr);

    meta.supportedResolutions = ExtractJsonArray(content, "supported_resolutions");

    return meta;
}

Metadata ParseFromWorkshopDirectory(const std::string& dirPath) {
    Metadata meta;
    namespace fs = std::filesystem;

    if (!fs::exists(dirPath)) return meta;

    fs::path infoPath = fs::path(dirPath) / "info.json";
    if (fs::exists(infoPath)) {
        meta = ParseMetadataFile(infoPath.string());
    }

    fs::path previewPath = fs::path(dirPath) / "preview.jpg";
    if (!fs::exists(previewPath)) {
        previewPath = fs::path(dirPath) / "preview.png";
    }
    meta.previewPath = fs::exists(previewPath) ? previewPath.string() : "";

    for (const auto& entry : fs::directory_iterator(dirPath)) {
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".mp4" || ext == ".webm" || ext == ".jpg" || ext == ".png" ||
            ext == ".html" || ext == ".pkg") {
            meta.contentPath = entry.path().string();
            break;
        }
    }

    return meta;
}

} // namespace WallpaperMetadata
