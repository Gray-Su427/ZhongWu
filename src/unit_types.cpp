#include "unit_types.h"
#include <cmath>
#include <random>
#include <algorithm>

// ==================== 工厂函数 ====================
std::unique_ptr<Unit> createUnitByName(const std::string& typeName,
                                        sf::Vector2f pos, int starRank) {
    if (typeName == "战士")     return std::make_unique<WarriorUnit>(pos, starRank);
    if (typeName == "弓箭手")   return std::make_unique<ArcherUnit>(pos, starRank);
    if (typeName == "法师")     return std::make_unique<MageUnit>(pos, starRank);
    if (typeName == "坦克")     return std::make_unique<TankUnit>(pos, starRank);
    if (typeName == "刺客")     return std::make_unique<AssassinUnit>(pos, starRank);
    return nullptr;
}

// ==================== 辅助函数 ====================
// 归一化向量，零向量返回零
static sf::Vector2f normalizeSafe(const sf::Vector2f& v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y);
    if (len < 0.001f) return {0.f, 0.f};
    return v / len;
}

// ==================== 战士 ====================
WarriorUnit::WarriorUnit(sf::Vector2f pos, int starRank)
: Unit(pos, starRank) {
    setUnitName("战士");
    setMaxHP(300);
    setHP(300);
    setATK(25);
    setRange(48.f);
    setMinRange(0.f);
    setMaxMana(0);
    setMana(0);
    setMoveSpeed(150.f);
    setAttackSpeed(1.0f);
    setDodgeChance(0.05f);
    color = sf::Color(200, 60, 60);
    addTrait("战士");
    addTrait("近战");
    syncShape();
}

void WarriorUnit::update(float dt) {
    (void)dt;
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;
    m_state = State::Idle;
}

void WarriorUnit::updateCombat(float dt, float uniformScale) {
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;

    float scaledRange = max_range * uniformScale;
    float scaledMinRange = min_range * uniformScale;
    float scaledSpeed = moveSpeed_ * uniformScale;

    switch (m_state) {
        case State::Idle: {
            // 搜索最近敌人由外部设置target_
            if (target_ && target_->isAlive()) {
                m_state = State::Moving;
            }
            break;
        }
        case State::Moving: {
            if (!target_ || !target_->isAlive()) {
                target_ = nullptr;
                m_state = State::Idle;
                break;
            }
            sf::Vector2f diff = target_->getPosition() - position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            if (dist < scaledMinRange) {
                // 太近了，远离
                sf::Vector2f awayDir = normalizeSafe(position - target_->getPosition());
                position += awayDir * scaledSpeed * dt;
            } else if (dist > scaledRange) {
                // 太远了，靠近
                sf::Vector2f dir = normalizeSafe(diff);
                position += dir * scaledSpeed * dt;
            } else {
                // 在攻击范围内
                m_state = State::Attacking;
                attackTimer_ = attackSpeed_; // 立即攻击
            }
            clampToBoardBounds();
            break;
        }
        case State::Attacking: {
            if (!target_ || !target_->isAlive()) {
                target_ = nullptr;
                m_state = State::Idle;
                break;
            }
            sf::Vector2f diff = target_->getPosition() - position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            // 目标超出范围，追击
            if (dist > scaledRange * 1.2f) {
                m_state = State::Moving;
                break;
            }

            attackTimer_ += dt;
            if (attackTimer_ >= attackSpeed_) {
                attackTimer_ = 0.f;
                performAttack(*target_);
            }
            break;
        }
        case State::Casting: {
            // 战士无技能，不应到达这里
            m_state = State::Attacking;
            break;
        }
        case State::Dead:
            break;
    }
}

std::unique_ptr<Unit> WarriorUnit::clone() const {
    return std::make_unique<WarriorUnit>(position, starRank);
}

