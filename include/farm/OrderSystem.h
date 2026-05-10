#pragma once

#include "Constants.h"
#include "Types.h"

#include <iosfwd>
#include <vector>

namespace farm {

class PlayerState;

struct Order {
    int id = 0;
    ItemId item_id = ItemId::Wheat;
    int quantity = 0;
    int reward_gold = 0;
};

// Fixed slots; single-item orders from pool (Wheat / Corn / Carrot / Egg); deterministic refresh.
class OrderSystem {
public:
    OrderSystem();

    const std::vector<Order>& GetOrders() const { return slots_; }

    void InitializeDefaults();

    void RefreshOrder(int slot_index);

    [[nodiscard]] Result<void> CompleteOrder(PlayerState& player, int slot_index);

    void Tick(int current_tick);

    void WriteBoardForDisplay(std::ostream& os) const;

    // Single source of truth for order quantity bounds (shared with RefreshOrder).
    [[nodiscard]] static bool IsValidOrderQuantity(ItemId item, int quantity);

private:
    std::vector<Order> slots_;
    int sequence_ = 0;
    int next_order_id_ = 1;

    static int ComputeRewardGold(ItemId item_id, int quantity);
};

}  // namespace farm
