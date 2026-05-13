#include "game_phase.h"
#include <algorithm>
#include <iostream>

namespace GamePhaseNS {

GameManager::GameManager()
: board_(8, 8, 8)
, rng_(std::random_device{}()) {
}

// --- 核心循环 ---

// 每帧调用：根据当前阶段分发到对应的更新函数
void GameManager::update(float dt) {
    switch (currentPhase_) {
        case Phase::Prep:
            // 准备阶段等待玩家操作，不需要自动更新
            break;
        case Phase::Combat:
            updateCombat(dt);
            if (isCombatOver()) {
                resolveRound();
            }
            break;
        case Phase::Resolve:
            // 结算后自动进入下一轮
            break;
    }
}

// --- 阶段控制 ---

// 直接设置当前游戏阶段
void GameManager::setPhase(Phase phase) {
    currentPhase_ = phase;
}

// 推进到下一阶段：Prep→Combat→Resolve→Prep（循环）
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

// 开始新一轮：回合数+1，进入准备阶段，生成新的敌方单位
void GameManager::startNewRound() {
    currentPhase_ = Phase::Prep;
    currentRound_++;
    // 清除上一轮的敌方单位
    // 保留玩家在棋盘上的单位
    // 生成新的敌方单位
    spawnEnemyUnits();
    std::cout << "第 " << currentRound_ << " 轮开始（准备阶段）" << std::endl;
}

// 生成敌方单位：清除旧敌方，在棋盘上半部分随机放置，属性随回合数缩放
void GameManager::spawnEnemyUnits() {
    // 清除旧的敌方单位
    auto enemies = board_.getEnemyUnitsOnBoard();
    for (auto* e : enemies) {
        // 找到并移除
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

    // 根据回合数生成敌方单位（简单递增难度）
    int numEnemies = std::min(1 + currentRound_ / 2, 5);
    std::uniform_int_distribution<int> typeDist(0, 4);

    // 敌方出现在棋盘右半部分（col 4-7）
    int halfCols = board_.getCols() / 2;
    std::uniform_int_distribution<int> colDist(halfCols, board_.getCols() - 1);
    // 行范围：全部行均可
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
            // 根据回合数增强敌方属性
            float scaleFactor = 1.0f + (currentRound_ - 1) * 0.15f;
            enemy->setMaxHP(static_cast<int>(enemy->getMaxHP() * scaleFactor));
            enemy->setHP(enemy->getMaxHP());
            enemy->setATK(static_cast<int>(enemy->getATK() * scaleFactor));

            board_.placeOnBoard(pos, std::move(enemy));
        }
    }
}

// 将Bench上指定槽位的单位移动到棋盘指定位置，目标非空则尝试交换
bool GameManager::moveBenchToBoard(int benchSlot, const BoardNS::GridPos& pos) {
    if (benchSlot < 0 || board_.isBoardCellEmpty(pos) == false) {
        // 目标格子非空，尝试交换
        return swapBenchAndBoard(benchSlot, pos);
    }
    auto unit = board_.removeFromBench(benchSlot);
    if (!unit) return false;
    if (!board_.placeOnBoard(pos, std::move(unit))) {
        // 放置失败，放回Bench
        board_.placeOnBench(benchSlot, std::move(unit));
        return false;
    }
    return true;
}

// 将棋盘上指定位置的单位移动到Bench指定槽位
bool GameManager::moveBoardToBench(const BoardNS::GridPos& pos, int benchSlot) {
    if (benchSlot < 0) return false;
    auto unit = board_.removeFromBoard(pos);
    if (!unit) return false;
    if (!board_.placeOnBench(benchSlot, std::move(unit))) {
        // 放置失败，放回棋盘
        board_.placeOnBoard(pos, std::move(unit));
        return false;
    }
    return true;
}

// 在棋盘内移动单位，若目标有单位则交换两者位置
bool GameManager::moveOnBoard(const BoardNS::GridPos& from, const BoardNS::GridPos& to) {
    if (from == to) return false;
    auto unitFrom = board_.removeFromBoard(from);
    if (!unitFrom) return false;

    auto unitTo = board_.removeFromBoard(to);
    // 先放from的单位到to
    if (!board_.placeOnBoard(to, std::move(unitFrom))) {
        // 失败，还原
        board_.placeOnBoard(from, std::move(unitFrom));
        if (unitTo) board_.placeOnBoard(to, std::move(unitTo));
        return false;
    }
    // 如果to原来有单位，放到from（交换）
    if (unitTo) {
        board_.placeOnBoard(from, std::move(unitTo));
    }
    return true;
}