// ==================== 弓箭手 ====================
ArcherUnit::ArcherUnit(sf::Vector2f pos, int starRank)
: Unit(pos, starRank) {
    setUnitName("弓箭手");
    setMaxHP(200);
    setHP(200);
    setATK(30);
    setRange(160.f);
    setMinRange(48.f);
    setMaxMana(0);
    setMana(0);
    setMoveSpeed(120.f);
    setAttackSpeed(1.2f);
    setDodgeChance(0.10f);
    color = sf::Color(60, 180, 60);
    addTrait("弓箭手");
    addTrait("远程");
    syncShape();
}

void ArcherUnit::update(float dt) {
    (void)dt;
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;
    m_state = State::Idle;
}

void ArcherUnit::updateCombat(float dt, float uniformScale) {
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;

    float scaledRange = max_range * uniformScale;
    float scaledMinRange = min_range * uniformScale;
    float scaledSpeed = moveSpeed_ * uniformScale;

    switch (m_state) {
        case State::Idle: {
            if (target_ && target_->isAlive()) {
                m_state = State::Moving;
            }
            break;
        }
        case State::Moving: {
            if (!target_ || !target_->isAlive()) {
                target_ = nullptr;
                m_state = State::Idle;
                break;
            }
            sf::Vector2f diff = target_->getPosition() - position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            if (dist < scaledMinRange) {
                // 太近了，远离目标
                sf::Vector2f awayDir = normalizeSafe(position - target_->getPosition());
                position += awayDir * scaledSpeed * dt;
            } else if (dist > scaledRange) {
                // 太远了，靠近目标
                sf::Vector2f dir = normalizeSafe(diff);
                position += dir * scaledSpeed * dt;
            } else {
                // 在攻击范围内
                m_state = State::Attacking;
                attackTimer_ = attackSpeed_;
            }
            clampToBoardBounds();
            break;
        }
        case State::Attacking: {
            if (!target_ || !target_->isAlive()) {
                target_ = nullptr;
                m_state = State::Idle;
                break;
            }
            sf::Vector2f diff = target_->getPosition() - position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            // 太近了，后退
            if (dist < scaledMinRange) {
                m_state = State::Moving;
                break;
            }
            // 目标超出范围，追击
            if (dist > scaledRange * 1.2f) {
                m_state = State::Moving;
                break;
            }

            attackTimer_ += dt;
            if (attackTimer_ >= attackSpeed_) {
                attackTimer_ = 0.f;
                performAttack(*target_);
            }
            break;
        }
        case State::Casting: {
            m_state = State::Attacking;
            break;
        }
        case State::Dead:
            break;
    }
}

std::unique_ptr<Unit> ArcherUnit::clone() const {
    return std::make_unique<ArcherUnit>(position, starRank);
}

// ==================== 法师 ====================
MageUnit::MageUnit(sf::Vector2f pos, int starRank)
: Unit(pos, starRank) {
    setUnitName("法师");
    setMaxHP(180);
    setHP(180);
    setATK(20);
    setRange(140.f);
    setMinRange(32.f);
    setMaxMana(60);
    setMana(0);
    setMoveSpeed(100.f);
    setAttackSpeed(1.5f);
    setDodgeChance(0.05f);
    color = sf::Color(100, 100, 220);
    addTrait("法师");
    addTrait("远程");
    syncShape();
}

void MageUnit::update(float dt) {
    (void)dt;
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;
    m_state = State::Idle;
}

