#include "farm/inventory/PlayerState.h"

#include "farm/achievement/AchievementSystem.h"
#include "farm/core/UnlockGraph.h"

#include <algorithm>

namespace farm {

PlayerState::PlayerState() {
    items_[ItemId::WheatSeed] = kInitialWheatSeeds;
    if (kInitialCornSeeds > 0) {
        items_[ItemId::CornSeed] = kInitialCornSeeds;
    }
    if (kInitialCarrotSeeds > 0) {
        items_[ItemId::CarrotSeed] = kInitialCarrotSeeds;
    }
    items_[ItemId::Fertilizer] = kInitialFertilizer;
    locked_items_.insert(ItemId::WheatSeed);
    unlocked_seeds_.insert(ItemId::WheatSeed);
    unlocked_content_.insert(UnlockId::WheatSeed);
    unlocked_content_.insert(UnlockId::ChickenCoop);
    unlocked_content_.insert(UnlockId::Chicken);
}

void PlayerState::Setup(AchievementSystem* ach) {
    ach_ = ach;
}

int PlayerState::WarehouseUsed() const {
    int used = 0;
    for (const auto& [item, quantity] : items_) {
        (void)item;
        used += quantity;
    }
    return used;
}

bool PlayerState::HasItem(ItemId item, int quantity) const {
    return quantity <= 0 || ItemCount(item) >= quantity;
}

int PlayerState::ItemCount(ItemId item) const {
    const auto it = items_.find(item);
    return it == items_.end() ? 0 : it->second;
}

bool PlayerState::IsItemLocked(ItemId item) const {
    return locked_items_.find(item) != locked_items_.end();
}

bool PlayerState::IsSeedUnlocked(ItemId seed) const {
    return unlocked_seeds_.find(seed) != unlocked_seeds_.end() ||
           IsUnlocked(UnlockGraph::SeedUnlock(seed));
}

bool PlayerState::IsUnlocked(UnlockId id) const {
    return unlocked_content_.find(id) != unlocked_content_.end();
}

bool PlayerState::CanUnlock(UnlockId id) const {
    const UnlockNode* node = UnlockGraph::Find(id);
    if (node == nullptr || IsUnlocked(id) || level_ < node->required_level) {
        return false;
    }
    for (UnlockId prerequisite : node->prerequisites) {
        if (!IsUnlocked(prerequisite)) {
            return false;
        }
    }
    return true;
}

std::vector<InventoryItemView> PlayerState::InventoryView() const {
    std::vector<InventoryItemView> view;
    for (ItemId item : AllItems()) {
        const int quantity = ItemCount(item);
        if (quantity > 0) {
            view.push_back(InventoryItemView{item, quantity, IsItemLocked(item)});
        }
    }
    return view;
}

Result<void> PlayerState::TryAddItem(ItemId item, int quantity) {
    if (quantity <= 0) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    if (WarehouseUsed() + quantity > warehouse_capacity_) {
        return Result<void>::failure(ErrorCode::WarehouseFull);
    }
    items_[item] = ItemCount(item) + quantity;
    return Result<void>::success();
}

Result<void> PlayerState::TryRemoveItem(ItemId item, int quantity) {
    if (quantity <= 0) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    if (ItemCount(item) < quantity) {
        return Result<void>::failure(ErrorCode::InsufficientItem);
    }
    const int remaining = ItemCount(item) - quantity;
    if (remaining == 0) {
        items_.erase(item);
    } else {
        items_[item] = remaining;
    }
    return Result<void>::success();
}

Result<void> PlayerState::TryRemoveItems(const std::vector<ItemStack>& items) {
    for (const ItemStack& stack : items) {
        if (!HasItem(stack.item, stack.quantity)) {
            return Result<void>::failure(ErrorCode::InsufficientItem);
        }
        if (IsItemLocked(stack.item)) {
            return Result<void>::failure(ErrorCode::ProtectedItem);
        }
    }
    for (const ItemStack& stack : items) {
        auto removed = TryRemoveItem(stack.item, stack.quantity);
        if (!removed.ok()) {
            return removed;
        }
    }
    return Result<void>::success();
}

Result<void> PlayerState::TrySpendGold(int amount) {
    if (amount < 0) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    if (gold_ < amount) {
        return Result<void>::failure(ErrorCode::InsufficientGold);
    }
    gold_ -= amount;
    return Result<void>::success();
}

Result<void> PlayerState::TrySellItem(ItemId item, int quantity) {
    if (IsItemLocked(item)) {
        return Result<void>::failure(ErrorCode::ProtectedItem);
    }
    const int price = GetItemInfo(item).sell_price;
    if (price <= 0) {
        return Result<void>::failure(ErrorCode::CannotSell);
    }
    auto removed = TryRemoveItem(item, quantity);
    if (!removed.ok()) {
        return removed;
    }
    AddGold(price * quantity);
    return Result<void>::success();
}

int PlayerState::BatchSellAll() {
    int total_gold = 0;
    int count = 0;
    std::vector<InventoryItemView> items = InventoryView();
    for (const InventoryItemView& iv : items) {
        if (iv.locked) continue;
        const ItemInfo& info = GetItemInfo(iv.item);
        if (info.sell_price <= 0) continue;
        total_gold += info.sell_price * iv.quantity;
        count += iv.quantity;
        items_.erase(iv.item);
    }
    if (total_gold > 0) AddGold(total_gold);
    return count;
}

Result<void> PlayerState::UpgradeWarehouse() {
    if (warehouse_level_ >= kWarehouseMaxLevel) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    int cost = kWarehouseUpgradeCosts[warehouse_level_];
    auto spent = TrySpendGold(cost);
    if (!spent.ok()) {
        return spent;
    }
    ++warehouse_level_;
    warehouse_capacity_ = kWarehouseCapacities[warehouse_level_ - 1];
    return Result<void>::success();
}

Result<void> PlayerState::UnlockContent(UnlockId id) {
    const UnlockNode* node = UnlockGraph::Find(id);
    if (node == nullptr) {
        return Result<void>::failure(ErrorCode::InvalidItem);
    }
    if (IsUnlocked(id)) {
        return Result<void>::success();
    }
    if (!CanUnlock(id)) {
        return Result<void>::failure(ErrorCode::ContentLocked);
    }
    auto spent = TrySpendGold(node->gold_cost);
    if (!spent.ok()) {
        return spent;
    }
    unlocked_content_.insert(id);
    if (node->category == UnlockCategory::Seed) {
        switch (id) {
            case UnlockId::WheatSeed:      unlocked_seeds_.insert(ItemId::WheatSeed);      break;
            case UnlockId::CornSeed:       unlocked_seeds_.insert(ItemId::CornSeed);       break;
            case UnlockId::CarrotSeed:     unlocked_seeds_.insert(ItemId::CarrotSeed);     break;
            case UnlockId::TomatoSeed:     unlocked_seeds_.insert(ItemId::TomatoSeed);     break;
            case UnlockId::StrawberrySeed: unlocked_seeds_.insert(ItemId::StrawberrySeed); break;
            case UnlockId::PumpkinSeed:    unlocked_seeds_.insert(ItemId::PumpkinSeed);    break;
            case UnlockId::MushroomSeed:   unlocked_seeds_.insert(ItemId::MushroomSeed);   break;
            default: break;
        }
    }
    if (ach_) ach_->OnUnlockContent(id);
    return Result<void>::success();
}

void PlayerState::AddGold(int amount) {
    if (amount > 0) {
        gold_ += amount;
        if (ach_) ach_->OnAddGold(amount);
    }
}

void PlayerState::AddExperience(int amount) {
    if (amount <= 0) {
        return;
    }
    experience_ += amount;
    while (experience_ >= kExpPerLevel) {
        experience_ -= kExpPerLevel;
        ++level_;
        if (ach_) ach_->OnPlayerLevelUp(level_);
    }
}

void PlayerState::SetItemLocked(ItemId item, bool locked) {
    if (locked) {
        locked_items_.insert(item);
    } else {
        locked_items_.erase(item);
    }
}

void PlayerState::ClearForLoad() {
    gold_ = 0;
    level_ = 1;
    experience_ = 0;
    warehouse_level_ = 1;
    warehouse_capacity_ = kInitialWarehouseCapacity;
    items_.clear();
    locked_items_.clear();
    unlocked_seeds_.clear();
    unlocked_content_.clear();
}

void PlayerState::SetItemForLoad(ItemId item, int quantity) {
    if (quantity > 0) {
        items_[item] = quantity;
    } else {
        items_.erase(item);
    }
}

void PlayerState::SetSeedUnlockedForLoad(ItemId seed, bool unlocked) {
    if (unlocked) {
        unlocked_seeds_.insert(seed);
    } else {
        unlocked_seeds_.erase(seed);
    }
}

void PlayerState::SetUnlockedForLoad(UnlockId id, bool unlocked) {
    if (unlocked) {
        unlocked_content_.insert(id);
        const UnlockNode* node = UnlockGraph::Find(id);
        if (node != nullptr && node->category == UnlockCategory::Seed) {
            switch (id) {
                case UnlockId::WheatSeed:      unlocked_seeds_.insert(ItemId::WheatSeed);      break;
                case UnlockId::CornSeed:       unlocked_seeds_.insert(ItemId::CornSeed);       break;
                case UnlockId::CarrotSeed:     unlocked_seeds_.insert(ItemId::CarrotSeed);     break;
                case UnlockId::TomatoSeed:     unlocked_seeds_.insert(ItemId::TomatoSeed);     break;
                case UnlockId::StrawberrySeed: unlocked_seeds_.insert(ItemId::StrawberrySeed); break;
                case UnlockId::PumpkinSeed:    unlocked_seeds_.insert(ItemId::PumpkinSeed);    break;
                case UnlockId::MushroomSeed:   unlocked_seeds_.insert(ItemId::MushroomSeed);   break;
                default: break;
            }
        }
    } else {
        unlocked_content_.erase(id);
    }
}

}  // namespace farm
