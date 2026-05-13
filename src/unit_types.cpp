#include "unit_types.h"

// ==================== 战士 ====================
// 构造函数：初始化战士属性（中等血量300、攻击25、近战范围48）
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
    color = sf::Color(200, 60, 60);   // 红色
    addTrait("战士");
    addTrait("近战");
    syncShape();
}

// 每帧更新：检查存活状态并更新FSM（阶段一简化实现）
void WarriorUnit::update(float dt) {
    (void)dt;
    // 阶段一：简单FSM占位
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;
    m_state = State::Idle;
}

// 克隆当前战士单位
std::unique_ptr<Unit> WarriorUnit::clone() const {
    return std::make_unique<WarriorUnit>(position, starRank);
}

// ==================== 弓箭手 ====================
// 构造函数：初始化弓箭手属性（低血量200、高攻击30、远程160、最小射程48）
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
    color = sf::Color(60, 180, 60);   // 绿色
    addTrait("弓箭手");
    addTrait("远程");
    syncShape();
}

// 每帧更新：检查存活状态并更新FSM
void ArcherUnit::update(float dt) {
    (void)dt;
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;
    m_state = State::Idle;
}

// 克隆当前弓箭手单位
std::unique_ptr<Unit> ArcherUnit::clone() const {
    return std::make_unique<ArcherUnit>(position, starRank);
}

// ==================== 法师 ====================
// 构造函数：初始化法师属性（低血量180、攻击20、远程140、魔法值60）
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
    color = sf::Color(100, 100, 220); // 蓝色
    addTrait("法师");
    addTrait("远程");
    syncShape();
}

// 每帧更新：检查存活状态并更新FSM
void MageUnit::update(float dt) {
    (void)dt;
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;
    m_state = State::Idle;
}

// 克隆当前法师单位
std::unique_ptr<Unit> MageUnit::clone() const {
    return std::make_unique<MageUnit>(position, starRank);
}

// ==================== 坦克 ====================
// 构造函数：初始化坦克属性（高血量500、低攻击15、近战范围48）
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
    color = sf::Color(180, 180, 60);  // 黄色
    addTrait("坦克");
    addTrait("近战");
    syncShape();
}

// 每帧更新：检查存活状态并更新FSM
void TankUnit::update(float dt) {
    (void)dt;
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;
    m_state = State::Idle;
}

// 克隆当前坦克单位
std::unique_ptr<Unit> TankUnit::clone() const {
    return std::make_unique<TankUnit>(position, starRank);
}

// ==================== 刺客 ====================
// 构造函数：初始化刺客属性（中等血量220、极高攻击40、近战范围48）
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
    color = sf::Color(180, 60, 180);  // 紫色
    addTrait("刺客");
    addTrait("近战");
    syncShape();
}

// 每帧更新：检查存活状态并更新FSM
void AssassinUnit::update(float dt) {
    (void)dt;
    if (!isAlive()) { m_state = State::Dead; return; }
    if (m_state == State::Dead) return;
    m_state = State::Idle;
}

// 克隆当前刺客单位
std::unique_ptr<Unit> AssassinUnit::clone() const {
    return std::make_unique<AssassinUnit>(position, starRank);
}