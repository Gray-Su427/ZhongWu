#include "unit.h"
#include <cmath>
#include <random>
#include <algorithm>

Unit::Unit(sf::Vector2f pos , int starRank_)
: position(pos)
, size({64.f, 64.f})
, velocity({0.0f, 0.0f})
, m_owner(Owner::Neutral)
, starRank(starRank_) {
    syncShape();
}

void Unit::setPosition(const sf::Vector2f& p) {
    position = p;
    shape.setPosition(position);
}
sf::Vector2f Unit::getPosition() const { return position; }

void Unit::setSize(const sf::Vector2f& s) {
    size = s;
    shape.setSize(size);
    shape.setOrigin(size * 0.5f);
}
sf::Vector2f Unit::getSize() const { return size; }

void Unit::setVelocity(const sf::Vector2f& v) { velocity = v; }
sf::Vector2f Unit::getVelocity() const { return velocity; }

// HP/ATK/Range/Mana
void Unit::setHP(int h) { hp = std::max(0, std::min(h, maxHp)); }
int Unit::getHP() const { return hp; }
int Unit::getMaxHP() const { return maxHp; }
void Unit::setMaxHP(int m) { maxHp = std::max(1, m); if (hp > maxHp) hp = maxHp; }

void Unit::setATK(int a) { atk = a; }
int Unit::getATK() const { return atk; }

void Unit::setRange(float r) { max_range = r; }
float Unit::getRange() const { return max_range; }

void Unit::setMinRange(float r) { min_range = r; }
float Unit::getMinRange() const { return min_range; }

void Unit::setMaxMana(int m) { maxMana = m; if (mana > maxMana) mana = maxMana; }
int Unit::getMaxMana() const { return maxMana; }
void Unit::setMana(int m) { mana = std::max(0, std::min(m, maxMana)); }
int Unit::getMana() const { return mana; }

bool Unit::isAlive() const { return hp > 0; }

Unit::Owner Unit::owner() const { return m_owner; }
void Unit::setOwner(Owner o) { m_owner = o; }

// --- FSM 状态 ---
// 获取当前FSM状态
Unit::State Unit::getState() const { return m_state; }
// 设置FSM状态
void Unit::setState(State s) { m_state = s; }

// --- Traits（羁绊）---
// 添加一个羁绊标签
void Unit::addTrait(const std::string& trait) { m_traits.insert(trait); }
// 移除一个羁绊标签
void Unit::removeTrait(const std::string& trait) { m_traits.erase(trait); }
// 检查是否拥有指定羁绊
bool Unit::hasTrait(const std::string& trait) const { return m_traits.count(trait) > 0; }
// 获取所有羁绊标签的集合
const std::unordered_set<std::string>& Unit::getTraits() const { return m_traits; }

// --- 名称 ---
// 设置单位名称
void Unit::setUnitName(const std::string& name) { m_unitName = name; }
// 获取单位名称
const std::string& Unit::getUnitName() const { return m_unitName; }

// 拖拽
void Unit::startDrag(const sf::Vector2f& mousePos) {
    dragging = true;
    originalPosition = position;
    dragOffset = mousePos - position;
}
void Unit::dragTo(const sf::Vector2f& mousePos) {
    if (!dragging) return;
    position = mousePos - dragOffset;
    shape.setPosition(position);
}
bool Unit::endDrag(bool commit) {
    if (!dragging) return false;
    dragging = false;
    if (!commit) {
        // 回弹
        position = originalPosition;
        shape.setPosition(position);
        return false;
    }
    // commit 为 true，则保持当前位置
    return true;
}
bool Unit::isDragging() const { return dragging; }

bool Unit::containsPoint(const sf::Vector2f& point) const {
    return getBounds().contains(point);
}

// 渲染与碰撞
void Unit::syncShape() {
    shape.setSize(size);
    shape.setOrigin(size * 0.5f);
    shape.setPosition(position);
    shape.setFillColor(color);
}

void Unit::render(sf::RenderWindow& window) const {
    window.draw(shape);

    // 受击闪烁效果：在单位上方叠加半透明红色
    if (hitFlashTimer_ > 0.f) {
        sf::RectangleShape flashShape;
        flashShape.setSize(size);
        flashShape.setOrigin(size * 0.5f);
        flashShape.setPosition(position);
        // 透明度随时间衰减
        uint8_t alpha = static_cast<uint8_t>(std::min(hitFlashTimer_ / 0.15f, 1.0f) * 120.f);
        flashShape.setFillColor(sf::Color(255, 0, 0, alpha));
        window.draw(flashShape);
    }
}

sf::FloatRect Unit::getBounds() const {
    return shape.getGlobalBounds();
}

void Unit::onCollision(Unit& other) {
    (void)other;
}

