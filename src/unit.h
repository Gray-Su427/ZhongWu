#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <unordered_set>

class Unit {
public:
    enum class Owner {
        PlayerCtrl,
        EnemyCtrl,
        Neutral
    };

    Unit(sf::Vector2f pos , int starRank = 1);

    virtual ~Unit() = default;

    // --- 属性接口 ---
    void setPosition(const sf::Vector2f& p);
    sf::Vector2f getPosition() const;

    void setSize(const sf::Vector2f& s);
    sf::Vector2f getSize() const;

    void setVelocity(const sf::Vector2f& v);
    sf::Vector2f getVelocity() const;

    // 战斗属性
    void setHP(int hp);
    int getHP() const;
    int getMaxHP() const;
    void setMaxHP(int m);

    void setATK(int atk);
    int getATK() const;

    void setRange(float r);
    float getRange() const;

    void setMinRange(float r);
    float getMinRange() const;

    void setMaxMana(int m);
    int getMaxMana() const;
    void setMana(int m);
    int getMana() const;

    bool isAlive() const;

    Owner owner() const;
    void setOwner(Owner o);

    // --- 拖拽接口（准备/布阵阶段） ---
    void startDrag(const sf::Vector2f& mousePos); // 记录原位并开启拖拽
    void dragTo(const sf::Vector2f& mousePos);    // 按鼠标位置移动（实时）
    // 结束拖拽：若 commit==true 则保持当前位置，否则回弹到原位。返回最终是否放置成功。
    bool endDrag(bool commit);

    bool isDragging() const;

    // 判断点是否在单位内（用于鼠标选中/拖拽）
    bool containsPoint(const sf::Vector2f& point) const;

    // 核心接口(纯虚函数)
    virtual void update(float dt) = 0;
    virtual std::unique_ptr<Unit> clone() const = 0;

    // 默认绘制实现（可被覆盖）
    virtual void render(sf::RenderWindow& window) const;

    // 攻击与受伤（可覆盖）
    virtual void attack(Unit& target); // 默认检查距离并扣血
    virtual void takeDamage(int dmg);
    virtual void heal(int amount);

    // 辅助：获取包围盒（用于简单碰撞检测）
    virtual sf::FloatRect getBounds() const;

    // 碰撞回调（可选覆盖）
    virtual void onCollision(Unit& other);

protected:
    // 内部属性
    sf::Vector2f position;
    sf::Vector2f size;
    sf::Vector2f velocity;
    sf::Color color = sf::Color::White;
    // 星阶，默认为 1
    int starRank = 1;

    // 战斗属性
    int hp = 100;
    int maxHp = 100;
    int atk = 10;
    float max_range = 48.f;
    float min_range = 0.f;
    int maxMana = 0;
    int mana = 0;

    Owner m_owner = Owner::Neutral;
    std::unordered_set<std::string> m_traits;

    // 默认渲染使用一个矩形 shape
    sf::RectangleShape shape;

    // 拖拽支持
    bool dragging = false;
    sf::Vector2f dragOffset{0.f, 0.f};
    sf::Vector2f originalPosition{0.f, 0.f};

    // 同步 shape 到 position/size/color
    void syncShape();
};