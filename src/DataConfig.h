#pragma once
#include "SimpleIni.h"
#include <string>

namespace DataConfig {

struct DisplaySettings {
    unsigned int width = 1200;
    unsigned int height = 800;
    std::string title = "忠武弈锋";
    bool fullscreen = false;
    int fontSize = 18;
    std::string fontPath = "lib/字魂白鸽天行体.ttf";
};

struct GameData {
    int playerHP = 100;
    int gold = 20;
    int level = 1;
    int killed_count = 0;
    int populationCap = 10;
    int currentStage = 1;
};

bool LoadConfig(const std::string& path = "data.ini");
bool SaveConfig(const std::string& path = "data.ini");

void ResetDisplaySettings(); // 恢复显示设置为默认值（内存中）
void ResetGameData();        // 恢复游戏数据为默认值（内存中）

const DisplaySettings& GetDisplaySettings();
DisplaySettings& MutDisplaySettings();

const GameData& GetGameData();
GameData& MutGameData();

} // namespace DataConfig