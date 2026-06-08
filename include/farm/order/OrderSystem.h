#pragma once

#include "farm/common/Types.h"

#include <vector>

namespace farm {

class PlayerState;

struct OrderData {
    int id = 0;
    OrderState state = OrderState::Available;
    ItemId item = ItemId::Wheat;
    int quantity = 1;
    int reward_gold = 0;
    int reward_exp = 0;
    int cooldown_until_tick = 0;
    bool locked = false;
};

class OrderSystem {
public:
    OrderSystem();

    const std::vector<OrderData>& Orders() const { return orders_; }
    bool CanDeliver(const PlayerState& player, int slot) const;
    Result<void> CompleteOrder(PlayerState& player, int slot, int current_tick);
    Result<void> AbandonOrder(int slot, int current_tick);
    Result<void> SetLocked(int slot, bool locked);
    void Tick(int current_tick);

    void ClearForLoad();
    void SetForLoad(const std::vector<OrderData>& orders, int next_order_id, int sequence);
    int NextOrderIdForSave() const { return next_order_id_; }
    int SequenceForSave() const { return sequence_; }

private:
    void RefreshOrder(int slot);
    int RewardGold(ItemId item, int quantity) const;

    std::vector<OrderData> orders_;
    int next_order_id_ = 1;
    int sequence_ = 0;
};

}  // namespace farm

