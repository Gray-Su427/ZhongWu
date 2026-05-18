#pragma once
#include "unit.h"

// ==================== 工厂函数 ====================
// 根据类型名称创建单位实例（用于反序列化）
// 支持的名称："战士"、"弓箭手"、"法师"、"坦克"、"刺客"
std::unique_ptr<Unit> createUnitByName(const std::string& typeName,
                                        sf::Vector2f pos = {0.f, 0.f},
                                        int starRank = 1);

// ==================== 战士（近战，均衡型） ====================
class WarriorUnit : public Unit {
public:
    WarriorUnit(sf::Vector2f pos = {0.f, 0.f}, int starRank = 1);
    std::string getUnitTypeName() const override { return "战士"; }
    void update(float dt) override;
    void updateCombat(float dt, float uniformScale) override;
    std::unique_ptr<Unit> clone() const override;
};

// ==================== 弓箭手（远程，高攻击） ====================
class ArcherUnit : public Unit {
public:
    ArcherUnit(sf::Vector2f pos = {0.f, 0.f}, int starRank = 1);
    std::string getUnitTypeName() const override { return "弓箭手"; }
    void update(float dt) override;
    void updateCombat(float dt, float uniformScale) override;
    std::unique_ptr<Unit> clone() const override;
};

// ==================== 法师（远程，有魔法技能） ====================
class MageUnit : public Unit {
public:
    MageUnit(sf::Vector2f pos = {0.f, 0.f}, int starRank = 1);
    std::string getUnitTypeName() const override { return "法师"; }
    void update(float dt) override;
    void updateCombat(float dt, float uniformScale) override;
    void performAttack(Unit& target, int atkBonus = 0, float critBonus = 0.f) override;  // 法师攻击增加Mana
    // AOE技能：对目标周围所有敌方造成伤害
    // aoeMultiplier: 羁绊提供的AOE伤害倍率加成
    void castAOE(const std::vector<Unit*>& enemies, float uniformScale, float aoeMultiplier = 0.f);
    std::unique_ptr<Unit> clone() const override;
};

// ==================== 坦克（近战，高血量） ====================
class TankUnit : public Unit {
public:
    TankUnit(sf::Vector2f pos = {0.f, 0.f}, int starRank = 1);
    std::string getUnitTypeName() const override { return "坦克"; }
    void update(float dt) override;
    void updateCombat(float dt, float uniformScale) override;
    void takeDamage(int dmg) override;  // 坦克减伤20%
    std::unique_ptr<Unit> clone() const override;
};

// ==================== 刺客（近战，高爆发） ====================
class AssassinUnit : public Unit {
public:
    AssassinUnit(sf::Vector2f pos = {0.f, 0.f}, int starRank = 1);
    std::string getUnitTypeName() const override { return "刺客"; }
    void update(float dt) override;
    void updateCombat(float dt, float uniformScale) override;
    void performAttack(Unit& target, int atkBonus = 0, float critBonus = 0.f) override;  // 刺客暴击
    std::unique_ptr<Unit> clone() const override;
};
