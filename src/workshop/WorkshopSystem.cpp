#include "farm/workshop/WorkshopSystem.h"

#include "farm/common/Constants.h"
#include "farm/inventory/PlayerState.h"

namespace farm {

WorkshopView WorkshopSystem::View() const {
    return WorkshopView{static_cast<int>(queue_.size()), kFeedMillQueueCapacity,
                        shelf_chicken_feed_, kFeedMillShelfCapacity,
                        queue_.empty() ? 0 : queue_.front().remaining_ticks};
}

Result<void> WorkshopSystem::StartProduction(PlayerState& player, RecipeId recipe, int times) {
    if (recipe != RecipeId::ChickenFeed) {
        return Result<void>::failure(ErrorCode::RecipeUnavailable);
    }
    if (times <= 0) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    if (static_cast<int>(queue_.size()) + times > kFeedMillQueueCapacity) {
        return Result<void>::failure(ErrorCode::ProductionQueueFull);
    }
    const int required_wheat = 2 * times;
    if (!player.HasItem(ItemId::Wheat, required_wheat)) {
        return Result<void>::failure(ErrorCode::InsufficientItem);
    }
    if (player.IsItemLocked(ItemId::Wheat)) {
        return Result<void>::failure(ErrorCode::ProtectedItem);
    }
    auto removed = player.TryRemoveItem(ItemId::Wheat, required_wheat);
    if (!removed.ok()) {
        return removed;
    }
    for (int i = 0; i < times; ++i) {
        queue_.push_back(ProductionJob{recipe, kChickenFeedTicks});
    }
    return Result<void>::success();
}

Result<void> WorkshopSystem::ClaimProduct(PlayerState& player) {
    if (shelf_chicken_feed_ <= 0) {
        return Result<void>::failure(ErrorCode::ProductNotReady);
    }
    auto added = player.TryAddItem(ItemId::ChickenFeed, 1);
    if (!added.ok()) {
        return added;
    }
    --shelf_chicken_feed_;
    return Result<void>::success();
}

Result<int> WorkshopSystem::ClaimAllProducts(PlayerState& player) {
    int claimed = 0;
    while (shelf_chicken_feed_ > 0) {
        auto one = ClaimProduct(player);
        if (!one.ok()) {
            break;
        }
        ++claimed;
    }
    return Result<int>::success(claimed);
}

void WorkshopSystem::Tick() {
    if (queue_.empty()) {
        return;
    }
    if (queue_.front().remaining_ticks > 0) {
        --queue_.front().remaining_ticks;
    }
    if (queue_.front().remaining_ticks <= 0 && shelf_chicken_feed_ < kFeedMillShelfCapacity) {
        ++shelf_chicken_feed_;
        queue_.erase(queue_.begin());
    }
}

void WorkshopSystem::ClearForLoad() {
    queue_.clear();
    shelf_chicken_feed_ = 0;
}

void WorkshopSystem::SetForLoad(const std::vector<ProductionJob>& queue, int shelf_chicken_feed) {
    queue_ = queue;
    shelf_chicken_feed_ = shelf_chicken_feed;
}

Result<void> WorkshopSystem::ConsumeInputs(PlayerState& player, RecipeId recipe) const {
    if (recipe != RecipeId::ChickenFeed) {
        return Result<void>::failure(ErrorCode::RecipeUnavailable);
    }
    return player.TryRemoveItem(ItemId::Wheat, 2);
}

}  // namespace farm

