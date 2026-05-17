#pragma once
#include "unit.h"
#include "unit_types.h"
#include <memory>
#include <string>
#include <random>
#include <array>

namespace ShopNS {

// 商店槽位数量
constexpr int SHOP_SLOT_COUNT = 5;

// 单位类型及其价格的定义
struct ShopUnitDef {
    std::string typeName;   // 类型名称（用于创建单位）
    int cost;               // 购买价格
};

// 获取所有可购买单位类型的定义
const std::array<ShopUnitDef, 5>& getShopUnitDefs();

// 单个商店槽位
struct ShopSlot {
    std::string unitTypeName;   // 单位类型名称（空字符串表示已售出或空）
    int cost = 0;               // 购买价格
    bool sold = false;          // 是否已售出

    // 检查槽位是否有可购买的单位
    bool isAvailable() const { return !unitTypeName.empty() && !sold; }
};

// 商店类：管理5个槽位，支持刷新和购买
class Shop {
public:
    Shop();

    // 刷新商店：随机填充5个单位槽位
    void refresh();

    // 购买指定槽位的单位，返回创建好的单位（需要外部扣金币）
    // 如果槽位无效或已售出，返回 nullptr
    std::unique_ptr<Unit> buyUnit(int slotIndex);

    // 查询接口
    const ShopSlot& getSlot(int slotIndex) const;
    bool isSlotAvailable(int slotIndex) const;
    int getSlotCost(int slotIndex) const;
    int getSlotCount() const { return SHOP_SLOT_COUNT; }

    // 检查商店是否所有槽位都已售出
    bool isAllSold() const;

    // 获取随机数生成器引用（用于外部设置种子等）
    void setSeed(unsigned int seed);

private:
    std::array<ShopSlot, SHOP_SLOT_COUNT> slots_;
    std::mt19937 rng_;
};

} // namespace ShopNS