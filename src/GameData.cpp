#include "GameData.h"
#include "unit_types.h"
#include "board.h"
#include "SimpleIni.h"
#include <iostream>
#include <filesystem>
#include <sstream>

namespace GameDataNS {

static const char* SECTION_GAME = "Game";
static const char* SECTION_BOARD = "Board";
static const char* SECTION_BENCH = "Bench";

// ---- GameConfig ----

void GameConfig::validate() {
    if (playerHP <= 0)    { playerHP = 100; }
    if (gold < 0)         { gold = 0; }
    if (round <= 0)        { round = 1; }
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

// ---- 序列化辅助 ----

static std::string serializeUnit(Unit* unit) {
    if (!unit) return "";
    int ownerInt = 0;
    if (unit->owner() == Unit::Owner::EnemyCtrl) ownerInt = 1;
    else if (unit->owner() == Unit::Owner::Neutral) ownerInt = 2;
    return unit->getUnitTypeName() + "," +
           std::to_string(ownerInt) + "," +
           std::to_string(unit->getHP()) + "," +
           std::to_string(unit->getMaxHP()) + "," +
           std::to_string(unit->getATK()) + "," +
           std::to_string(1);
}

static UnitSaveData deserializeUnitData(const std::string& str) {
    UnitSaveData data;
    std::istringstream iss(str);
    std::string token;
    if (std::getline(iss, token, ',')) data.typeName = token;
    if (std::getline(iss, token, ',')) data.owner = std::atoi(token.c_str());
    if (std::getline(iss, token, ',')) data.hp = std::atoi(token.c_str());
    if (std::getline(iss, token, ',')) data.maxHp = std::atoi(token.c_str());
    if (std::getline(iss, token, ',')) data.atk = std::atoi(token.c_str());
    if (std::getline(iss, token, ',')) data.starRank = std::atoi(token.c_str());
    return data;
}

static std::unique_ptr<Unit> createUnitFromSaveData(const UnitSaveData& data) {
    auto unit = createUnitByName(data.typeName, sf::Vector2f(0.f, 0.f), data.starRank);
    if (!unit) return nullptr;
    if (data.owner == 0) unit->setOwner(Unit::Owner::PlayerCtrl);
    else if (data.owner == 1) unit->setOwner(Unit::Owner::EnemyCtrl);
    else unit->setOwner(Unit::Owner::Neutral);
    unit->setMaxHP(data.maxHp);
    unit->setHP(data.hp);
    unit->setATK(data.atk);
    return unit;
}

static void saveUnits(CSimpleIniA& ini, const BoardNS::Board& board) {
    ini.Delete(SECTION_BOARD, nullptr);
    ini.Delete(SECTION_BENCH, nullptr);

    int boardCount = 0;
    board.forEachBoardUnit([&](const BoardNS::GridPos& pos, Unit* unit) {
        std::string key = "Unit" + std::to_string(boardCount);
        std::string val = serializeUnit(unit) + "," +
                          std::to_string(pos.row) + "," + std::to_string(pos.col);
        ini.SetValue(SECTION_BOARD, key.c_str(), val.c_str());
        boardCount++;
    });
    ini.SetValue(SECTION_BOARD, "Count", std::to_string(boardCount).c_str());

    int benchCount = 0;
    for (int i = 0; i < board.getBenchSize(); ++i) {
        Unit* unit = board.getUnitOnBench(i);
        if (unit) {
            std::string key = "Unit" + std::to_string(benchCount);
            std::string val = serializeUnit(unit) + "," + std::to_string(i);
            ini.SetValue(SECTION_BENCH, key.c_str(), val.c_str());
            benchCount++;
        }
    }
    ini.SetValue(SECTION_BENCH, "Count", std::to_string(benchCount).c_str());
}

static void loadUnits(const CSimpleIniA& ini, BoardNS::Board& board) {
    board.clearAll();

    const char* countStr = ini.GetValue(SECTION_BOARD, "Count", "0");
    int boardCount = std::atoi(countStr);
    for (int i = 0; i < boardCount; ++i) {
        std::string key = "Unit" + std::to_string(i);
        const char* val = ini.GetValue(SECTION_BOARD, key.c_str(), nullptr);
        if (!val) continue;
        std::string str(val);
        UnitSaveData data = deserializeUnitData(str);
        std::istringstream iss(str);
        std::string token;
        int fieldIdx = 0;
        while (std::getline(iss, token, ',')) {
            if (fieldIdx == 6) data.row = std::atoi(token.c_str());
            if (fieldIdx == 7) data.col = std::atoi(token.c_str());
            fieldIdx++;
        }
        auto unit = createUnitFromSaveData(data);
        if (unit && data.row >= 0 && data.col >= 0) {
            board.placeOnBoard(BoardNS::GridPos{data.row, data.col}, std::move(unit));
        }
    }

    countStr = ini.GetValue(SECTION_BENCH, "Count", "0");
    int benchCount = std::atoi(countStr);
    for (int i = 0; i < benchCount; ++i) {
        std::string key = "Unit" + std::to_string(i);
        const char* val = ini.GetValue(SECTION_BENCH, key.c_str(), nullptr);
        if (!val) continue;
        std::string str(val);
        UnitSaveData data = deserializeUnitData(str);
        std::istringstream iss(str);
        std::string token;
        int fieldIdx = 0;
        while (std::getline(iss, token, ',')) {
            if (fieldIdx == 6) data.benchSlot = std::atoi(token.c_str());
            fieldIdx++;
        }
        auto unit = createUnitFromSaveData(data);
        if (unit && data.benchSlot >= 0) {
            board.placeOnBench(data.benchSlot, std::move(unit));
        }
    }
}

// ---- GameDataManager ----

GameDataManager::GameDataManager(const std::string& path)
: path_(path) {
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
    val = ini.GetValue(SECTION_GAME, "Round", nullptr);
    if (val) config_.round = std::atoi(val);
    val = ini.GetValue(SECTION_GAME, "KilledCount", nullptr);
    if (val) config_.killedCount = std::atoi(val);
    val = ini.GetValue(SECTION_GAME, "BenchSize", nullptr);
    if (val) config_.benchSize = std::atoi(val);
    val = ini.GetValue(SECTION_GAME, "CurrentStage", nullptr);
    if (val) config_.currentStage = std::atoi(val);
    val = ini.GetValue(SECTION_GAME, "Died", nullptr);
    if (val) config_.died = (std::string(val) == "true" || std::atoi(val) != 0);

    config_.validate();
    return true;
}

bool GameDataManager::Save() {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (std::filesystem::exists(path_)) {
        ini.LoadFile(path_.c_str());
    }

    auto setStr = [&](const char* sec, const char* key, const std::string& v) {
        ini.SetValue(sec, key, v.c_str(), nullptr, false);
    };

    setStr(SECTION_GAME, "PlayerHP",      std::to_string(config_.playerHP));
    setStr(SECTION_GAME, "Gold",          std::to_string(config_.gold));
    setStr(SECTION_GAME, "Round",         std::to_string(config_.round));
    setStr(SECTION_GAME, "KilledCount",   std::to_string(config_.killedCount));
    setStr(SECTION_GAME, "BenchSize",     std::to_string(config_.benchSize));
    setStr(SECTION_GAME, "CurrentStage",  std::to_string(config_.currentStage));
    setStr(SECTION_GAME, "Died",          config_.died ? "true" : "false");

    return ini.SaveFile(path_.c_str()) == SI_OK;
}

void GameDataManager::ResetConfig() {
    config_ = GameConfig();
    Save();
}

void GameDataManager::ResetState() {
    state_.reset();
}

// ---- 全局函数 ----

bool SaveFullGame(const BoardNS::Board& board) {
    CSimpleIniA ini;
    ini.SetUnicode();
    if (std::filesystem::exists(g_GameData.Path())) {
        ini.LoadFile(g_GameData.Path().c_str());
    }

    auto& config = g_GameData.Config();
    auto setStr = [&](const char* sec, const char* key, const std::string& v) {
        ini.SetValue(sec, key, v.c_str(), nullptr, false);
    };

    setStr(SECTION_GAME, "PlayerHP",      std::to_string(config.playerHP));
    setStr(SECTION_GAME, "Gold",          std::to_string(config.gold));
    setStr(SECTION_GAME, "Round",         std::to_string(config.round));
    setStr(SECTION_GAME, "KilledCount",   std::to_string(config.killedCount));
    setStr(SECTION_GAME, "BenchSize",     std::to_string(config.benchSize));
    setStr(SECTION_GAME, "CurrentStage",  std::to_string(config.currentStage));
    setStr(SECTION_GAME, "Died",          config.died ? "true" : "false");

    saveUnits(ini, board);
    return ini.SaveFile(g_GameData.Path().c_str()) == SI_OK;
}

bool LoadFullGame(BoardNS::Board& board) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error rc = ini.LoadFile(g_GameData.Path().c_str());
    if (rc < 0) return false;

    g_GameData.Load();
    loadUnits(ini, board);
    return true;
}

// 全局实例
GameDataManager g_GameData("data.ini");

} // namespace GameDataNS