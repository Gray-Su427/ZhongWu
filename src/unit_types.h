#pragma once
#include "unit.h"

// ==================== 战士（近战，均衡型） ====================
// 特点：中等血量和攻击力，近战范围
// 羁绊：战士、近战
class WarriorUnit : public Unit {
public:
    WarriorUnit(sf::Vector2f pos = {0.f, 0.f}, int starRank = 1);
    // 每帧更新FSM状态
    void update(float dt) override;
    // 克隆当前单位（用于复制）
    std::unique_ptr<Unit> clone() const override;
};

// ==================== 弓箭手（远程，高攻击） ====================
// 特点：较低血量，高攻击力，远程攻击但有最小射程
// 羁绊：弓箭手、远程
class ArcherUnit : public Unit {
public:
    ArcherUnit(sf::Vector2f pos = {0.f, 0.f}, int starRank = 1);
    void update(float dt) override;
    std::unique_ptr<Unit> clone() const override;
};

// ==================== 法师（远程，有魔法技能） ====================
// 特点：低血量，中等攻击力，远程攻击，拥有魔法值
// 羁绊：法师、远程
class MageUnit : public Unit {
public:
    MageUnit(sf::Vector2f pos = {0.f, 0.f}, int starRank = 1);
    void update(float dt) override;
    std::unique_ptr<Unit> clone() const override;
};

// ==================== 坦克（近战，高血量） ====================
// 特点：高血量，低攻击力，近战范围
// 羁绊：坦克、近战
class TankUnit : public Unit {
public:
    TankUnit(sf::Vector2f pos = {0.f, 0.f}, int starRank = 1);
    void update(float dt) override;
    std::unique_ptr<Unit> clone() const override;
};

// ==================== 刺客（近战，高爆发） ====================
// 特点：中等血量，极高攻击力，近战范围
// 羁绊：刺客、近战
class AssassinUnit : public Unit {
public:
    AssassinUnit(sf::Vector2f pos = {0.f, 0.f}, int starRank = 1);
    void update(float dt) override;
    std::unique_ptr<Unit> clone() const override;
};
