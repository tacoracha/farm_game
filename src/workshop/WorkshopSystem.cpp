#include "farm/workshop/WorkshopSystem.h"

#include "farm/common/Constants.h"
#include "farm/inventory/PlayerState.h"

namespace farm {

WorkshopView WorkshopSystem::View() const {
    return WorkshopView{static_cast<int>(queue_.size()), kFeedMillQueueCapacity,
                        shelf_chicken_feed_ + shelf_cow_feed_, shelf_chicken_feed_,
                        shelf_cow_feed_, kFeedMillShelfCapacity,
                        queue_.empty() ? 0 : queue_.front().remaining_ticks};
}

Result<void> WorkshopSystem::StartProduction(PlayerState& player, RecipeId recipe, int times) {
    if (recipe != RecipeId::ChickenFeed && recipe != RecipeId::CowFeed) {
        return Result<void>::failure(ErrorCode::RecipeUnavailable);
    }
    if (times <= 0) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    if (static_cast<int>(queue_.size()) + times > kFeedMillQueueCapacity) {
        return Result<void>::failure(ErrorCode::ProductionQueueFull);
    }
    std::vector<ItemStack> inputs;
    if (recipe == RecipeId::ChickenFeed) {
        inputs.push_back(ItemStack{ItemId::Wheat, 2 * times});
    } else {
        inputs.push_back(ItemStack{ItemId::Corn, 2 * times});
        inputs.push_back(ItemStack{ItemId::Carrot, 1 * times});
    }
    auto removed = player.TryRemoveItems(inputs);
    if (!removed.ok()) {
        return removed;
    }
    for (int i = 0; i < times; ++i) {
        queue_.push_back(ProductionJob{recipe, recipe == RecipeId::ChickenFeed ? kChickenFeedTicks
                                                                                : kCowFeedTicks});
    }
    return Result<void>::success();
}

Result<void> WorkshopSystem::ClaimProduct(PlayerState& player) {
    if (shelf_chicken_feed_ <= 0 && shelf_cow_feed_ <= 0) {
        return Result<void>::failure(ErrorCode::ProductNotReady);
    }
    const ItemId item = shelf_chicken_feed_ > 0 ? ItemId::ChickenFeed : ItemId::CowFeed;
    auto added = player.TryAddItem(item, 1);
    if (!added.ok()) {
        return added;
    }
    if (item == ItemId::ChickenFeed) {
        --shelf_chicken_feed_;
    } else {
        --shelf_cow_feed_;
    }
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
    if (queue_.front().remaining_ticks <= 0 &&
        shelf_chicken_feed_ + shelf_cow_feed_ < kFeedMillShelfCapacity) {
        if (queue_.front().recipe == RecipeId::ChickenFeed) {
            ++shelf_chicken_feed_;
        } else {
            ++shelf_cow_feed_;
        }
        queue_.erase(queue_.begin());
    }
}

void WorkshopSystem::ClearForLoad() {
    queue_.clear();
    shelf_chicken_feed_ = 0;
    shelf_cow_feed_ = 0;
}

void WorkshopSystem::SetForLoad(const std::vector<ProductionJob>& queue, int shelf_chicken_feed,
                                int shelf_cow_feed) {
    queue_ = queue;
    shelf_chicken_feed_ = shelf_chicken_feed;
    shelf_cow_feed_ = shelf_cow_feed;
}

Result<void> WorkshopSystem::ConsumeInputs(PlayerState& player, RecipeId recipe) const {
    if (recipe == RecipeId::ChickenFeed) {
        return player.TryRemoveItem(ItemId::Wheat, 2);
    }
    if (recipe == RecipeId::CowFeed) {
        return player.TryRemoveItems({ItemStack{ItemId::Corn, 2}, ItemStack{ItemId::Carrot, 1}});
    }
    return Result<void>::failure(ErrorCode::RecipeUnavailable);
}

}  // namespace farm
