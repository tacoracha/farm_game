#include "farm/inventory/PlayerState.h"

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

Result<void> PlayerState::UpgradeWarehouse() {
    auto spent = TrySpendGold(kWarehouseUpgradeCost);
    if (!spent.ok()) {
        return spent;
    }
    warehouse_capacity_ += kWarehouseUpgradeSlots;
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
            case UnlockId::WheatSeed:
                unlocked_seeds_.insert(ItemId::WheatSeed);
                break;
            case UnlockId::CornSeed:
                unlocked_seeds_.insert(ItemId::CornSeed);
                break;
            case UnlockId::CarrotSeed:
                unlocked_seeds_.insert(ItemId::CarrotSeed);
                break;
            case UnlockId::TomatoSeed:
                unlocked_seeds_.insert(ItemId::TomatoSeed);
                break;
            default:
                break;
        }
    }
    return Result<void>::success();
}

void PlayerState::AddGold(int amount) {
    if (amount > 0) {
        gold_ += amount;
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
                case UnlockId::WheatSeed:
                    unlocked_seeds_.insert(ItemId::WheatSeed);
                    break;
                case UnlockId::CornSeed:
                    unlocked_seeds_.insert(ItemId::CornSeed);
                    break;
                case UnlockId::CarrotSeed:
                    unlocked_seeds_.insert(ItemId::CarrotSeed);
                    break;
                case UnlockId::TomatoSeed:
                    unlocked_seeds_.insert(ItemId::TomatoSeed);
                    break;
                default:
                    break;
            }
        }
    } else {
        unlocked_content_.erase(id);
    }
}

}  // namespace farm
