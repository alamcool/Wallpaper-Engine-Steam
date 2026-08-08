#include "WallpaperEditor.h"
#include "Config.h"
#include <fstream>
#include <sstream>
#include <iomanip>

WallpaperEditor& WallpaperEditor::GetInstance() {
    static WallpaperEditor instance;
    return instance;
}

void WallpaperEditor::Initialize() {
    m_timeline.clear();
    m_layers.clear();
    m_currentFrame = 0;
    m_totalFrames = 300;
    m_fps = 30;
    m_width = 1920;
    m_height = 1080;
}

void WallpaperEditor::AddLayer(const std::string& name, const std::string& sourcePath) {
    Layer layer;
    layer.name = name;
    layer.sourcePath = sourcePath;
    layer.opacity = 1.0f;
    layer.visible = true;
    layer.x = 0;
    layer.y = 0;
    layer.scale = 1.0f;
    layer.rotation = 0.0f;
    layer.blendMode = BlendMode::Normal;
    m_layers.push_back(layer);
}

void WallpaperEditor::RemoveLayer(int index) {
    if (index >= 0 && index < (int)m_layers.size()) {
        m_layers.erase(m_layers.begin() + index);
    }
}

void WallpaperEditor::SetLayerProperty(int index, const std::string& prop, float value) {
    if (index < 0 || index >= (int)m_layers.size()) return;

    Layer& layer = m_layers[index];
    if (prop == "opacity") layer.opacity = value;
    else if (prop == "x") layer.x = value;
    else if (prop == "y") layer.y = value;
    else if (prop == "scale") layer.scale = value;
    else if (prop == "rotation") layer.rotation = value;
}

void WallpaperEditor::AddKeyframe(int layerIndex, int frame, const std::string& property, float value) {
    if (layerIndex < 0 || layerIndex >= (int)m_layers.size()) return;

    auto& timeline = m_timeline[layerIndex];
    timeline[property][frame] = value;
}

float WallpaperEditor::GetInterpolatedValue(int layerIndex, const std::string& property, int frame) const {
    auto layerIt = m_timeline.find(layerIndex);
    if (layerIt == m_timeline.end()) return 0.0f;

    auto propIt = layerIt->second.find(property);
    if (propIt == layerIt->second.end()) return 0.0f;

    const auto& keyframes = propIt->second;

    if (keyframes.empty()) return 0.0f;
    if (frame <= keyframes.begin()->first) return keyframes.begin()->second;
    if (frame >= keyframes.rbegin()->first) return keyframes.rbegin()->second;

    auto upper = keyframes.lower_bound(frame);
    auto lower = std::prev(upper);

    float t = (float)(frame - lower->first) / (float)(upper->first - lower->first);
    return lower->second + (upper->second - lower->second) * EaseInOut(t);
}

float WallpaperEditor::EaseInOut(float t) const {
    return t * t * (3.0f - 2.0f * t);
}

void WallpaperEditor::ExportToPackage(const std::string& outputPath) {
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) return;

    file.write("PKGW", 4);

    uint32_t version = 3;
    file.write((char*)&version, sizeof(version));

    file.write((char*)&m_width, sizeof(m_width));
    file.write((char*)&m_height, sizeof(m_height));
    file.write((char*)&m_fps, sizeof(m_fps));
    file.write((char*)&m_totalFrames, sizeof(m_totalFrames));

    uint32_t layerCount = (uint32_t)m_layers.size();
    file.write((char*)&layerCount, sizeof(layerCount));

    for (const auto& layer : m_layers) {
        uint32_t nameLen = (uint32_t)layer.name.size();
        file.write((char*)&nameLen, sizeof(nameLen));
        file.write(layer.name.c_str(), nameLen);

        uint32_t pathLen = (uint32_t)layer.sourcePath.size();
        file.write((char*)&pathLen, sizeof(pathLen));
        file.write(layer.sourcePath.c_str(), pathLen);

        file.write((char*)&layer.opacity, sizeof(layer.opacity));
        file.write((char*)&layer.x, sizeof(layer.x));
        file.write((char*)&layer.y, sizeof(layer.y));
        file.write((char*)&layer.scale, sizeof(layer.scale));
        file.write((char*)&layer.rotation, sizeof(layer.rotation));
    }

    uint32_t timelineCount = (uint32_t)m_timeline.size();
    file.write((char*)&timelineCount, sizeof(timelineCount));

    for (const auto& [layerIdx, props] : m_timeline) {
        file.write((char*)&layerIdx, sizeof(layerIdx));
        uint32_t propCount = (uint32_t)props.size();
        file.write((char*)&propCount, sizeof(propCount));

        for (const auto& [propName, frames] : props) {
            uint32_t pLen = (uint32_t)propName.size();
            file.write((char*)&pLen, sizeof(pLen));
            file.write(propName.c_str(), pLen);

            uint32_t frameCount = (uint32_t)frames.size();
            file.write((char*)&frameCount, sizeof(frameCount));

            for (const auto& [frame, value] : frames) {
                file.write((char*)&frame, sizeof(frame));
                file.write((char*)&value, sizeof(value));
            }
        }
    }

    file.close();
}

void WallpaperEditor::RenderPreview() {
    m_currentFrame = (m_currentFrame + 1) % m_totalFrames;

    for (int i = 0; i < (int)m_layers.size(); i++) {
        if (!m_layers[i].visible) continue;

        float opacity = GetInterpolatedValue(i, "opacity", m_currentFrame);
        float x = GetInterpolatedValue(i, "x", m_currentFrame);
        float y = GetInterpolatedValue(i, "y", m_currentFrame);
        float scale = GetInterpolatedValue(i, "scale", m_currentFrame);
        float rotation = GetInterpolatedValue(i, "rotation", m_currentFrame);

        m_layers[i].opacity = opacity;
        m_layers[i].x = x;
        m_layers[i].y = y;
        m_layers[i].scale = scale;
        m_layers[i].rotation = rotation;
    }
}
