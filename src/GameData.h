#pragma once
#include "unit.h"
#include "board.h"
#include <vector>
#include <memory>
#include <string>

constexpr float AUTO_SAVE_INTERVAL = 60.0f;

namespace GameDataNS {

// 单位序列化数据（保存到 INI）
struct UnitSaveData {
    std::string typeName;       // 类型名称："战士"、"弓箭手"等
    int owner = 0;              // 0=Player, 1=Enemy, 2=Neutral
    int hp = 100;
    int maxHp = 100;
    int atk = 10;
    int starRank = 1;
    int row = -1;               // 棋盘行（-1表示在Bench上）
    int col = -1;               // 棋盘列
    int benchSlot = -1;         // Bench槽位（-1表示在棋盘上）
};

// 持久化游戏数据（保存到 INI）
struct GameConfig {
    int playerHP = 100;
    int gold = 20;
    int round = 1;
    int killedCount = 0;
    int benchSize = 8;
    int currentStage = 1;
    bool died = false;          // 是否因死亡结束上一局

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

// 保存完整游戏状态（含棋盘单位）到INI
bool SaveFullGame(const BoardNS::Board& board);

// 从INI恢复完整游戏状态（含棋盘单位）
bool LoadFullGame(BoardNS::Board& board);

} // namespace GameDataNS
