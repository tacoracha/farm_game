#include "farm/shop/ShopSystem.h"

#include "farm/common/Constants.h"
#include "farm/inventory/PlayerState.h"

namespace farm {

std::vector<ShopItemView> ShopSystem::Items(const PlayerState& player) const {
    return {
        {ItemId::WheatSeed, GetItemInfo(ItemId::WheatSeed).buy_price,
         player.IsSeedUnlocked(ItemId::WheatSeed)},
        {ItemId::CornSeed, GetItemInfo(ItemId::CornSeed).buy_price,
         player.IsSeedUnlocked(ItemId::CornSeed)},
        {ItemId::CarrotSeed, GetItemInfo(ItemId::CarrotSeed).buy_price,
         player.IsSeedUnlocked(ItemId::CarrotSeed)},
        {ItemId::Fertilizer, kFertilizerCost, true},
    };
}

Result<void> ShopSystem::BuyItem(PlayerState& player, ItemId item, int quantity) const {
    if (quantity <= 0) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    const ItemInfo& info = GetItemInfo(item);
    if (info.buy_price <= 0) {
        return Result<void>::failure(IsSeed(item) ? ErrorCode::SeedNotUnlocked
                                                  : ErrorCode::InvalidItem);
    }
    if (IsSeed(item) && !player.IsSeedUnlocked(item)) {
        return Result<void>::failure(ErrorCode::SeedNotUnlocked);
    }
    const int cost = info.buy_price * quantity;
    if (player.Gold() < cost) {
        return Result<void>::failure(ErrorCode::InsufficientGold);
    }
    if (player.WarehouseRemaining() < quantity) {
        return Result<void>::failure(ErrorCode::WarehouseFull);
    }
    auto spent = player.TrySpendGold(cost);
    if (!spent.ok()) {
        return spent;
    }
    auto added = player.TryAddItem(item, quantity);
    if (!added.ok()) {
        player.AddGold(cost);
        return added;
    }
    return Result<void>::success();
}

}  // namespace farm