// 交换Bench槽位和棋盘位置上的单位（其中一个可以为空）
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

// 开始战斗：切换到Combat阶段，重置计时器，将所有单位设为Idle状态
void GameManager::startCombat() {
    currentPhase_ = Phase::Combat;
    combatTimer_ = 0.f;
    combatActive_ = true;

    // 设置所有单位状态为Idle
    board_.forEachBoardUnit([](const BoardNS::GridPos&, Unit* unit) {
        if (unit->isAlive()) {
            unit->setState(Unit::State::Idle);
        }
    });

    std::cout << "战斗开始！" << std::endl;
}

// 更新战斗逻辑：每隔固定时间执行一轮攻击，每个单位攻击最近的敌方
void GameManager::updateCombat(float dt) {
    if (!combatActive_) return;

    combatTimer_ += dt;
    if (combatTimer_ < combatTickInterval_) return;
    combatTimer_ -= combatTickInterval_;

    // 获取所有存活单位
    auto playerUnits = board_.getPlayerUnitsOnBoard();
    auto enemyUnits = board_.getEnemyUnitsOnBoard();

    // 移除已死亡的单位
    auto removeDead = [&](std::vector<Unit*>& units) {
        units.erase(
            std::remove_if(units.begin(), units.end(),
                [](Unit* u) { return !u->isAlive(); }),
            units.end());
    };
    removeDead(playerUnits);
    removeDead(enemyUnits);

    // 每个玩家单位攻击一个敌方单位
    for (auto* attacker : playerUnits) {
        if (!attacker->isAlive()) continue;
        if (enemyUnits.empty()) break;

        // 找最近的敌方单位
        Unit* closest = nullptr;
        float closestDist = std::numeric_limits<float>::max();
        for (auto* target : enemyUnits) {
            if (!target->isAlive()) continue;
            float dist = std::hypot(
                attacker->getPosition().x - target->getPosition().x,
                attacker->getPosition().y - target->getPosition().y);
            if (dist < closestDist) {
                closestDist = dist;
                closest = target;
            }
        }
        if (closest) {
            attacker->setState(Unit::State::Attacking);
            attacker->attack(*closest);
        }
    }

    // 每个敌方单位攻击一个玩家单位
    for (auto* attacker : enemyUnits) {
        if (!attacker->isAlive()) continue;
        if (playerUnits.empty()) break;

        Unit* closest = nullptr;
        float closestDist = std::numeric_limits<float>::max();
        for (auto* target : playerUnits) {
            if (!target->isAlive()) continue;
            float dist = std::hypot(
                attacker->getPosition().x - target->getPosition().x,
                attacker->getPosition().y - target->getPosition().y);
            if (dist < closestDist) {
                closestDist = dist;
                closest = target;
            }
        }
        if (closest) {
            attacker->setState(Unit::State::Attacking);
            attacker->attack(*closest);
        }
    }

    // 更新所有单位
    board_.forEachBoardUnit([dt](const BoardNS::GridPos&, Unit* unit) {
        unit->update(dt);
    });
}

// 检查战斗是否结束：玩家方或敌方任一全灭则结束
bool GameManager::isCombatOver() const {
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

// 结算本轮：判断胜负，失败扣HP，清除敌方单位，恢复玩家单位血量
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
        std::cout << "第 " << currentRound_ << " 轮：胜利！" << std::endl;
    } else {
        // 玩家失败，扣除HP
        int damage = 10;
        playerHP_ -= damage;
        std::cout << "第 " << currentRound_ << " 轮：失败！扣除 " << damage << " HP" << std::endl;
    }

    // 清除死亡的单位和所有敌方单位
    // 清除敌方
    for (int r = 0; r < board_.getRows(); ++r) {
        for (int c = 0; c < board_.getCols(); ++c) {
            BoardNS::GridPos pos{r, c};
            Unit* u = board_.getUnitOnBoard(pos);
            if (u && u->owner() == Unit::Owner::EnemyCtrl) {
                board_.removeFromBoard(pos);
            }
        }
    }

    // 恢复玩家单位HP（阶段一简化处理）
    board_.forEachBoardUnit([](const BoardNS::GridPos&, Unit* unit) {
        if (unit->owner() == Unit::Owner::PlayerCtrl) {
            unit->setHP(unit->getMaxHP());
            unit->setState(Unit::State::Idle);
        }
    });
}

// --- 调试用 ---

// 添加5种测试单位到Bench并生成初始敌方单位
void GameManager::addTestUnits() {
    // 添加一些测试单位到Bench
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

    // 生成初始敌方
    spawnEnemyUnits();
}

} // namespace GamePhaseNS