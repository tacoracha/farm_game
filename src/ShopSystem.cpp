#include "farm/ShopSystem.h"

#include "farm/ItemCatalog.h"
#include "farm/PlayerState.h"

namespace farm {

bool ShopSystem::IsPurchasableSeed(ItemId id) {
    return ItemCatalog::IsSeed(id) && ItemCatalog::SeedShopPrice(id) > 0;
}

int ShopSystem::GetSeedPrice(ItemId id) {
    if (!ItemCatalog::IsSeed(id)) {
        return 0;
    }
    return ItemCatalog::SeedShopPrice(id);
}

bool ShopSystem::IsUnlockedSeed(const PlayerState& player, ItemId seed_id) {
    return player.IsSeedUnlocked(seed_id);
}

Result<void> ShopSystem::BuySeed(PlayerState& player, ItemId seed_id, int quantity) {
    if (quantity <= 0) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    if (!IsPurchasableSeed(seed_id)) {
        return Result<void>::failure(ErrorCode::NotASeed);
    }
    if (!IsUnlockedSeed(player, seed_id)) {
        return Result<void>::failure(ErrorCode::SeedNotUnlocked);
    }

    const int unit_price = GetSeedPrice(seed_id);
    if (unit_price <= 0) {
        return Result<void>::failure(ErrorCode::InvalidItem);
    }

    const long long total_ll = static_cast<long long>(unit_price) * quantity;
    if (total_ll > static_cast<long long>(2147483647)) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    const int total_cost = static_cast<int>(total_ll);

    if (player.Gold() < total_cost) {
        return Result<void>::failure(ErrorCode::InsufficientGold);
    }

    const int used = player.GetWarehouse().UsedSlots();
    const int cap = player.GetWarehouse().MaxCapacity();
    if (used > cap || quantity > cap - used) {
        return Result<void>::failure(ErrorCode::WarehouseFull);
    }

    auto spent = player.TrySpendGold(total_cost);
    if (!spent.ok()) {
        return Result<void>::failure(spent.code);
    }

    auto added = player.TryAddToWarehouse(seed_id, quantity);
    if (!added.ok()) {
        player.AddGold(total_cost);
        return Result<void>::failure(added.code);
    }

    return Result<void>::success();
}

}  // namespace farm
