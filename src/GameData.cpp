#include "GameData.h"
#include "SimpleIni.h"
#include <iostream>
#include <filesystem>

using namespace GameDataNS;

static const char* SECTION_GAME = "Game";

// ---- GameConfig ----

void GameConfig::validate() {
    if (playerHP <= 0)    { playerHP = 100; }
    if (gold < 0)         { gold = 0; }
    if (level <= 0)       { level = 1; }
    if (killedCount < 0)  { killedCount = 0; }
    if (benchSize <= 0)   { benchSize = 8; }
    if (currentStage <= 0){ currentStage = 1; }
}

// ---- GameState ----

void GameState::reset() {
    currentPhase = Phase::Prep;
    playerUnits.clear();
    benchUnits.clear();
    enemyUnits.clear();
    shopSlots.clear();
}

// ---- GameDataManager ----

GameDataManager::GameDataManager(const std::string& path)
: path_(path) {
    // 注意：DisplayManager 已经负责在文件不存在时创建默认配置，
    // 这里只需要加载即可（文件可能已由 DisplayManager 创建）
    Load();
}

bool GameDataManager::Load() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error rc = ini.LoadFile(path_.c_str());
    if (rc < 0) {
        std::cerr << "GameData: LoadFile 失败，使用内存默认值\n";
        return false;
    }

    const char* val = nullptr;

    val = ini.GetValue(SECTION_GAME, "PlayerHP", nullptr);
    if (val) config_.playerHP = std::atoi(val);

    val = ini.GetValue(SECTION_GAME, "Gold", nullptr);
    if (val) config_.gold = std::atoi(val);

    val = ini.GetValue(SECTION_GAME, "Level", nullptr);
    if (val) config_.level = std::atoi(val);

    val = ini.GetValue(SECTION_GAME, "KilledCount", nullptr);
    if (val) config_.killedCount = std::atoi(val);

    val = ini.GetValue(SECTION_GAME, "BenchSize", nullptr);
    if (val) config_.benchSize = std::atoi(val);

    val = ini.GetValue(SECTION_GAME, "CurrentStage", nullptr);
    if (val) config_.currentStage = std::atoi(val);

    // 加载后校验数据
    config_.validate();

    return true;
}

bool GameDataManager::Save() {
    CSimpleIniA ini;
    ini.SetUnicode();

    // 先加载已有文件，保留其他 section 和注释
    if (std::filesystem::exists(path_)) {
        ini.LoadFile(path_.c_str());
    }

    auto setStr = [&](const char* sec, const char* key, const std::string& v) {
        ini.SetValue(sec, key, v.c_str(), nullptr, false);
    };

    setStr(SECTION_GAME, "PlayerHP",      std::to_string(config_.playerHP));
    setStr(SECTION_GAME, "Gold",          std::to_string(config_.gold));
    setStr(SECTION_GAME, "Level",         std::to_string(config_.level));
    setStr(SECTION_GAME, "KilledCount",   std::to_string(config_.killedCount));
    setStr(SECTION_GAME, "BenchSize",     std::to_string(config_.benchSize));
    setStr(SECTION_GAME, "CurrentStage",  std::to_string(config_.currentStage));

    return ini.SaveFile(path_.c_str()) == SI_OK;
}

void GameDataManager::ResetConfig() {
    config_ = GameConfig();
    Save();
}

void GameDataManager::ResetState() {
    state_.reset();
}

namespace GameDataNS {
    GameDataManager g_GameData("data.ini");
}