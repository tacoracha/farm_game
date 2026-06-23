#include "farm/order/OrderSystem.h"

#include "farm/achievement/AchievementSystem.h"
#include "farm/common/Constants.h"
#include "farm/dailytask/DailyTaskSystem.h"
#include "farm/core/UnlockGraph.h"
#include "farm/inventory/PlayerState.h"

#include <array>
#include <string>
#include <vector>

namespace farm {

OrderSystem::OrderSystem() {
    orders_.assign(static_cast<std::size_t>(kOrderSlotCount), OrderData{});
}

void OrderSystem::Setup(AchievementSystem* ach, DailyTaskSystem* dts) {
    ach_ = ach;
    dts_ = dts;
}

bool OrderSystem::CanDeliver(const PlayerState& player, int slot) const {
    if (slot < 0 || slot >= static_cast<int>(orders_.size())) {
        return false;
    }
    const OrderData& order = orders_[static_cast<std::size_t>(slot)];
    if (order.state != OrderState::Available || order.locked) {
        return false;
    }
    for (const OrderRequirement& req : order.requirements) {
        if (!player.HasItem(req.item, req.quantity)) {
            return false;
        }
    }
    return true;
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
    for (const OrderRequirement& req : order.requirements) {
        if (player.IsItemLocked(req.item)) {
            return Result<void>::failure(ErrorCode::ProtectedItem);
        }
    }
    for (const OrderRequirement& req : order.requirements) {
        if (!player.HasItem(req.item, req.quantity)) {
            return Result<void>::failure(ErrorCode::InsufficientItem);
        }
    }
    for (const OrderRequirement& req : order.requirements) {
        auto removed = player.TryRemoveItem(req.item, req.quantity);
        if (!removed.ok()) {
            return removed;
        }
    }
    player.AddGold(order.reward_gold);
    player.AddExperience(order.reward_exp);
    order.state = OrderState::CoolingDown;
    order.cooldown_until_tick = current_tick + kOrderCompleteCooldownTicks;
    if (ach_) ach_->OnCompleteOrder();
    if (dts_) dts_->OnCompleteOrder(1);
    return Result<void>::success();
}

Result<void> OrderSystem::AbandonOrder(int slot, int current_tick) {
    int count = AbandonOrders({slot}, current_tick);
    return count > 0 ? Result<void>::success() : Result<void>::failure(ErrorCode::OrderLocked);
}

int OrderSystem::AbandonOrders(const std::vector<int>& slots, int current_tick) {
    int count = 0;
    for (int slot : slots) {
        if (slot < 0 || slot >= static_cast<int>(orders_.size())) {
            continue;
        }
        OrderData& order = orders_[static_cast<std::size_t>(slot)];
        if (order.locked || order.state == OrderState::Locked) {
            continue;
        }
        order.state = OrderState::CoolingDown;
        order.cooldown_until_tick = current_tick + kOrderAbandonCooldownTicks;
        ++count;
    }
    return count;
}

Result<BatchCompleteResult> OrderSystem::BatchComplete(PlayerState& player, int current_tick) {
    BatchCompleteResult result;
    for (std::size_t i = 0; i < orders_.size(); ++i) {
        OrderData& order = orders_[i];
        if (order.state != OrderState::Available || order.locked) continue;
        bool can_deliver = true;
        for (const OrderRequirement& req : order.requirements) {
            if (!player.HasItem(req.item, req.quantity)) {
                can_deliver = false;
                break;
            }
        }
        if (!can_deliver) continue;
        auto r = CompleteOrder(player, static_cast<int>(i), current_tick);
        if (r.ok()) {
            result.gold_earned += order.reward_gold;
            result.exp_earned += order.reward_exp;
            ++result.completed;
        }
    }
    return Result<BatchCompleteResult>::success(result);
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

void OrderSystem::Tick(int current_tick, const PlayerState& player, Season season) {
    for (int i = 0; i < static_cast<int>(orders_.size()); ++i) {
        OrderData& order = orders_[static_cast<std::size_t>(i)];
        if (order.id == 0) {
            RefreshOrder(i, player, season);
            continue;
        }
        if (order.state == OrderState::CoolingDown && current_tick >= order.cooldown_until_tick) {
            RefreshOrder(i, player, season);
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

std::vector<ItemId> OrderSystem::BuildItemPool(const PlayerState& player, Season season) const {
    std::vector<ItemId> pool;

    if (player.IsSeedUnlocked(ItemId::WheatSeed))
        for (int i = 0; i < 3; ++i) pool.push_back(ItemId::Wheat);
    if (player.IsSeedUnlocked(ItemId::CornSeed))
        for (int i = 0; i < 3; ++i) pool.push_back(ItemId::Corn);
    if (player.IsSeedUnlocked(ItemId::CarrotSeed))
        for (int i = 0; i < 3; ++i) pool.push_back(ItemId::Carrot);
    if (player.IsSeedUnlocked(ItemId::TomatoSeed))
        for (int i = 0; i < 3; ++i) pool.push_back(ItemId::Tomato);

    if (player.IsUnlocked(UnlockId::ChickenCoop) && player.IsUnlocked(UnlockId::Chicken))
        pool.push_back(ItemId::Egg);
    if (player.IsUnlocked(UnlockId::CowBarn) && player.IsUnlocked(UnlockId::Cow))
        pool.push_back(ItemId::Milk);
    if (player.IsUnlocked(UnlockId::SheepPen) && player.IsUnlocked(UnlockId::Sheep))
        pool.push_back(ItemId::Wool);

    if (player.IsSeedUnlocked(ItemId::WheatSeed))
        pool.push_back(ItemId::ChickenFeed);
    if (player.IsSeedUnlocked(ItemId::CornSeed) && player.IsSeedUnlocked(ItemId::CarrotSeed))
        pool.push_back(ItemId::CowFeed);

    if (player.Level() >= 2 && player.IsSeedUnlocked(ItemId::WheatSeed))
        pool.push_back(ItemId::Bread);
    if (player.Level() >= 3 && player.IsUnlocked(UnlockId::CowBarn) && player.IsUnlocked(UnlockId::Cow))
        pool.push_back(ItemId::Cheese);
    if (player.Level() >= 4 && player.IsSeedUnlocked(ItemId::TomatoSeed))
        pool.push_back(ItemId::Jam);

    if (season == Season::Spring && player.IsSeedUnlocked(ItemId::StrawberrySeed))
        for (int i = 0; i < 3; ++i) pool.push_back(ItemId::Strawberry);
    if (season == Season::Summer && player.IsSeedUnlocked(ItemId::TomatoSeed))
        for (int i = 0; i < 3; ++i) pool.push_back(ItemId::Tomato);
    if (season == Season::Autumn && player.IsSeedUnlocked(ItemId::PumpkinSeed))
        for (int i = 0; i < 3; ++i) pool.push_back(ItemId::Pumpkin);
    if (season == Season::Winter && player.IsSeedUnlocked(ItemId::MushroomSeed))
        for (int i = 0; i < 3; ++i) pool.push_back(ItemId::Mushroom);

    if (pool.empty()) pool.push_back(ItemId::Wheat);
    return pool;
}

namespace {
ItemId PickFromPool(const std::vector<ItemId>& pool, int mix, int offset) {
    return pool[static_cast<std::size_t>((mix + offset) % static_cast<int>(pool.size()))];
}

int QtyFor(ItemId item, int mix, int level) {
    switch (item) {
        case ItemId::Wheat:
        case ItemId::Corn:
        case ItemId::Carrot:
        case ItemId::Tomato:
        case ItemId::Strawberry:
        case ItemId::Pumpkin:
        case ItemId::Mushroom: {
            if (level <= 1) return 1 + (mix % 3);
            if (level == 2) return 2 + (mix % 3);
            if (level == 3) return 3 + (mix % 4);
            return 4 + level + (mix % 5);
        }
        case ItemId::Egg:         return 1 + level / 2 + (mix % 3);
        case ItemId::Milk:        return 1 + level / 3 + (mix % 2);
        case ItemId::Wool:        return 1 + (mix % 2);
        case ItemId::ChickenFeed: return 1 + level / 3 + (mix % 3);
        case ItemId::CowFeed:     return 1 + level / 3 + (mix % 2);
        case ItemId::Bread:       return 1 + level / 2 + (mix % 3);
        case ItemId::Cheese:      return 1 + level / 3 + (mix % 2);
        case ItemId::Jam:         return 1 + level / 3 + (mix % 2);
        default:                  return 1;
    }
}
}  // namespace

void OrderSystem::RefreshOrder(int slot, const PlayerState& player, Season season) {
    const std::vector<ItemId> pool = BuildItemPool(player, season);
    ++sequence_;
    const int mix = sequence_ + slot * 11;

    if (next_order_id_ <= 3) {
        std::vector<OrderRequirement> reqs;
        reqs.push_back({ItemId::Wheat, 1 + (mix % 2)});
        OrderData order;
        order.id = next_order_id_++;
        order.requirements = reqs;
        order.reward_gold = RewardGold(reqs) + 2;
        order.reward_exp = 4 + 2;
        order.label = "新手采购订单";
        orders_[static_cast<std::size_t>(slot)] = order;
        return;
    }

    const int category = mix % 10;
    std::vector<OrderRequirement> reqs;
    const char* label = "";
    int base_reward = 0;

    if (pool.size() >= 3 && category >= 3 && category < 7) {
        ItemId item1 = PickFromPool(pool, mix, 0);
        ItemId item2;
        int tries = 0;
        do {
            item2 = PickFromPool(pool, mix, 1 + tries);
            ++tries;
        } while (item2 == item1 && tries < 20);

        int q1 = QtyFor(item1, mix, player.Level());
        int q2 = QtyFor(item2, mix + 3, player.Level());
        reqs.push_back({item1, q1});
        reqs.push_back({item2, q2});
        label = "混合订单";
        base_reward = 5;
    } else if (pool.size() >= 4 && category >= 7) {
        const bool has_wheat = player.IsSeedUnlocked(ItemId::WheatSeed);
        const bool has_corn = player.IsSeedUnlocked(ItemId::CornSeed);
        const bool has_carrot = player.IsSeedUnlocked(ItemId::CarrotSeed);
        const bool has_egg = player.IsUnlocked(UnlockId::ChickenCoop) && player.IsUnlocked(UnlockId::Chicken);
        const bool has_milk = player.IsUnlocked(UnlockId::CowBarn) && player.IsUnlocked(UnlockId::Cow);
        const bool has_wool = player.IsUnlocked(UnlockId::SheepPen) && player.IsUnlocked(UnlockId::Sheep);
        const bool has_tomato = player.IsSeedUnlocked(ItemId::TomatoSeed);

        struct Theme { const char* name; std::vector<OrderRequirement> reqs; int bonus; };
        std::vector<Theme> themes;

        if (has_wheat && has_egg)
            themes.push_back({"早餐篮", {{ItemId::Wheat, 5}, {ItemId::Egg, 2}}, 12});
        if (has_corn && has_carrot)
            themes.push_back({"饲料原料包", {{ItemId::Corn, 4}, {ItemId::Carrot, 3}}, 10});
        if (has_wheat && has_corn && has_carrot)
            themes.push_back({"杂粮大礼包", {{ItemId::Wheat, 5}, {ItemId::Corn, 3}, {ItemId::Carrot, 3}}, 15});
        if (has_egg && has_milk)
            themes.push_back({"乳蛋特供", {{ItemId::Egg, 3}, {ItemId::Milk, 2}}, 12});
        if (has_tomato && has_wheat)
            themes.push_back({"田园时蔬篮", {{ItemId::Tomato, 4}, {ItemId::Wheat, 4}}, 12});
        if (has_wheat && has_egg && has_milk)
            themes.push_back({"农场晨间套餐", {{ItemId::Wheat, 5}, {ItemId::Egg, 2}, {ItemId::Milk, 1}}, 16});
        if (has_wool && has_wheat)
            themes.push_back({"纺织原料包", {{ItemId::Wool, 2}, {ItemId::Wheat, 6}}, 15});

        const bool has_bread = player.Level() >= 2 && has_wheat;
        const bool has_cheese = player.Level() >= 3 && has_milk;
        const bool has_jam = player.Level() >= 4 && has_tomato;
        if (has_bread)
            themes.push_back({"面包房订单", {{ItemId::Bread, 2}}, 10});
        if (has_bread && has_egg)
            themes.push_back({"早餐套装", {{ItemId::Bread, 2}, {ItemId::Egg, 2}}, 14});
        if (has_cheese)
            themes.push_back({"奶酪工坊", {{ItemId::Cheese, 1}}, 12});
        if (has_cheese && has_bread)
            themes.push_back({"西式套餐", {{ItemId::Bread, 2}, {ItemId::Cheese, 1}}, 16});
        if (has_jam)
            themes.push_back({"果酱工坊", {{ItemId::Jam, 1}}, 14});
        if (has_jam && has_bread)
            themes.push_back({"甜蜜套餐", {{ItemId::Bread, 2}, {ItemId::Jam, 1}}, 17});

        if (!themes.empty()) {
            const Theme& theme = themes[static_cast<std::size_t>(mix % static_cast<int>(themes.size()))];
            reqs = theme.reqs;
            label = theme.name;
            base_reward = theme.bonus;
        } else {
            ItemId item = PickFromPool(pool, mix, 0);
            reqs.push_back({item, QtyFor(item, mix, player.Level())});
            label = "采购订单";
            base_reward = 2;
        }
    } else {
        ItemId item = PickFromPool(pool, mix, 0);
        reqs.push_back({item, QtyFor(item, mix, player.Level())});
        label = "采购订单";
        base_reward = 2;
    }

    OrderData order;
    order.id = next_order_id_++;
    order.requirements = reqs;
    float winter_mult = (season == Season::Winter) ? 1.5f : 1.0f;
    order.reward_gold = static_cast<int>((RewardGold(reqs) + base_reward) * winter_mult);
    order.reward_exp = static_cast<int>((4 + base_reward + player.Level()) * winter_mult);
    order.label = label;
    orders_[static_cast<std::size_t>(slot)] = order;
}

int OrderSystem::RewardGold(const std::vector<OrderRequirement>& reqs) const {
    int total = 0;
    for (const OrderRequirement& req : reqs) {
        total += GetItemInfo(req.item).sell_price * req.quantity * 3 / 2;
    }
    return total + 2;
}

}  // namespace farm
