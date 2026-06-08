#include "farm/core/UnlockGraph.h"

namespace farm {

const std::vector<UnlockNode>& UnlockGraph::Nodes() {
    static const std::vector<UnlockNode> nodes = {
        {UnlockId::WheatSeed, UnlockCategory::Seed, "小麦种子", 1, 0, {}},
        {UnlockId::CornSeed, UnlockCategory::Seed, "玉米种子", 2, 35, {UnlockId::WheatSeed}},
        {UnlockId::CarrotSeed, UnlockCategory::Seed, "胡萝卜种子", 3, 45, {UnlockId::CornSeed}},
        {UnlockId::TomatoSeed, UnlockCategory::Seed, "番茄种子", 4, 65, {UnlockId::CarrotSeed}},
        {UnlockId::ExtraLand, UnlockCategory::Land, "扩建土地", 2, 0, {UnlockId::CornSeed}},
        {UnlockId::ChickenCoop, UnlockCategory::Ranch, "鸡圈", 1, 0, {}},
        {UnlockId::CowBarn, UnlockCategory::Ranch, "牛棚", 3, 90, {UnlockId::CarrotSeed, UnlockId::ChickenCoop}},
        {UnlockId::SheepPen, UnlockCategory::Ranch, "羊圈", 5, 130, {UnlockId::CowBarn}},
        {UnlockId::Chicken, UnlockCategory::Animal, "鸡", 1, 0, {UnlockId::ChickenCoop}},
        {UnlockId::Cow, UnlockCategory::Animal, "牛", 3, 60, {UnlockId::CowBarn}},
        {UnlockId::Sheep, UnlockCategory::Animal, "羊", 5, 75, {UnlockId::SheepPen}},
    };
    return nodes;
}

const UnlockNode* UnlockGraph::Find(UnlockId id) {
    for (const UnlockNode& node : Nodes()) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

UnlockId UnlockGraph::SeedUnlock(ItemId seed) {
    switch (seed) {
        case ItemId::WheatSeed:
            return UnlockId::WheatSeed;
        case ItemId::CornSeed:
            return UnlockId::CornSeed;
        case ItemId::CarrotSeed:
            return UnlockId::CarrotSeed;
        case ItemId::TomatoSeed:
            return UnlockId::TomatoSeed;
        default:
            return UnlockId::WheatSeed;
    }
}

UnlockId UnlockGraph::FacilityUnlock(RanchFacilityKind kind) {
    switch (kind) {
        case RanchFacilityKind::ChickenCoop:
            return UnlockId::ChickenCoop;
        case RanchFacilityKind::CowBarn:
            return UnlockId::CowBarn;
        case RanchFacilityKind::SheepPen:
            return UnlockId::SheepPen;
        case RanchFacilityKind::PigPen:
            return UnlockId::SheepPen;
    }
    return UnlockId::ChickenCoop;
}

UnlockId UnlockGraph::AnimalUnlock(AnimalKind kind) {
    switch (kind) {
        case AnimalKind::Chicken:
            return UnlockId::Chicken;
        case AnimalKind::Cow:
            return UnlockId::Cow;
        case AnimalKind::Sheep:
            return UnlockId::Sheep;
        case AnimalKind::Pig:
            return UnlockId::Sheep;
    }
    return UnlockId::Chicken;
}

bool UnlockGraph::IsSeed(ItemId item) {
    return item == ItemId::WheatSeed || item == ItemId::CornSeed ||
           item == ItemId::CarrotSeed || item == ItemId::TomatoSeed;
}

}  // namespace farm