// 默认攻击实现：检测距离并造成伤害
void Unit::attack(Unit& target) {
    if (!isAlive() || !target.isAlive()) return;
    // 计算中心距离
    sf::Vector2f diff = target.getPosition() - position;
    float dist2 = diff.x*diff.x + diff.y*diff.y;
    if (dist2 <= max_range*max_range && dist2 > min_range*min_range) {
        target.takeDamage(atk);
    }
}

void Unit::takeDamage(int dmg) {
    if (!isAlive()) return;
    // 触发受击闪烁
    triggerHitFlash();
    // 基类受伤：应用羁绊减伤（战士等使用此默认实现）
    float multiplier = 1.0f - synergyDamageReduction_;
    if (multiplier < 0.1f) multiplier = 0.1f;
    int reduced = static_cast<int>(dmg * multiplier);
    setHP(hp - reduced);
}

void Unit::heal(int amount) {
    if (!isAlive()) return;
    setHP(hp + amount);
}

// ==================== 战斗系统方法 ====================

// 重置战斗状态
void Unit::resetCombatState() {
    target_ = nullptr;
    attackTimer_ = 0.f;
    m_state = State::Idle;
    velocity = {0.f, 0.f};
}

// 清零所有羁绊Buff
void Unit::clearSynergyBuffs() {
    synergyATKBonus_ = 0;
    synergyCritBonus_ = 0.f;
    synergyDamageReduction_ = 0.f;
    synergyAOEMultiplier_ = 0.f;
    synergyKillReset_ = false;
}

// --- 受击闪烁效果 ---
void Unit::triggerHitFlash() {
    hitFlashTimer_ = 0.15f;  // 闪烁持续 0.15 秒
}

bool Unit::isHitFlashing() const {
    return hitFlashTimer_ > 0.f;
}

void Unit::updateHitFlash(float dt) {
    if (hitFlashTimer_ > 0.f) {
        hitFlashTimer_ -= dt;
        if (hitFlashTimer_ < 0.f) hitFlashTimer_ = 0.f;
    }
}

void Unit::setInCombat(bool v) { inCombat_ = v; }
bool Unit::isInCombat() const { return inCombat_; }

void Unit::setBoardBounds(const sf::FloatRect& bounds) { boardBounds_ = bounds; }
const sf::FloatRect& Unit::getBoardBounds() const { return boardBounds_; }

void Unit::setMoveSpeed(float s) { moveSpeed_ = s; }
float Unit::getMoveSpeed() const { return moveSpeed_; }

void Unit::setAttackSpeed(float s) { attackSpeed_ = s; }
float Unit::getAttackSpeed() const { return attackSpeed_; }

void Unit::setDodgeChance(float d) { dodgeChance_ = d; }
float Unit::getDodgeChance() const { return dodgeChance_; }

Unit* Unit::getTarget() const { return target_; }
void Unit::setTarget(Unit* t) { target_ = t; }

// 执行攻击（含闪避判定）
// atkBonus: 羁绊提供的额外攻击力, critBonus: 羁绊提供的额外暴击率
void Unit::performAttack(Unit& target, int atkBonus, float critBonus) {
    if (!isAlive() || !target.isAlive()) return;

    // 闪避判定
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    if (dist(rng) < target.getDodgeChance()) {
        // 闪避成功，不造成伤害
        return;
    }

    (void)critBonus; // 基类不使用暴击，由子类覆盖
    int totalAtk = atk + atkBonus;
    target.takeDamage(totalAtk);

    // 攻击后增加Mana（如果有MaxMana）
    if (maxMana > 0) {
        mana = std::min(mana + 10, maxMana);
    }
}

// 找最近的敌方单位
Unit* Unit::findClosestEnemy(const std::vector<Unit*>& enemies) const {
    Unit* closest = nullptr;
    float closestDist = std::numeric_limits<float>::max();
    for (auto* enemy : enemies) {
        if (!enemy || !enemy->isAlive()) continue;
        float dx = position.x - enemy->getPosition().x;
        float dy = position.y - enemy->getPosition().y;
        float dist = dx * dx + dy * dy;  // 平方距离，避免开方
        if (dist < closestDist) {
            closestDist = dist;
            closest = enemy;
        }
    }
    return closest;
}

// 约束位置在棋盘边界内
void Unit::clampToBoardBounds() {
    float halfW = size.x * 0.5f;
    float halfH = size.y * 0.5f;
    position.x = std::max(boardBounds_.position.x + halfW,
                          std::min(position.x, boardBounds_.position.x + boardBounds_.size.x - halfW));
    position.y = std::max(boardBounds_.position.y + halfH,
                          std::min(position.y, boardBounds_.position.y + boardBounds_.size.y - halfH));
    shape.setPosition(position);
}
