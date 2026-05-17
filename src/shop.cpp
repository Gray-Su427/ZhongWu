#include "shop.h"

namespace ShopNS {

// 单位类型定义：名称和价格
// 战士:2金, 弓箭手:2金, 法师:3金, 坦克:3金, 刺客:3金
static const std::array<ShopUnitDef, 5> SHOP_UNIT_DEFS = {{
    {"战士",   2},
    {"弓箭手", 2},
    {"法师",   3},
    {"坦克",   3},
    {"刺客",   3}
}};

const std::array<ShopUnitDef, 5>& getShopUnitDefs() {
    return SHOP_UNIT_DEFS;
}

Shop::Shop()
: rng_(std::random_device{}()) {
    // 初始化空槽位
    for (auto& slot : slots_) {
        slot.unitTypeName = "";
        slot.cost = 0;
        slot.sold = false;
    }
}

void Shop::refresh() {
    std::uniform_int_distribution<int> dist(0, static_cast<int>(SHOP_UNIT_DEFS.size()) - 1);

    for (int i = 0; i < SHOP_SLOT_COUNT; ++i) {
        int typeIndex = dist(rng_);
        slots_[i].unitTypeName = SHOP_UNIT_DEFS[typeIndex].typeName;
        slots_[i].cost = SHOP_UNIT_DEFS[typeIndex].cost;
        slots_[i].sold = false;
    }
}

std::unique_ptr<Unit> Shop::buyUnit(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= SHOP_SLOT_COUNT) return nullptr;
    if (!slots_[slotIndex].isAvailable()) return nullptr;

    // 创建单位
    auto unit = createUnitByName(slots_[slotIndex].unitTypeName);
    if (!unit) return nullptr;

    // 标记为已售出
    slots_[slotIndex].sold = true;
    return unit;
}

const ShopSlot& Shop::getSlot(int slotIndex) const {
    static ShopSlot emptySlot{"", 0, true};
    if (slotIndex < 0 || slotIndex >= SHOP_SLOT_COUNT) return emptySlot;
    return slots_[slotIndex];
}

bool Shop::isSlotAvailable(int slotIndex) const {
    if (slotIndex < 0 || slotIndex >= SHOP_SLOT_COUNT) return false;
    return slots_[slotIndex].isAvailable();
}

int Shop::getSlotCost(int slotIndex) const {
    if (slotIndex < 0 || slotIndex >= SHOP_SLOT_COUNT) return -1;
    return slots_[slotIndex].cost;
}

bool Shop::isAllSold() const {
    for (const auto& slot : slots_) {
        if (slot.isAvailable()) return false;
    }
    return true;
}

void Shop::setSeed(unsigned int seed) {
    rng_.seed(seed);
}

} // namespace ShopNS