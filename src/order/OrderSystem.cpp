#include "farm/order/OrderSystem.h"

#include "farm/common/Constants.h"
#include "farm/inventory/PlayerState.h"

#include <array>

namespace farm {

OrderSystem::OrderSystem() {
    orders_.assign(static_cast<std::size_t>(kOrderSlotCount), OrderData{});
    for (int i = 0; i < kOrderSlotCount; ++i) {
        RefreshOrder(i);
    }
}

bool OrderSystem::CanDeliver(const PlayerState& player, int slot) const {
    if (slot < 0 || slot >= static_cast<int>(orders_.size())) {
        return false;
    }
    const OrderData& order = orders_[static_cast<std::size_t>(slot)];
    return order.state == OrderState::Available && !order.locked &&
           player.HasItem(order.item, order.quantity);
}

Result<void> OrderSystem::CompleteOrder(PlayerState& player, int slot, int current_tick) {
    if (slot < 0 || slot >= static_cast<int>(orders_.size())) {
        return Result<void>::failure(ErrorCode::OrderSlotOutOfRange);
    }
    OrderData& order = orders_[static_cast<std::size_t>(slot)];
    if (order.locked || order.state == OrderState::Locked) {
        return Result<void>::failure(ErrorCode::OrderLocked);
    }
    if (order.state == OrderState::CoolingDown) {
        return Result<void>::failure(ErrorCode::OrderCoolingDown);
    }
    if (player.IsItemLocked(order.item)) {
        return Result<void>::failure(ErrorCode::ProtectedItem);
    }
    auto removed = player.TryRemoveItem(order.item, order.quantity);
    if (!removed.ok()) {
        return removed;
    }
    player.AddGold(order.reward_gold);
    player.AddExperience(order.reward_exp);
    order.state = OrderState::CoolingDown;
    order.cooldown_until_tick = current_tick + kOrderCompleteCooldownTicks;
    return Result<void>::success();
}

Result<void> OrderSystem::AbandonOrder(int slot, int current_tick) {
    if (slot < 0 || slot >= static_cast<int>(orders_.size())) {
        return Result<void>::failure(ErrorCode::OrderSlotOutOfRange);
    }
    OrderData& order = orders_[static_cast<std::size_t>(slot)];
    if (order.locked) {
        return Result<void>::failure(ErrorCode::OrderLocked);
    }
    order.state = OrderState::CoolingDown;
    order.cooldown_until_tick = current_tick + kOrderAbandonCooldownTicks;
    return Result<void>::success();
}

Result<void> OrderSystem::SetLocked(int slot, bool locked) {
    if (slot < 0 || slot >= static_cast<int>(orders_.size())) {
        return Result<void>::failure(ErrorCode::OrderSlotOutOfRange);
    }
    OrderData& order = orders_[static_cast<std::size_t>(slot)];
    order.locked = locked;
    order.state = locked ? OrderState::Locked : OrderState::Available;
    return Result<void>::success();
}

void OrderSystem::Tick(int current_tick) {
    for (int i = 0; i < static_cast<int>(orders_.size()); ++i) {
        OrderData& order = orders_[static_cast<std::size_t>(i)];
        if (order.state == OrderState::CoolingDown && current_tick >= order.cooldown_until_tick) {
            RefreshOrder(i);
        }
    }
}

void OrderSystem::ClearForLoad() {
    orders_.clear();
    next_order_id_ = 1;
    sequence_ = 0;
}

void OrderSystem::SetForLoad(const std::vector<OrderData>& orders, int next_order_id,
                             int sequence) {
    orders_ = orders;
    next_order_id_ = next_order_id;
    sequence_ = sequence;
}

void OrderSystem::RefreshOrder(int slot) {
    static constexpr std::array<ItemId, 4> kPool = {
        ItemId::Wheat, ItemId::Corn, ItemId::Carrot, ItemId::Egg};
    ++sequence_;
    const int mix = sequence_ + slot * 11;
    const ItemId item = kPool[static_cast<std::size_t>(mix % static_cast<int>(kPool.size()))];
    const int quantity = 1 + (mix % (item == ItemId::Egg ? 2 : 3));
    OrderData order;
    order.id = next_order_id_++;
    order.item = item;
    order.quantity = quantity;
    order.reward_gold = RewardGold(item, quantity);
    order.reward_exp = 4 + quantity;
    orders_[static_cast<std::size_t>(slot)] = order;
}

int OrderSystem::RewardGold(ItemId item, int quantity) const {
    return GetItemInfo(item).sell_price * quantity * 3 / 2 + 2;
}

}  // namespace farm

