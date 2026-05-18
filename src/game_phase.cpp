#include "game_phase.h"
#include "game_renderer.h"
#include "GameData.h"
#include <algorithm>
#include <iostream>

namespace GamePhaseNS {

GameManager::GameManager()
: board_(8, 8, 8)
, rng_(std::random_device{}()) {
}

// --- 核心循环 ---

void GameManager::update(float dt) {
    switch (currentPhase_) {
        case Phase::Prep:
            break;
        case Phase::Combat:
            updateCombat(dt);
            if (isCombatOver()) {
                resolveRound();
            }
            break;
        case Phase::Resolve:
            break;
    }
}

// --- 阶段控制 ---

void GameManager::setPhase(Phase phase) {
    currentPhase_ = phase;
}

void GameManager::nextPhase() {
    switch (currentPhase_) {
        case Phase::Prep:
            startCombat();
            break;
        case Phase::Combat:
            resolveRound();
            break;
        case Phase::Resolve:
            startNewRound();
            break;
    }
}

// --- 准备阶段操作 ---

void GameManager::startNewRound() {
    currentPhase_ = Phase::Prep;
    currentRound_++;
    GameDataNS::g_GameData.Config().round = currentRound_;

    // 结算收入：基础收入 + 利息
    int income = calculateIncome(false);
    addGold(income);

    // 自动刷新商店
    freeRefreshShop();

    spawnEnemyUnits();
    std::cout << "第 " << currentRound_ << " 轮开始（准备阶段），收入: " << income
              << " 金币，当前金币: " << gold_ << std::endl;
}

void GameManager::spawnEnemyUnits() {
    // 清除旧的敌方单位
    auto enemies = board_.getEnemyUnitsOnBoard();
    for (auto* e : enemies) {
        for (int r = 0; r < board_.getRows(); ++r) {
            for (int c = 0; c < board_.getCols(); ++c) {
                BoardNS::GridPos pos{r, c};
                if (board_.getUnitOnBoard(pos) == e) {
                    board_.removeFromBoard(pos);
                    break;
                }
            }
        }
    }

    int numEnemies = std::min(1 + currentRound_ / 2, 5);
    std::uniform_int_distribution<int> typeDist(0, 4);

    int halfCols = board_.getCols() / 2;
    std::uniform_int_distribution<int> colDist(halfCols, board_.getCols() - 1);
    std::uniform_int_distribution<int> rowDist(0, board_.getRows() - 1);

    for (int i = 0; i < numEnemies; ++i) {
        BoardNS::GridPos pos;
        int attempts = 0;
        do {
            pos.row = rowDist(rng_);
            pos.col = colDist(rng_);
            attempts++;
        } while (!board_.isBoardCellEmpty(pos) && attempts < 50);

        if (board_.isBoardCellEmpty(pos)) {
            std::unique_ptr<Unit> enemy;
            int type = typeDist(rng_);
            switch (type) {
                case 0: enemy = std::make_unique<WarriorUnit>(); break;
                case 1: enemy = std::make_unique<ArcherUnit>(); break;
                case 2: enemy = std::make_unique<MageUnit>(); break;
                case 3: enemy = std::make_unique<TankUnit>(); break;
                case 4: enemy = std::make_unique<AssassinUnit>(); break;
                default: enemy = std::make_unique<WarriorUnit>(); break;
            }
            enemy->setOwner(Unit::Owner::EnemyCtrl);
            float scaleFactor = 1.0f + (currentRound_ - 1) * 0.15f;
            enemy->setMaxHP(static_cast<int>(enemy->getMaxHP() * scaleFactor));
            enemy->setHP(enemy->getMaxHP());
            enemy->setATK(static_cast<int>(enemy->getATK() * scaleFactor));

            board_.placeOnBoard(pos, std::move(enemy));
        }
    }
}

bool GameManager::moveBenchToBoard(int benchSlot, const BoardNS::GridPos& pos) {
    if (benchSlot < 0 || board_.isBoardCellEmpty(pos) == false) {
        return swapBenchAndBoard(benchSlot, pos);
    }
    auto unit = board_.removeFromBench(benchSlot);
    if (!unit) return false;
    if (!board_.placeOnBoard(pos, std::move(unit))) {
        board_.placeOnBench(benchSlot, std::move(unit));
        return false;
    }
    return true;
}

bool GameManager::moveBoardToBench(const BoardNS::GridPos& pos, int benchSlot) {
    if (benchSlot < 0) return false;
    auto unit = board_.removeFromBoard(pos);
    if (!unit) return false;
    if (!board_.placeOnBench(benchSlot, std::move(unit))) {
        board_.placeOnBoard(pos, std::move(unit));
        return false;
    }
    return true;
}

bool GameManager::moveOnBoard(const BoardNS::GridPos& from, const BoardNS::GridPos& to) {
    if (from == to) return false;
    auto unitFrom = board_.removeFromBoard(from);
    if (!unitFrom) return false;

    auto unitTo = board_.removeFromBoard(to);
    if (!board_.placeOnBoard(to, std::move(unitFrom))) {
        board_.placeOnBoard(from, std::move(unitFrom));
        if (unitTo) board_.placeOnBoard(to, std::move(unitTo));
        return false;
    }
    if (unitTo) {
        board_.placeOnBoard(from, std::move(unitTo));
    }
    return true;
}

bool GameManager::swapBenchAndBoard(int benchSlot, const BoardNS::GridPos& pos) {
    auto benchUnit = board_.removeFromBench(benchSlot);
    auto boardUnit = board_.removeFromBoard(pos);

    bool success = true;
    if (benchUnit) {
        if (!board_.placeOnBoard(pos, std::move(benchUnit))) {
            success = false;
        }
    }
    if (boardUnit) {
        if (!board_.placeOnBench(benchSlot, std::move(boardUnit))) {
            success = false;
        }
    }
    return success;
}

// --- 战斗阶段 ---

void GameManager::setRenderer(RendererNS::GameRenderer* renderer) {
    renderer_ = renderer;
}

void GameManager::startCombat() {
    currentPhase_ = Phase::Combat;
    combatTimer_ = 0.f;
    combatActive_ = true;

    // 计算羁绊 Buff
    synergyMgr_.calculateSynergies(board_);

    // 输出激活的羁绊信息
    const auto& actives = synergyMgr_.getActiveSynergies();
    for (const auto& s : actives) {
        if (s.level > 0) {
            std::cout << "[羁绊] " << s.traitName << " 激活! "
                      << "数量:" << s.count << " 层级:" << s.level
                      << " (" << s.buffDescription << ")" << std::endl;
        }
    }

    // 获取渲染器布局信息
    float uniformScale = 1.0f;
    sf::FloatRect boardBounds({0.f, 0.f}, {512.f, 512.f});
    if (renderer_) {
        const auto& layout = renderer_->getLayout();
        uniformScale = layout.uniformScale;
        boardBounds = sf::FloatRect(
            {layout.boardOriginX, layout.boardOriginY},
            {layout.boardPixelW, layout.boardPixelH}
        );
    }

    // 初始化所有棋盘单位的战斗状态
    board_.forEachBoardUnit([&](const BoardNS::GridPos& pos, Unit* unit) {
        if (unit->isAlive()) {
            // 计算格子中心的像素坐标
            sf::Vector2f pixelPos;
            if (renderer_) {
                pixelPos = renderer_->boardCellToScreen(pos);
            } else {
                pixelPos = sf::Vector2f(
                    pos.col * 64.f + 32.f,
                    pos.row * 64.f + 32.f
                );
            }
            unit->setPosition(pixelPos);
            unit->setInCombat(true);
            unit->setBoardBounds(boardBounds);
            unit->resetCombatState();

            // 为玩家单位设置羁绊 Buff
            if (unit->owner() == Unit::Owner::PlayerCtrl) {
                unit->setSynergyATKBonus(synergyMgr_.getATKBonus(unit));
                unit->setSynergyCritBonus(synergyMgr_.getCritBonus(unit));
                unit->setSynergyDamageReduction(synergyMgr_.getDamageReduction(unit));
                unit->setSynergyAOEMultiplier(synergyMgr_.getAOEMultiplier(unit));
            }
        }
    });

    std::cout << "战斗开始！" << std::endl;
}

void GameManager::updateCombat(float dt) {
    if (!combatActive_) return;

    combatTimer_ += dt;

    // 获取缩放系数
    float uniformScale = 1.0f;
    if (renderer_) {
        uniformScale = renderer_->getLayout().uniformScale;
    }

    // 获取所有存活单位
    auto allUnits = board_.getAllUnitsOnBoard();

    // 分阵营
    std::vector<Unit*> playerUnits;
    std::vector<Unit*> enemyUnits;
    for (auto* u : allUnits) {
        if (!u->isAlive()) continue;
        if (u->owner() == Unit::Owner::PlayerCtrl) {
            playerUnits.push_back(u);
        } else if (u->owner() == Unit::Owner::EnemyCtrl) {
            enemyUnits.push_back(u);
        }
    }

    // 为每个单位分配目标（找最近的敌方）
    for (auto* unit : allUnits) {
        if (!unit->isAlive()) continue;

        // 检查当前目标是否仍然有效
        Unit* currentTarget = unit->getTarget();
        if (!currentTarget || !currentTarget->isAlive()) {
            // 需要重新寻找目标
            if (unit->owner() == Unit::Owner::PlayerCtrl) {
                unit->setTarget(unit->findClosestEnemy(enemyUnits));
            } else if (unit->owner() == Unit::Owner::EnemyCtrl) {
                unit->setTarget(unit->findClosestEnemy(playerUnits));
            }
        }

        // 更新战斗FSM
        unit->updateCombat(dt, uniformScale);

        // 法师AOE处理：如果法师进入Casting状态，执行AOE
        if (unit->getState() == Unit::State::Casting && unit->hasTrait("法师")) {
            auto* mage = dynamic_cast<MageUnit*>(unit);
            if (mage) {
                if (unit->owner() == Unit::Owner::PlayerCtrl) {
                    mage->castAOE(enemyUnits, uniformScale, mage->getSynergyAOEMultiplier());
                } else {
                    mage->castAOE(playerUnits, uniformScale, 0.f);
                }
            }
        }
    }
}

bool GameManager::isCombatOver() const {
    // 超时判定
    if (combatTimer_ >= combatMaxTime_) {
        return true;
    }

    auto playerUnits = board_.getPlayerUnitsOnBoard();
    auto enemyUnits = board_.getEnemyUnitsOnBoard();

    bool playerAlive = false;
    for (auto* u : playerUnits) {
        if (u->isAlive()) { playerAlive = true; break; }
    }

    bool enemyAlive = false;
    for (auto* u : enemyUnits) {
        if (u->isAlive()) { enemyAlive = true; break; }
    }

    return !playerAlive || !enemyAlive;
}

// --- 结算阶段 ---

void GameManager::resolveRound() {
    currentPhase_ = Phase::Resolve;
    combatActive_ = false;

    auto playerUnits = board_.getPlayerUnitsOnBoard();
    auto enemyUnits = board_.getEnemyUnitsOnBoard();

    bool playerAlive = false;
    for (auto* u : playerUnits) {
        if (u->isAlive()) { playerAlive = true; break; }
    }

    bool enemyAlive = false;
    for (auto* u : enemyUnits) {
        if (u->isAlive()) { enemyAlive = true; break; }
    }

    if (!enemyAlive) {
        // 胜利奖励：+1金币
        addGold(1);
        std::cout << "第 " << currentRound_ << " 轮：胜利！+1金币（当前: " << gold_ << "）" << std::endl;
    } else {
        int damage = 10;
        playerHP_ -= damage;
        std::cout << "第 " << currentRound_ << " 轮：失败！扣除 " << damage << " HP" << std::endl;
    }

    // 将玩家单位的像素位置映射回最近的格子
    if (renderer_) {
        board_.forEachBoardUnit([&](const BoardNS::GridPos& pos, Unit* unit) {
            if (unit->owner() == Unit::Owner::PlayerCtrl && unit->isAlive()) {
                // 找到最近的空格子
                sf::Vector2f pixelPos = unit->getPosition();
                BoardNS::GridPos nearestPos = renderer_->screenToBoardCell(pixelPos);
                if (nearestPos.isValid() && nearestPos != pos) {
                    // 如果目标格子为空或就是自己，移动
                    if (board_.isBoardCellEmpty(nearestPos)) {
                        auto unitPtr = board_.removeFromBoard(pos);
                        board_.placeOnBoard(nearestPos, std::move(unitPtr));
                    }
                }
            }
        });
    }

    // 清除敌方单位
    for (int r = 0; r < board_.getRows(); ++r) {
        for (int c = 0; c < board_.getCols(); ++c) {
            BoardNS::GridPos pos{r, c};
            Unit* u = board_.getUnitOnBoard(pos);
            if (u && u->owner() == Unit::Owner::EnemyCtrl) {
                board_.removeFromBoard(pos);
            }
        }
    }

    // 恢复玩家单位HP，重置战斗状态，清空羁绊Buff
    board_.forEachBoardUnit([](const BoardNS::GridPos& pos, Unit* unit) {
        if (unit->owner() == Unit::Owner::PlayerCtrl) {
            unit->setHP(unit->getMaxHP());
            unit->setState(Unit::State::Idle);
            unit->setInCombat(false);
            unit->resetCombatState();
            unit->clearSynergyBuffs();
            // 重置位置到格子中心（由渲染器在下一帧处理）
        }
    });

    // 清空羁绊缓存
    synergyMgr_.clearSynergies();
}

// --- 经济系统 ---

void GameManager::addGold(int amount) {
    gold_ += amount;
    if (gold_ < 0) gold_ = 0;
}

bool GameManager::spendGold(int amount) {
    if (gold_ < amount) return false;
    gold_ -= amount;
    return true;
}

int GameManager::calculateIncome(bool wonLastRound) const {
    // 基础收入：5金币
    int income = 5;
    // 利息：每10金币+1，最多+5
    int interest = std::min(gold_ / 10, 5);
    income += interest;
    // 胜利奖励：+1金币
    if (wonLastRound) {
        income += 1;
    }
    return income;
}

// --- 商店系统 ---

bool GameManager::buyFromShop(int slotIndex) {
    // 检查槽位是否有效且有可购买单位
    if (!shop_.isSlotAvailable(slotIndex)) return false;

    // 检查金币是否足够
    int cost = shop_.getSlotCost(slotIndex);
    if (gold_ < cost) return false;

    // 检查Bench是否有空位
    int benchSlot = board_.findEmptyBenchSlot();
    if (benchSlot < 0) return false;

    // 购买单位
    auto unit = shop_.buyUnit(slotIndex);
    if (!unit) return false;

    // 扣除金币
    gold_ -= cost;

    // 设置为玩家单位
    unit->setOwner(Unit::Owner::PlayerCtrl);

    // 放入Bench
    board_.placeOnBench(benchSlot, std::move(unit));

    std::cout << "购买 " << shop_.getSlot(slotIndex).unitTypeName
              << "（-" << cost << "金币），剩余金币: " << gold_ << std::endl;
    return true;
}

bool GameManager::refreshShop() {
    // 手动刷新花费2金币
    if (!spendGold(2)) return false;
    shop_.refresh();
    std::cout << "刷新商店（-2金币），剩余金币: " << gold_ << std::endl;
    return true;
}

void GameManager::freeRefreshShop() {
    shop_.refresh();
}

// --- 存档相关 ---

void GameManager::saveToGameData() {
    auto& config = GameDataNS::g_GameData.Config();
    config.playerHP = playerHP_;
    config.round = currentRound_;
    config.gold = gold_;
    config.died = false;

    int boardCount = 0, benchCount = 0;
    board_.forEachBoardUnit([&](const BoardNS::GridPos&, Unit*) { boardCount++; });
    for (int i = 0; i < board_.getBenchSize(); ++i) {
        if (board_.getUnitOnBench(i)) benchCount++;
    }
    std::cout << "[saveToGameData] 保存: round=" << currentRound_
              << " playerHP=" << playerHP_
              << " gold=" << gold_
              << " 棋盘单位=" << boardCount
              << " Bench单位=" << benchCount << std::endl;

    bool ok = GameDataNS::SaveFullGame(board_);
    std::cout << "[saveToGameData] SaveFullGame " << (ok ? "成功" : "失败") << std::endl;
}

void GameManager::restoreFromSave() {
    auto& config = GameDataNS::g_GameData.GetConfig();
    playerHP_ = config.playerHP;
    gold_ = config.gold;
    currentRound_ = config.round;
    currentPhase_ = Phase::Prep;
    combatActive_ = false;
    combatTimer_ = 0.f;

    bool ok = GameDataNS::LoadFullGame(board_);
    std::cout << "[restoreFromSave] LoadFullGame " << (ok ? "成功" : "失败")
              << " round=" << currentRound_ << " playerHP=" << playerHP_
              << " gold=" << gold_ << std::endl;

    // 刷新商店
    freeRefreshShop();
    spawnEnemyUnits();
}

// --- 调试用 ---

void GameManager::addTestUnits() {
    auto w = std::make_unique<WarriorUnit>();
    w->setOwner(Unit::Owner::PlayerCtrl);
    board_.placeOnBench(board_.findEmptyBenchSlot(), std::move(w));

    auto a = std::make_unique<ArcherUnit>();
    a->setOwner(Unit::Owner::PlayerCtrl);
    board_.placeOnBench(board_.findEmptyBenchSlot(), std::move(a));

    auto m = std::make_unique<MageUnit>();
    m->setOwner(Unit::Owner::PlayerCtrl);
    board_.placeOnBench(board_.findEmptyBenchSlot(), std::move(m));

    auto t = std::make_unique<TankUnit>();
    t->setOwner(Unit::Owner::PlayerCtrl);
    board_.placeOnBench(board_.findEmptyBenchSlot(), std::move(t));

    auto as = std::make_unique<AssassinUnit>();
    as->setOwner(Unit::Owner::PlayerCtrl);
    board_.placeOnBench(board_.findEmptyBenchSlot(), std::move(as));

    spawnEnemyUnits();
}

// --- 羁绊系统 ---

void GameManager::recalculateSynergies() {
    synergyMgr_.calculateSynergies(board_);
}

} // namespace GamePhaseNS
