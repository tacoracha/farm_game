#pragma once

#include "Types.h"

#include <unordered_map>
#include <unordered_set>

namespace farm {

// Low-level stacked inventory (capacity, stacks, protected-for-sell flags).
// Player-facing mutations should go through PlayerState in gameplay code; tests may use Warehouse
// directly for focused inventory checks.

class Warehouse {
public:
    explicit Warehouse(int max_capacity);

    int MaxCapacity() const { return max_capacity_; }
    int UsedSlots() const { return used_slots_; }

    [[nodiscard]] Result<void> TryAdd(ItemId id, int quantity);
    [[nodiscard]] Result<void> TryRemove(ItemId id, int quantity);

    bool HasItem(ItemId id, int minimum_count) const;
    int GetCount(ItemId id) const;

    void SetItemProtected(ItemId id, bool protect);
    bool IsItemProtected(ItemId id) const;

    // Removes items and returns total gold (quantity * unit_price). Does not modify gold.
    [[nodiscard]] Result<int> TrySell(ItemId id, int quantity, int unit_price);

private:
    int max_capacity_;
    int used_slots_ = 0;
    std::unordered_map<ItemId, int> stacks_;
    std::unordered_set<ItemId> protected_;
};

}  // namespace farm
