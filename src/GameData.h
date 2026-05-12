#pragma once
#include "unit.h"
#include <vector>
#include <memory>
#include <string>

constexpr float AUTO_SAVE_INTERVAL = 60.0f;

namespace GameDataNS {

// 持久化游戏数据（保存到 INI）
struct GameConfig {
    int playerHP = 100;
    int gold = 20;
    int level = 1;
    int killedCount = 0;
    int benchSize = 8;
    int currentStage = 1;

    /// 校验数据合法性，将非法值修正为默认值
    void validate();
};

// 运行时状态（不保存到 INI）
struct GameState {
    enum class Phase { Prep, Combat, Resolve };
    Phase currentPhase = Phase::Prep;

    std::vector<std::unique_ptr<Unit>> playerUnits;   // 场上玩家棋子
    std::vector<std::unique_ptr<Unit>> benchUnits;     // 备战席棋子
    std::vector<std::unique_ptr<Unit>> enemyUnits;     // 敌方棋子
    std::vector<std::unique_ptr<Unit>> shopSlots;      // 商店槽位

    /// 重置运行时状态（清空所有单位、回到准备阶段）
    void reset();
};

class GameDataManager {
public:
    explicit GameDataManager(const std::string& path = "data.ini");

    bool Load();
    bool Save();

    GameConfig& Config() { return config_; }
    const GameConfig& GetConfig() const { return config_; }

    GameState& State() { return state_; }
    const GameState& GetState() const { return state_; }

    /// 重置持久化数据并自动保存
    void ResetConfig();
    /// 重置运行时状态
    void ResetState();

    const std::string& Path() const { return path_; }

private:
    std::string path_;
    GameConfig config_;
    GameState state_;
};

extern GameDataManager g_GameData;

} // namespace GameDataNS