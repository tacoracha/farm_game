#pragma once

#include "farm/common/Types.h"

#include <vector>

namespace farm {

class PlayerState;

struct ShopItemView {
    ItemId item = ItemId::WheatSeed;
    int unit_price = 0;
    bool unlocked = true;
};

class ShopSystem {
public:
    std::vector<ShopItemView> Items(const PlayerState& player) const;
    Result<void> BuyItem(PlayerState& player, ItemId item, int quantity) const;
};

}  // namespace farm

