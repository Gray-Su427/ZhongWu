#include "synergy.h"
#include "unit.h"
#include "board.h"
#include <algorithm>
#include <sstream>

namespace SynergyNS {

SynergyManager::SynergyManager() {
    // ===== 5 种羁绊定义（数据驱动） =====

    // 战士：减伤
    synergyDefs_.push_back(SynergyDef{
        "战士", "战士羁绊",
        {
            SynergyLevel{2, { SynergyBuff{BuffType::DamageReduction, 0.10f, 0} }},
            SynergyLevel{4, { SynergyBuff{BuffType::DamageReduction, 0.20f, 0} }}
        }
    });

    // 弓箭手：ATK 加成
    synergyDefs_.push_back(SynergyDef{
        "弓箭手", "弓箭手羁绊",
        {
            SynergyLevel{2, { SynergyBuff{BuffType::ATKBonus, 0.f, 15} }},
            SynergyLevel{4, { SynergyBuff{BuffType::ATKBonus, 0.f, 35} }}
        }
    });

    // 法师：AOE 伤害倍率加成
    synergyDefs_.push_back(SynergyDef{
        "法师", "法师羁绊",
        {
            SynergyLevel{2, { SynergyBuff{BuffType::AOEMultiplier, 0.30f, 0} }},
            SynergyLevel{4, { SynergyBuff{BuffType::AOEMultiplier, 0.60f, 0} }}
        }
    });

    // 坦克：额外减伤
    synergyDefs_.push_back(SynergyDef{
        "坦克", "坦克羁绊",
        {
            SynergyLevel{2, { SynergyBuff{BuffType::DamageReduction, 0.15f, 0} }},
            SynergyLevel{4, { SynergyBuff{BuffType::DamageReduction, 0.30f, 0} }}
        }
    });

    // 刺客：暴击率加成
    synergyDefs_.push_back(SynergyDef{
        "刺客", "刺客羁绊",
        {
            SynergyLevel{2, { SynergyBuff{BuffType::CritBonus, 0.20f, 0} }},
            SynergyLevel{4, { SynergyBuff{BuffType::CritBonus, 0.40f, 0} }}
        }
    });

    // 近战：机制改变 — 击杀后重置攻击冷却
    synergyDefs_.push_back(SynergyDef{
        "近战", "近战羁绊",
        {
            SynergyLevel{3, { SynergyBuff{BuffType::KillResetAttack, 0.f, 1} }}
        }
    });
}

void SynergyManager::calculateSynergies(const BoardNS::Board& board) {
    // 清空旧数据
    activeSynergies_.clear();
    activeBuffsByTrait_.clear();

    // 统计场上玩家单位每个 trait 的数量
    std::map<std::string, int> traitCount;

    // 遍历棋盘上所有玩家单位
    auto playerUnits = board.getPlayerUnitsOnBoard();
    for (auto* unit : playerUnits) {
        if (!unit || !unit->isAlive()) continue;
        for (const auto& trait : unit->getTraits()) {
            traitCount[trait]++;
        }
    }

    // 对每个羁绊定义，检查是否激活
    for (const auto& def : synergyDefs_) {
        int count = 0;
        auto it = traitCount.find(def.traitName);
        if (it != traitCount.end()) {
            count = it->second;
        }

        ActiveSynergy active;
        active.traitName = def.traitName;
        active.count = count;
        active.level = 0;
        active.threshold = 0;

        // 找到最高激活的层级（从高到低检查）
        std::vector<SynergyBuff> activeBuffs;
        for (int i = static_cast<int>(def.levels.size()) - 1; i >= 0; --i) {
            if (count >= def.levels[i].threshold) {
                active.level = i + 1;
                active.threshold = def.levels[i].threshold;
                activeBuffs = def.levels[i].buffs;
                break;
            }
        }

        // 生成 Buff 描述
        std::ostringstream desc;
        if (active.level > 0) {
            for (size_t b = 0; b < activeBuffs.size(); ++b) {
                if (b > 0) desc << ", ";
                switch (activeBuffs[b].type) {
                    case BuffType::DamageReduction:
                        desc << "减伤" << static_cast<int>(activeBuffs[b].value * 100) << "%";
                        break;
                    case BuffType::ATKBonus:
                        desc << "ATK+" << activeBuffs[b].intValue;
                        break;
                    case BuffType::AOEMultiplier:
                        desc << "AOE伤害+" << static_cast<int>(activeBuffs[b].value * 100) << "%";
                        break;
                    case BuffType::CritBonus:
                        desc << "暴击率+" << static_cast<int>(activeBuffs[b].value * 100) << "%";
                        break;
                    case BuffType::KillResetAttack:
                        desc << "击杀重置攻击冷却";
                        break;
                }
            }
        }
        active.buffDescription = desc.str();

        activeSynergies_.push_back(active);

        // 缓存激活的 Buff
        if (active.level > 0) {
            activeBuffsByTrait_[def.traitName] = activeBuffs;
        }
    }
}

void SynergyManager::clearSynergies() {
    activeSynergies_.clear();
    activeBuffsByTrait_.clear();
}

// --- 查询接口 ---

const std::vector<SynergyBuff>* SynergyManager::getBuffsForTrait(const std::string& traitName) const {
    auto it = activeBuffsByTrait_.find(traitName);
    if (it != activeBuffsByTrait_.end()) {
        return &it->second;
    }
    return nullptr;
}

int SynergyManager::getATKBonus(const Unit* unit) const {
    if (!unit) return 0;
    int total = 0;
    for (const auto& trait : unit->getTraits()) {
        auto* buffs = getBuffsForTrait(trait);
        if (buffs) {
            for (const auto& buff : *buffs) {
                if (buff.type == BuffType::ATKBonus) {
                    total += buff.intValue;
                }
            }
        }
    }
    return total;
}

float SynergyManager::getDamageReduction(const Unit* unit) const {
    if (!unit) return 0.f;
    float total = 0.f;
    for (const auto& trait : unit->getTraits()) {
        auto* buffs = getBuffsForTrait(trait);
        if (buffs) {
            for (const auto& buff : *buffs) {
                if (buff.type == BuffType::DamageReduction) {
                    total += buff.value;
                }
            }
        }
    }
    return total;
}

float SynergyManager::getCritBonus(const Unit* unit) const {
    if (!unit) return 0.f;
    float total = 0.f;
    for (const auto& trait : unit->getTraits()) {
        auto* buffs = getBuffsForTrait(trait);
        if (buffs) {
            for (const auto& buff : *buffs) {
                if (buff.type == BuffType::CritBonus) {
                    total += buff.value;
                }
            }
        }
    }
    return total;
}

float SynergyManager::getAOEMultiplier(const Unit* unit) const {
    if (!unit) return 0.f;
    float total = 0.f;
    for (const auto& trait : unit->getTraits()) {
        auto* buffs = getBuffsForTrait(trait);
        if (buffs) {
            for (const auto& buff : *buffs) {
                if (buff.type == BuffType::AOEMultiplier) {
                    total += buff.value;
                }
            }
        }
    }
    return total;
}

const std::vector<ActiveSynergy>& SynergyManager::getActiveSynergies() const {
    return activeSynergies_;
}

const std::vector<SynergyDef>& SynergyManager::getAllSynergyDefs() const {
    return synergyDefs_;
}

bool SynergyManager::hasKillResetAttack(const Unit* unit) const {
    if (!unit) return false;
    for (const auto& trait : unit->getTraits()) {
        auto* buffs = getBuffsForTrait(trait);
        if (buffs) {
            for (const auto& buff : *buffs) {
                if (buff.type == BuffType::KillResetAttack && buff.intValue > 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

} // namespace SynergyNS
