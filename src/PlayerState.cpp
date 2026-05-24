#include "farm/PlayerState.h"

namespace farm {

PlayerState::PlayerState()
    : gold_(kInitialGold), warehouse_(kWarehouseCapacity) {
    (void)warehouse_.TryAdd(ItemId::WheatSeed, kInitialWheatSeedCount);
    (void)warehouse_.TryAdd(ItemId::CornSeed, kInitialCornSeedCount);
    (void)warehouse_.TryAdd(ItemId::CarrotSeed, kInitialCarrotSeedCount);
    if (kInitialFertilizerCount > 0) {
        (void)warehouse_.TryAdd(ItemId::Fertilizer, kInitialFertilizerCount);
    }

    warehouse_.SetItemProtected(ItemId::WheatSeed, true);
    warehouse_.SetItemProtected(ItemId::CornSeed, true);
    warehouse_.SetItemProtected(ItemId::CarrotSeed, true);

    unlocked_seeds_.insert(ItemId::WheatSeed);
    unlocked_seeds_.insert(ItemId::CornSeed);
    unlocked_seeds_.insert(ItemId::CarrotSeed);
}

Result<void> PlayerState::TryAddToWarehouse(ItemId id, int quantity) {
    return warehouse_.TryAdd(id, quantity);
}

Result<void> PlayerState::TryRemoveFromWarehouse(ItemId id, int quantity) {
    return warehouse_.TryRemove(id, quantity);
}

bool PlayerState::HasItem(ItemId id, int minimum_count) const {
    return warehouse_.HasItem(id, minimum_count);
}

int PlayerState::GetItemCount(ItemId id) const { return warehouse_.GetCount(id); }

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

void PlayerState::AddGold(int amount) {
    if (amount > 0) {
        gold_ += amount;
    }
}

Result<void> PlayerState::TrySellFromWarehouse(ItemId id, int quantity) {
    if (warehouse_.IsItemProtected(id)) {
        return Result<void>::failure(ErrorCode::ProtectedItem);
    }
    const int unit_price = ItemCatalog::SellPrice(id);
    auto sold = warehouse_.TrySell(id, quantity, unit_price);
    if (!sold.ok()) {
        return Result<void>::failure(sold.code);
    }
    AddGold(sold.value);
    return Result<void>::success();
}

void PlayerState::SetItemProtected(ItemId id, bool protect) {
    warehouse_.SetItemProtected(id, protect);
}

bool PlayerState::IsSeedUnlocked(ItemId seed_id) const {
    return unlocked_seeds_.find(seed_id) != unlocked_seeds_.end();
}

}  // namespace farm
