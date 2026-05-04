#include "DataConfig.h"
#include <cstdlib>

using namespace DataConfig;

static DisplaySettings g_display;
static GameData g_game;

bool DataConfig::LoadConfig(const std::string& path) {
    CSimpleIniA ini;
    ini.SetUnicode(); // 允许以 UTF-8 读写（SimpleIni 支持）
    SI_Error rc = ini.LoadFile(path.c_str());
    // 若文件不存在也继续使用默认值
    (void)rc;

    const char* val = nullptr;

    val = ini.GetValue("Display", "Width", nullptr);
    if (val) g_display.width = std::atoi(val);
    val = ini.GetValue("Display", "Height", nullptr);
    if (val) g_display.height = std::atoi(val);
    val = ini.GetValue("Display", "Title", nullptr);
    if (val) g_display.title = val;
    val = ini.GetValue("Display", "Fullscreen", nullptr);
    if (val) g_display.fullscreen = (std::atoi(val) != 0);
    val = ini.GetValue("Display", "FontSize", nullptr);
    if (val) g_display.fontSize = std::atoi(val);
    val = ini.GetValue("Display", "FontPath", nullptr);
    if (val) g_display.fontPath = val;

    val = ini.GetValue("Game", "PlayerHP", nullptr);
    if (val) g_game.playerHP = std::atoi(val);
    val = ini.GetValue("Game", "Gold", nullptr);
    if (val) g_game.gold = std::atoi(val);
    val = ini.GetValue("Game", "Level", nullptr);
    if (val) g_game.level = std::atoi(val);
    val = ini.GetValue("Game", "PopulationCap", nullptr);
    if (val) g_game.populationCap = std::atoi(val);
    val = ini.GetValue("Game", "CurrentStage", nullptr);
    if (val) g_game.currentStage = std::atoi(val);

    return true;
}

bool DataConfig::SaveConfig(const std::string& path) {
    CSimpleIniA ini;
    ini.SetUnicode();

    ini.SetValue("Display", "Width", std::to_string(g_display.width).c_str(), nullptr, false);
    ini.SetValue("Display", "Height", std::to_string(g_display.height).c_str(), nullptr, false);
    ini.SetValue("Display", "Title", g_display.title.c_str(), nullptr, false);
    ini.SetValue("Display", "Fullscreen", g_display.fullscreen ? "1" : "0", nullptr, false);
    ini.SetValue("Display", "FontSize", std::to_string(g_display.fontSize).c_str(), nullptr, false);
    ini.SetValue("Display", "FontPath", g_display.fontPath.c_str(), nullptr, false);

    ini.SetValue("Game", "PlayerHP", std::to_string(g_game.playerHP).c_str(), nullptr, false);
    ini.SetValue("Game", "Gold", std::to_string(g_game.gold).c_str(), nullptr, false);
    ini.SetValue("Game", "Level", std::to_string(g_game.level).c_str(), nullptr, false);
    ini.SetValue("Game", "PopulationCap", std::to_string(g_game.populationCap).c_str(), nullptr, false);
    ini.SetValue("Game", "CurrentStage", std::to_string(g_game.currentStage).c_str(), nullptr, false);

    SI_Error rc = ini.SaveFile(path.c_str());
    return rc == SI_OK;
}

const DisplaySettings& DataConfig::GetDisplaySettings() { return g_display; }
DisplaySettings& DataConfig::MutDisplaySettings() { return g_display; }

const GameData& DataConfig::GetGameData() { return g_game; }
GameData& DataConfig::MutGameData() { return g_game; }

void DataConfig::ResetDisplaySettings() {
    g_display = DisplaySettings(); // 使用默认构造恢复默认值
}

void DataConfig::ResetGameData() {
    g_game = GameData(); // 使用默认构造恢复默认值
}