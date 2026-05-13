#include "unit.h"
#include <cmath>

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
    setHP(hp - dmg);
}

void Unit::heal(int amount) {
    if (!isAlive()) return;
    setHP(hp + amount);
}
