#pragma once

#include "Types.h"

namespace farm {

class PlayerState;

// Seed shop only (phase); prices come from ItemCatalog / Constants.
class ShopSystem {
public:
    [[nodiscard]] static bool IsPurchasableSeed(ItemId id);
    [[nodiscard]] static int GetSeedPrice(ItemId id);
    [[nodiscard]] static bool IsUnlockedSeed(const PlayerState& player, ItemId seed_id);

    [[nodiscard]] Result<void> BuySeed(PlayerState& player, ItemId seed_id, int quantity);
};

}  // namespace farm
