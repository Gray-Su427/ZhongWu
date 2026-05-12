#pragma once
#include <string>
#include <algorithm>
#include <cmath>

namespace DisplayConfig {

struct DisplaySettings {
    unsigned int width = 1200;
    unsigned int height = 800;
    std::string title = "忠武弈锋";
    float scaling = 1.0f;
    bool fullscreen = false;
    int fontSize = 18;
    std::string fontPath = "lib/字魂白鸽天行体.ttf";
};

class DisplayManager {
public:
    explicit DisplayManager(const std::string& path = "data.ini");

    bool Load();
    bool Save();

    DisplaySettings& Settings() { return settings_; }
    const DisplaySettings& GetSettings() const { return settings_; }

    void Reset();

    // 缩放辅助
    float GetScale() const {
        return std::clamp(settings_.scaling, 0.5f, 2.5f);
    }

    int GetScaledFontSize() const {
        return std::max(8, static_cast<int>(std::round(settings_.fontSize * GetScale())));
    }

    float ScaleFloat(float v) const {
        return v * GetScale();
    }

    const std::string& Path() const { return path_; }

private:
    std::string path_;
    DisplaySettings settings_;
};

extern DisplayManager g_Display;

} // namespace DisplayConfig