void MageUnit::updateCombat(float dt, float uniformScale) {
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;

    float scaledRange = max_range * uniformScale;
    float scaledMinRange = min_range * uniformScale;
    float scaledSpeed = moveSpeed_ * uniformScale;

    switch (m_state) {
        case State::Idle: {
            if (target_ && target_->isAlive()) {
                m_state = State::Moving;
            }
            break;
        }
        case State::Moving: {
            if (!target_ || !target_->isAlive()) {
                target_ = nullptr;
                m_state = State::Idle;
                break;
            }
            sf::Vector2f diff = target_->getPosition() - position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            if (dist < scaledMinRange) {
                sf::Vector2f awayDir = normalizeSafe(position - target_->getPosition());
                position += awayDir * scaledSpeed * dt;
            } else if (dist > scaledRange) {
                sf::Vector2f dir = normalizeSafe(diff);
                position += dir * scaledSpeed * dt;
            } else {
                m_state = State::Attacking;
                attackTimer_ = attackSpeed_;
            }
            clampToBoardBounds();
            break;
        }
        case State::Attacking: {
            if (!target_ || !target_->isAlive()) {
                target_ = nullptr;
                m_state = State::Idle;
                break;
            }
            sf::Vector2f diff = target_->getPosition() - position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            if (dist < scaledMinRange) {
                m_state = State::Moving;
                break;
            }
            if (dist > scaledRange * 1.2f) {
                m_state = State::Moving;
                break;
            }

            // Mana满，施放AOE
            if (mana >= maxMana) {
                m_state = State::Casting;
                break;
            }

            attackTimer_ += dt;
            if (attackTimer_ >= attackSpeed_) {
                attackTimer_ = 0.f;
                performAttack(*target_);
            }
            break;
        }
        case State::Casting: {
            // 施放AOE技能（需要外部提供敌人列表，这里先对target施放）
            // 实际AOE在game_phase中调用castAOE
            // 这里只消耗Mana并切回Attacking
            mana = 0;
            m_state = State::Attacking;
            attackTimer_ = 0.f;
            break;
        }
        case State::Dead:
            break;
    }
}

void MageUnit::performAttack(Unit& target) {
    if (!isAlive() || !target.isAlive()) return;

    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    if (dist(rng) < target.getDodgeChance()) {
        return;
    }

    target.takeDamage(atk);

    // 法师攻击增加Mana
    if (maxMana > 0) {
        mana = std::min(mana + 10, maxMana);
    }
}

void MageUnit::castAOE(const std::vector<Unit*>& enemies, float uniformScale) {
    if (!target_ || !target_->isAlive()) return;

    float aoeRadius = 128.f * uniformScale;
    int aoeDamage = static_cast<int>(atk * 1.5f);

    for (auto* enemy : enemies) {
        if (!enemy || !enemy->isAlive()) continue;
        sf::Vector2f diff = enemy->getPosition() - target_->getPosition();
        float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        if (dist <= aoeRadius) {
            enemy->takeDamage(aoeDamage);
        }
    }
}

std::unique_ptr<Unit> MageUnit::clone() const {
    return std::make_unique<MageUnit>(position, starRank);
}

// ==================== 坦克 ====================
TankUnit::TankUnit(sf::Vector2f pos, int starRank)
: Unit(pos, starRank) {
    setUnitName("坦克");
    setMaxHP(500);
    setHP(500);
    setATK(15);
    setRange(48.f);
    setMinRange(0.f);
    setMaxMana(0);
    setMana(0);
    setMoveSpeed(80.f);
    setAttackSpeed(1.5f);
    setDodgeChance(0.03f);
    color = sf::Color(180, 180, 60);
    addTrait("坦克");
    addTrait("近战");
    syncShape();
}

void TankUnit::update(float dt) {
    (void)dt;
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;
    m_state = State::Idle;
}

void TankUnit::updateCombat(float dt, float uniformScale) {
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;

    float scaledRange = max_range * uniformScale;
    float scaledMinRange = min_range * uniformScale;
    float scaledSpeed = moveSpeed_ * uniformScale;

    switch (m_state) {
        case State::Idle: {
            if (target_ && target_->isAlive()) {
                m_state = State::Moving;
            }
            break;
        }
        case State::Moving: {
            if (!target_ || !target_->isAlive()) {
                target_ = nullptr;
                m_state = State::Idle;
                break;
            }
            sf::Vector2f diff = target_->getPosition() - position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            if (dist < scaledMinRange) {
                sf::Vector2f awayDir = normalizeSafe(position - target_->getPosition());
                position += awayDir * scaledSpeed * dt;
            } else if (dist > scaledRange) {
                sf::Vector2f dir = normalizeSafe(diff);
                position += dir * scaledSpeed * dt;
            } else {
                m_state = State::Attacking;
                attackTimer_ = attackSpeed_;
            }
            clampToBoardBounds();
            break;
        }
        case State::Attacking: {
            if (!target_ || !target_->isAlive()) {
                target_ = nullptr;
                m_state = State::Idle;
                break;
            }
            sf::Vector2f diff = target_->getPosition() - position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            if (dist > scaledRange * 1.2f) {
                m_state = State::Moving;
                break;
            }

            attackTimer_ += dt;
            if (attackTimer_ >= attackSpeed_) {
                attackTimer_ = 0.f;
                performAttack(*target_);
            }
            break;
        }
        case State::Casting: {
            m_state = State::Attacking;
            break;
        }
        case State::Dead:
            break;
    }
}

