#pragma once

#include "farm/common/Types.h"

#include <string>
#include <vector>

namespace farm {

class PlayerState;
class AchievementSystem;
class DailyTaskSystem;

struct OrderData {
    int id = 0;
    OrderState state = OrderState::Available;
    std::vector<OrderRequirement> requirements;
    int reward_gold = 0;
    int reward_exp = 0;
    int cooldown_until_tick = 0;
    bool locked = false;
    std::string label;  // e.g. "早餐篮", "饲料原料包"
};

struct BatchCompleteResult {
    int completed = 0;
    int gold_earned = 0;
    int exp_earned = 0;
};

class OrderSystem {
public:
    OrderSystem();

    const std::vector<OrderData>& Orders() const { return orders_; }
    bool CanDeliver(const PlayerState& player, int slot) const;
    Result<void> CompleteOrder(PlayerState& player, int slot, int current_tick);
    Result<void> AbandonOrder(int slot, int current_tick);
    int AbandonOrders(const std::vector<int>& slots, int current_tick);
    Result<BatchCompleteResult> BatchComplete(PlayerState& player, int current_tick);
    Result<void> SetLocked(int slot, bool locked);
    void Tick(int current_tick, const PlayerState& player, Season season);

    // Called by Game after construction to wire up event systems
    void Setup(AchievementSystem* ach, DailyTaskSystem* dts);

    // Save/Load — called by SaveManager via Game
    void ClearForLoad();
    void SetForLoad(const std::vector<OrderData>& orders, int next_order_id, int sequence);
    int NextOrderIdForSave() const { return next_order_id_; }
    int SequenceForSave() const { return sequence_; }

private:
    void RefreshOrder(int slot, const PlayerState& player, Season season);
    std::vector<ItemId> BuildItemPool(const PlayerState& player, Season season) const;
    int RewardGold(const std::vector<OrderRequirement>& reqs) const;

    std::vector<OrderData> orders_;
    int next_order_id_ = 1;
    int sequence_ = 0;

    // Event system pointers (non-owning)
    AchievementSystem* ach_ = nullptr;
    DailyTaskSystem* dts_ = nullptr;
};

}  // namespace farm
