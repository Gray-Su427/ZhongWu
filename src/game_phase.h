#pragma once
#include "board.h"
#include "unit_types.h"
#include "shop.h"
#include <memory>
#include <random>

// 前向声明，避免循环依赖
namespace RendererNS { class GameRenderer; }

namespace GamePhaseNS {

// 游戏阶段枚举
enum class Phase {
    Prep,     // 准备阶段：布阵、购买
    Combat,   // 战斗阶段：自动战斗
    Resolve   // 结算阶段：计算结果、进入下一轮
};

// 游戏管理器：管理整个游戏流程（阶段切换、战斗逻辑、回合控制）
class GameManager {
public:
    GameManager();

    // --- 核心循环 ---
    // 每帧调用，根据当前阶段执行对应逻辑
    void update(float dt);

    // --- 阶段控制 ---
    // 获取当前游戏阶段
    Phase getCurrentPhase() const { return currentPhase_; }
    // 直接设置游戏阶段
    void setPhase(Phase phase);
    // 推进到下一阶段（Prep→Combat→Resolve→Prep）
    void nextPhase();

    // --- 准备阶段操作 ---
    // 开始新一轮（进入Prep阶段，回合数+1，生成敌方）
    void startNewRound();
    // 在棋盘上半部分随机生成敌方单位，属性随回合数增强
    void spawnEnemyUnits();
    // 将Bench上指定槽位的单位移动到棋盘指定位置
    bool moveBenchToBoard(int benchSlot, const BoardNS::GridPos& pos);
    // 将棋盘上指定位置的单位移动到Bench指定槽位
    bool moveBoardToBench(const BoardNS::GridPos& pos, int benchSlot);
    // 在棋盘内移动单位（支持交换两个位置的单位）
    bool moveOnBoard(const BoardNS::GridPos& from, const BoardNS::GridPos& to);
    // 交换Bench槽位和棋盘位置上的单位
    bool swapBenchAndBoard(int benchSlot, const BoardNS::GridPos& pos);

    // --- 战斗阶段 ---
    // 开始战斗，初始化所有单位像素位置和战斗状态
    void startCombat();
    // 每帧更新战斗逻辑（实时FSM，每帧调用各单位updateCombat）
    void updateCombat(float dt);
    // 检查战斗是否结束（一方全灭或超时）
    bool isCombatOver() const;
    // 设置渲染器引用（用于获取布局信息）
    void setRenderer(RendererNS::GameRenderer* renderer);

    // --- 结算阶段 ---
    // 结算本轮结果：判断胜负，失败则扣除玩家HP，清除敌方单位，恢复玩家单位
    void resolveRound();

    // --- 查询 ---
    // 获取棋盘引用（可修改）
    BoardNS::Board& getBoard() { return board_; }
    // 获取棋盘常量引用（只读）
    const BoardNS::Board& getBoard() const { return board_; }

    // 获取当前回合数
    int getCurrentRound() const { return currentRound_; }
    // 获取玩家当前HP
    int getPlayerHP() const { return playerHP_; }
    // 设置玩家HP
    void setPlayerHP(int hp) { playerHP_ = hp; }

    // --- 经济系统 ---
    // 获取玩家当前金币
    int getGold() const { return gold_; }
    // 设置玩家金币
    void setGold(int gold) { gold_ = gold; }
    // 增加金币（收入、奖励等）
    void addGold(int amount);
    // 花费金币（购买等），返回是否成功（余额不足返回false）
    bool spendGold(int amount);
    // 计算本轮收入：基础收入5 + 利息(每10金+1, 最多+5) + 胜利奖励
    int calculateIncome(bool wonLastRound) const;

    // --- 商店系统 ---
    // 获取商店引用（只读）
    const ShopNS::Shop& getShop() const { return shop_; }
    // 获取商店引用（可修改）
    ShopNS::Shop& getShop() { return shop_; }
    // 从商店购买单位到Bench（扣金币+创建单位+放入Bench），返回是否成功
    bool buyFromShop(int slotIndex);
    // 手动刷新商店（花费2金币），返回是否成功
    bool refreshShop();
    // 自动刷新商店（免费，每轮开始时调用）
    void freeRefreshShop();

    // --- 存档相关 ---
    // 将当前游戏状态保存到 GameData（含棋盘单位、金币、商店）
    void saveToGameData();
    // 从 GameData 恢复游戏状态（含棋盘单位、金币、商店）
    void restoreFromSave();

    // --- 调试用 ---
    // 添加5种测试单位到Bench并生成初始敌方
    void addTestUnits();

private:
    BoardNS::Board board_;          // 棋盘（含Bench）
    Phase currentPhase_ = Phase::Prep;  // 当前游戏阶段
    int currentRound_ = 1;          // 当前回合数
    int playerHP_ = 100;            // 玩家HP
    int gold_ = 20;                 // 玩家金币

    // 商店
    ShopNS::Shop shop_;             // 商店实例

    // 战斗相关
    float combatTimer_ = 0.f;       // 战斗计时器（已用时间）
    float combatMaxTime_ = 60.f;    // 战斗超时时间（秒）
    bool combatActive_ = false;     // 战斗是否进行中
    RendererNS::GameRenderer* renderer_ = nullptr;  // 渲染器引用（获取布局信息）

    // 随机数生成器（用于敌方生成）
    std::mt19937 rng_;
};

} // namespace GamePhaseNS