void TankUnit::takeDamage(int dmg) {
    if (!isAlive()) return;
    // 坦克减伤20%
    int reduced = static_cast<int>(dmg * 0.8f);
    setHP(hp - reduced);
}

std::unique_ptr<Unit> TankUnit::clone() const {
    return std::make_unique<TankUnit>(position, starRank);
}

// ==================== 刺客 ====================
AssassinUnit::AssassinUnit(sf::Vector2f pos, int starRank)
: Unit(pos, starRank) {
    setUnitName("刺客");
    setMaxHP(220);
    setHP(220);
    setATK(40);
    setRange(48.f);
    setMinRange(0.f);
    setMaxMana(0);
    setMana(0);
    setMoveSpeed(200.f);
    setAttackSpeed(0.8f);
    setDodgeChance(0.15f);
    color = sf::Color(180, 60, 180);
    addTrait("刺客");
    addTrait("近战");
    syncShape();
}

void AssassinUnit::update(float dt) {
    (void)dt;
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;
    m_state = State::Idle;
}

void AssassinUnit::updateCombat(float dt, float uniformScale) {
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;

    float scaledRange = max_range * uniformScale;
    float scaledMinRange = min_range * uniformScale;
    float scaledSpeed = moveSpeed_ * uniformScale;

    switch (m_state) {
        case State::Idle: {
            if (target_ && target_->isAlive()) {
                m_state = State::Moving;
            }
            break;
        }
        case State::Moving: {
            if (!target_ || !target_->isAlive()) {
                target_ = nullptr;
                m_state = State::Idle;
                break;
            }
            sf::Vector2f diff = target_->getPosition() - position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            if (dist < scaledMinRange) {
                sf::Vector2f awayDir = normalizeSafe(position - target_->getPosition());
                position += awayDir * scaledSpeed * dt;
            } else if (dist > scaledRange) {
                sf::Vector2f dir = normalizeSafe(diff);
                position += dir * scaledSpeed * dt;
            } else {
                m_state = State::Attacking;
                attackTimer_ = attackSpeed_;
            }
            clampToBoardBounds();
            break;
        }
        case State::Attacking: {
            if (!target_ || !target_->isAlive()) {
                target_ = nullptr;
                m_state = State::Idle;
                break;
            }
            sf::Vector2f diff = target_->getPosition() - position;
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            if (dist > scaledRange * 1.2f) {
                m_state = State::Moving;
                break;
            }

            attackTimer_ += dt;
            if (attackTimer_ >= attackSpeed_) {
                attackTimer_ = 0.f;
                performAttack(*target_);
            }
            break;
        }
        case State::Casting: {
            m_state = State::Attacking;
            break;
        }
        case State::Dead:
            break;
    }
}

void AssassinUnit::performAttack(Unit& target) {
    if (!isAlive() || !target.isAlive()) return;

    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    // 闪避判定
    if (dist(rng) < target.getDodgeChance()) {
        return;
    }

    // 20%暴击率，暴击2倍伤害
    bool isCrit = dist(rng) < 0.2f;
    int damage = isCrit ? atk * 2 : atk;
    target.takeDamage(damage);
}

std::unique_ptr<Unit> AssassinUnit::clone() const {
    return std::make_unique<AssassinUnit>(position, starRank);
}