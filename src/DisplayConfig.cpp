#include "DisplayConfig.h"
#include "SimpleIni.h"
#include <iostream>
#include <filesystem>

using namespace DisplayConfig;

static const char* SECTION_DISPLAY = "Display";

DisplayManager::DisplayManager(const std::string& path)
: path_(path) {
    if (!std::filesystem::exists(path_)) {
        std::cout << "DisplayConfig: 未找到 " << path_ << "，创建默认配置\n";
        Save();
    }
    Load();
}

bool DisplayManager::Load() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error rc = ini.LoadFile(path_.c_str());
    if (rc < 0) {
        std::cerr << "DisplayConfig: LoadFile 失败，使用内存默认值\n";
        return false;
    }

    const char* val = nullptr;

    val = ini.GetValue(SECTION_DISPLAY, "Width", nullptr);
    if (val) settings_.width = static_cast<unsigned int>(std::atoi(val));

    val = ini.GetValue(SECTION_DISPLAY, "Height", nullptr);
    if (val) settings_.height = static_cast<unsigned int>(std::atoi(val));

    val = ini.GetValue(SECTION_DISPLAY, "Title", nullptr);
    if (val) settings_.title = val;

    val = ini.GetValue(SECTION_DISPLAY, "Fullscreen", nullptr);
    if (val) settings_.fullscreen = (std::atoi(val) != 0);

    val = ini.GetValue(SECTION_DISPLAY, "FontSize", nullptr);
    if (val) settings_.fontSize = std::max(8, std::atoi(val));

    val = ini.GetValue(SECTION_DISPLAY, "FontPath", nullptr);
    if (val) settings_.fontPath = val;

    val = ini.GetValue(SECTION_DISPLAY, "Scaling", nullptr);
    if (val) {
        try {
            settings_.scaling = std::clamp(std::stof(val), 0.5f, 2.5f);
        } catch (const std::exception& e) {
            std::cerr << "DisplayConfig: Scaling 值无效（" << val << "），使用默认值\n";
            settings_.scaling = 1.0f;
        }
    }

    return true;
}

bool DisplayManager::Save() {
    CSimpleIniA ini;
    ini.SetUnicode();

    // 先加载已有文件，保留其他 section 和注释
    if (std::filesystem::exists(path_)) {
        ini.LoadFile(path_.c_str());
    }

    auto setStr = [&](const char* sec, const char* key, const std::string& v) {
        ini.SetValue(sec, key, v.c_str(), nullptr, false);
    };

    setStr(SECTION_DISPLAY, "Width",      std::to_string(settings_.width));
    setStr(SECTION_DISPLAY, "Height",     std::to_string(settings_.height));
    setStr(SECTION_DISPLAY, "Title",      settings_.title);
    setStr(SECTION_DISPLAY, "Fullscreen", settings_.fullscreen ? "1" : "0");
    setStr(SECTION_DISPLAY, "FontSize",   std::to_string(settings_.fontSize));
    setStr(SECTION_DISPLAY, "FontPath",   settings_.fontPath);
    setStr(SECTION_DISPLAY, "Scaling",    std::to_string(settings_.scaling));

    return ini.SaveFile(path_.c_str()) == SI_OK;
}

void DisplayManager::Reset() {
    settings_ = DisplaySettings();
}

namespace DisplayConfig {
    DisplayManager g_Display("data.ini");
}