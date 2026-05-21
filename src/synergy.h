#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>

// 前向声明
class Unit;

namespace BoardNS { class Board; }

namespace SynergyNS {

// Buff 类型枚举
enum class BuffType {
    DamageReduction,   // 减伤比例（0.0~1.0）
    ATKBonus,          // 攻击力加成
    AOEMultiplier,     // AOE伤害倍率加成（0.0~N）
    CritBonus,         // 暴击率加成（0.0~1.0）
    KillResetAttack    // 机制改变：击杀后重置攻击冷却（intValue=1 表示启用）
};

// 单个 Buff 效果
struct SynergyBuff {
    BuffType type;
    float value = 0.f;          // 浮点值（减伤比例、暴击率等）
    int intValue = 0;           // 整数值（ATK加成等）
};

// 羁绊层级：激活阈值 + Buff 列表
struct SynergyLevel {
    int threshold = 0;          // 需要的单位数量
    std::vector<SynergyBuff> buffs;
};

// 羁绊定义
struct SynergyDef {
    std::string traitName;      // 对应的 trait 名称（如"战士"）
    std::string displayName;    // 显示名称
    std::vector<SynergyLevel> levels;  // 层级列表（按阈值从小到大）
};

// 激活的羁绊信息（供 GUI 显示）
struct ActiveSynergy {
    std::string traitName;      // 羁绊名称
    int count = 0;              // 当前场上该 trait 的单位数
    int level = 0;              // 当前激活的层级（0=未激活, 1/2=层级）
    int threshold = 0;          // 当前层级的激活阈值
    std::string buffDescription;// Buff 描述文字
};

// 羁绊管理器：独立系统，通过查询接口提供 Buff 数据
class SynergyManager {
public:
    SynergyManager();

    // 根据棋盘上玩家单位计算激活的羁绊（战斗开始时调用一次）
    void calculateSynergies(const BoardNS::Board& board);

    // 清空缓存的羁绊数据（战斗结束时调用）
    void clearSynergies();

    // --- 查询接口（战斗中调用，获取某单位因羁绊获得的 Buff） ---

    // 查询某单位因羁绊获得的 ATK 加成
    int getATKBonus(const Unit* unit) const;

    // 查询某单位因羁绊获得的减伤比例（0.0~1.0）
    float getDamageReduction(const Unit* unit) const;

    // 查询某单位因羁绊获得的暴击率加成（0.0~1.0）
    float getCritBonus(const Unit* unit) const;

    // 查询法师因羁绊获得的 AOE 伤害倍率加成（0.0~N）
    float getAOEMultiplier(const Unit* unit) const;

    // 查询某单位是否有击杀重置攻击冷却的机制改变 Buff
    bool hasKillResetAttack(const Unit* unit) const;

    // --- GUI 查询 ---
    // 获取当前激活的羁绊列表（供 ImGui 显示）
    const std::vector<ActiveSynergy>& getActiveSynergies() const;

    // 获取所有羁绊定义（供 GUI 显示完整羁绊列表）
    const std::vector<SynergyDef>& getAllSynergyDefs() const;

private:
    // 所有羁绊定义（数据驱动）
    std::vector<SynergyDef> synergyDefs_;

    // 缓存的激活羁绊
    std::vector<ActiveSynergy> activeSynergies_;

    // 缓存每个 trait 激活的 Buff（traitName → Buff列表）
    std::map<std::string, std::vector<SynergyBuff>> activeBuffsByTrait_;

    // 内部辅助：获取某 trait 激活的所有 Buff
    const std::vector<SynergyBuff>* getBuffsForTrait(const std::string& traitName) const;
};

} // namespace SynergyNS