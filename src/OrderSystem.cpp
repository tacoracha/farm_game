#include "farm/OrderSystem.h"

#include "farm/ItemCatalog.h"
#include "farm/PlayerState.h"

#include <ostream>

namespace farm {

namespace {

void OrderItemQuantityBounds(ItemId item, int& out_min, int& out_max) {
    switch (item) {
        case ItemId::Wheat:
            out_min = kOrderWheatQtyMin;
            out_max = kOrderWheatQtyMax;
            return;
        case ItemId::Corn:
            out_min = kOrderCornQtyMin;
            out_max = kOrderCornQtyMax;
            return;
        case ItemId::Carrot:
            out_min = kOrderCarrotQtyMin;
            out_max = kOrderCarrotQtyMax;
            return;
        case ItemId::Egg:
            out_min = kOrderEggQtyMin;
            out_max = kOrderEggQtyMax;
            return;
        default:
            out_min = 1;
            out_max = 1;
            return;
    }
}

}  // namespace

bool OrderSystem::IsValidOrderQuantity(ItemId item, int quantity) {
    int min_q = 0;
    int max_q = 0;
    OrderItemQuantityBounds(item, min_q, max_q);
    return quantity >= min_q && quantity <= max_q;
}

OrderSystem::OrderSystem() { InitializeDefaults(); }

void OrderSystem::InitializeDefaults() {
    sequence_ = 0;
    next_order_id_ = 1;
    slots_.assign(static_cast<std::size_t>(kOrderBoardSlotCount), Order{});
    for (int i = 0; i < kOrderBoardSlotCount; ++i) {
        RefreshOrder(i);
    }
}

int OrderSystem::ComputeRewardGold(ItemId item_id, int quantity) {
    const int unit = ItemCatalog::SellPrice(item_id);
    const long long numer =
        static_cast<long long>(unit) * quantity * kOrderRewardBonusNumerator;
    return static_cast<int>(numer / kOrderRewardBonusDenominator);
}

void OrderSystem::RefreshOrder(int slot_index) {
    if (slot_index < 0 || slot_index >= kOrderBoardSlotCount) {
        return;
    }

    ++sequence_;
    const int mix = sequence_ + slot_index * 17;
    const int pool_idx = ((mix % kOrderItemPoolCount) + kOrderItemPoolCount) % kOrderItemPoolCount;
    static const ItemId kOrderPool[] = {
        ItemId::Wheat, ItemId::Corn, ItemId::Carrot, ItemId::Egg};
    const ItemId item = kOrderPool[pool_idx];

    int min_q = 0;
    int max_q = 0;
    OrderItemQuantityBounds(item, min_q, max_q);
    const int span = max_q - min_q + 1;
    const int qty = min_q + ((mix / 3) % span);

    Order o;
    o.id = next_order_id_++;
    o.item_id = item;
    o.quantity = qty;
    o.reward_gold = ComputeRewardGold(item, qty);

    slots_[static_cast<std::size_t>(slot_index)] = o;
}

Result<void> OrderSystem::CompleteOrder(PlayerState& player, int slot_index) {
    if (slot_index < 0 || slot_index >= kOrderBoardSlotCount) {
        return Result<void>::failure(ErrorCode::OrderSlotOutOfRange);
    }

    const Order ord = slots_[static_cast<std::size_t>(slot_index)];

    auto removed = player.TryRemoveFromWarehouse(ord.item_id, ord.quantity);
    if (!removed.ok()) {
        return Result<void>::failure(removed.code);
    }

    player.AddGold(ord.reward_gold);
    RefreshOrder(slot_index);
    return Result<void>::success();
}

void OrderSystem::Tick(int /*current_tick*/) {}

void OrderSystem::WriteBoardForDisplay(std::ostream& os) const {
    os << "Orders (" << static_cast<int>(slots_.size()) << " slots):\n";
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        const Order& o = slots_[i];
        os << "  [" << i << "] id=" << o.id << " item=" << ItemCatalog::Get(o.item_id).name
           << " x" << o.quantity << " reward_gold=" << o.reward_gold << "\n";
    }
}

}  // namespace farm
