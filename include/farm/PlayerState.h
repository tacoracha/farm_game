#pragma once

#include "ItemCatalog.h"
#include "Types.h"
#include "Warehouse.h"

#include <unordered_set>

namespace farm {

// Resource flow (production gameplay code):
// - Change gold and warehouse stock through PlayerState (TrySpendGold, TryAddToWarehouse,
//   TryRemoveFromWarehouse, TrySellFromWarehouse, etc.). Subsystems take PlayerState& for mutations.
// - Warehouse is the low-level stacked inventory implementation; avoid bypassing PlayerState for
//   the player's economy/inventory in gameplay paths.
// - Unit tests may use Warehouse directly to isolate inventory math; that is a test convenience,
//   not the recommended integration path for player resources.

class PlayerState {
public:
    PlayerState();

    int Gold() const { return gold_; }
    const Warehouse& GetWarehouse() const { return warehouse_; }

    [[nodiscard]] Result<void> TryAddToWarehouse(ItemId id, int quantity);
    [[nodiscard]] Result<void> TryRemoveFromWarehouse(ItemId id, int quantity);

    bool HasItem(ItemId id, int minimum_count) const;
    int GetItemCount(ItemId id) const;

    [[nodiscard]] Result<void> TrySpendGold(int amount);
    void AddGold(int amount);

    [[nodiscard]] Result<void> TrySellFromWarehouse(ItemId id, int quantity);

    void SetItemProtected(ItemId id, bool protect);

    bool IsSeedUnlocked(ItemId seed_id) const;

private:
    int gold_ = 0;
    Warehouse warehouse_;
    std::unordered_set<ItemId> unlocked_seeds_;
};

}  // namespace farm
