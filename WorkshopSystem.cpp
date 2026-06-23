#include "farm/workshop/WorkshopSystem.h"

#include "farm/achievement/AchievementSystem.h"
#include "farm/common/Constants.h"
#include "farm/dailytask/DailyTaskSystem.h"
#include "farm/inventory/PlayerState.h"

namespace farm {

void WorkshopSystem::Setup(AchievementSystem* ach, DailyTaskSystem* dts) {
    ach_ = ach;
    dts_ = dts;
}

WorkshopView WorkshopSystem::View() const {
    return WorkshopView{static_cast<int>(queue_.size()), kFeedMillQueueCapacity,
                        shelf_chicken_feed_ + shelf_cow_feed_ + shelf_bread_ + shelf_cheese_ +
                            shelf_jam_,
                        shelf_chicken_feed_, shelf_cow_feed_, shelf_bread_, shelf_cheese_,
                        shelf_jam_, kFeedMillShelfCapacity,
                        queue_.empty() ? 0 : queue_.front().remaining_ticks};
}

int WorkshopSystem::ShelfCount(ItemId product) const {
    switch (product) {
        case ItemId::ChickenFeed: return shelf_chicken_feed_;
        case ItemId::CowFeed:     return shelf_cow_feed_;
        case ItemId::Bread:       return shelf_bread_;
        case ItemId::Cheese:      return shelf_cheese_;
        case ItemId::Jam:         return shelf_jam_;
        default:                  return 0;
    }
}

int WorkshopSystem::MinLevelFor(RecipeId recipe) {
    switch (recipe) {
        case RecipeId::ChickenFeed: return 1;
        case RecipeId::CowFeed:     return 1;
        case RecipeId::Bread:       return 2;
        case RecipeId::Cheese:      return 3;
        case RecipeId::Jam:         return 4;
    }
    return 99;
}

ItemId WorkshopSystem::ProductFor(RecipeId recipe) {
    switch (recipe) {
        case RecipeId::ChickenFeed: return ItemId::ChickenFeed;
        case RecipeId::CowFeed:     return ItemId::CowFeed;
        case RecipeId::Bread:       return ItemId::Bread;
        case RecipeId::Cheese:      return ItemId::Cheese;
        case RecipeId::Jam:         return ItemId::Jam;
    }
    return ItemId::ChickenFeed;
}

int WorkshopSystem::TicksFor(RecipeId recipe) {
    switch (recipe) {
        case RecipeId::ChickenFeed: return kChickenFeedTicks;
        case RecipeId::CowFeed:     return kCowFeedTicks;
        case RecipeId::Bread:       return kBreadTicks;
        case RecipeId::Cheese:      return kCheeseTicks;
        case RecipeId::Jam:         return kJamTicks;
    }
    return 10;
}

Result<void> WorkshopSystem::StartProduction(PlayerState& player, RecipeId recipe, int times,
                                             int player_level) {
    if (recipe != RecipeId::ChickenFeed && recipe != RecipeId::CowFeed &&
        recipe != RecipeId::Bread && recipe != RecipeId::Cheese && recipe != RecipeId::Jam) {
        return Result<void>::failure(ErrorCode::RecipeUnavailable);
    }
    if (player_level < MinLevelFor(recipe)) {
        return Result<void>::failure(ErrorCode::ContentLocked);
    }
    if (times <= 0) {
        return Result<void>::failure(ErrorCode::InvalidQuantity);
    }
    if (static_cast<int>(queue_.size()) + times > kFeedMillQueueCapacity) {
        return Result<void>::failure(ErrorCode::ProductionQueueFull);
    }

    std::vector<ItemStack> inputs;
    switch (recipe) {
        case RecipeId::ChickenFeed:
            inputs.push_back({ItemId::Wheat, 2 * times});
            break;
        case RecipeId::CowFeed:
            inputs.push_back({ItemId::Corn, 2 * times});
            inputs.push_back({ItemId::Carrot, 1 * times});
            break;
        case RecipeId::Bread:
            inputs.push_back({ItemId::Wheat, 3 * times});
            break;
        case RecipeId::Cheese:
            inputs.push_back({ItemId::Milk, 2 * times});
            break;
        case RecipeId::Jam:
            inputs.push_back({ItemId::Tomato, 3 * times});
            break;
    }
    auto removed = player.TryRemoveItems(inputs);
    if (!removed.ok()) {
        return removed;
    }

    for (int i = 0; i < times; ++i) {
        queue_.push_back({recipe, TicksFor(recipe)});
    }
    return Result<void>::success();
}

Result<void> WorkshopSystem::ClaimProduct(PlayerState& player) {
    ItemId item;
    if (shelf_chicken_feed_ > 0) {
        item = ItemId::ChickenFeed;
    } else if (shelf_cow_feed_ > 0) {
        item = ItemId::CowFeed;
    } else if (shelf_bread_ > 0) {
        item = ItemId::Bread;
    } else if (shelf_cheese_ > 0) {
        item = ItemId::Cheese;
    } else if (shelf_jam_ > 0) {
        item = ItemId::Jam;
    } else {
        return Result<void>::failure(ErrorCode::ProductNotReady);
    }
    auto added = player.TryAddItem(item, 1);
    if (!added.ok()) {
        return added;
    }
    switch (item) {
        case ItemId::ChickenFeed: --shelf_chicken_feed_; break;
        case ItemId::CowFeed:     --shelf_cow_feed_;     break;
        case ItemId::Bread:       --shelf_bread_;        break;
        case ItemId::Cheese:      --shelf_cheese_;       break;
        case ItemId::Jam:         --shelf_jam_;          break;
        default: break;
    }
    return Result<void>::success();
}

Result<int> WorkshopSystem::ClaimAllProducts(PlayerState& player) {
    int claimed = 0;
    while (true) {
        auto one = ClaimProduct(player);
        if (!one.ok()) break;
        ++claimed;
    }
    return Result<int>::success(claimed);
}

void WorkshopSystem::Tick(PlayerState& player) {
    if (queue_.empty()) return;
    if (queue_.front().remaining_ticks > 0) {
        --queue_.front().remaining_ticks;
    }
    if (queue_.front().remaining_ticks <= 0) {
        RecipeId recipe = queue_.front().recipe;
        ItemId product = ProductFor(recipe);
        if (player.TryAddItem(product, 1).ok()) {
            queue_.erase(queue_.begin());
            if (ach_) ach_->OnProcessItem();
            if (dts_) dts_->OnProcessFeed(1);
            return;
        }
        if (shelf_chicken_feed_ + shelf_cow_feed_ + shelf_bread_ + shelf_cheese_ +
                shelf_jam_ < kFeedMillShelfCapacity) {
            switch (recipe) {
                case RecipeId::ChickenFeed: ++shelf_chicken_feed_; break;
                case RecipeId::CowFeed:     ++shelf_cow_feed_;     break;
                case RecipeId::Bread:       ++shelf_bread_;        break;
                case RecipeId::Cheese:      ++shelf_cheese_;       break;
                case RecipeId::Jam:         ++shelf_jam_;          break;
            }
            queue_.erase(queue_.begin());
            if (ach_) ach_->OnProcessItem();
            if (dts_) dts_->OnProcessFeed(1);
        }
    }
}

void WorkshopSystem::ClearForLoad() {
    queue_.clear();
    shelf_chicken_feed_ = 0;
    shelf_cow_feed_ = 0;
    shelf_bread_ = 0;
    shelf_cheese_ = 0;
    shelf_jam_ = 0;
}

void WorkshopSystem::SetForLoad(const std::vector<ProductionJob>& queue, int shelf_chicken_feed,
                                int shelf_cow_feed, int shelf_bread, int shelf_cheese,
                                int shelf_jam) {
    queue_ = queue;
    shelf_chicken_feed_ = shelf_chicken_feed;
    shelf_cow_feed_ = shelf_cow_feed;
    shelf_bread_ = shelf_bread;
    shelf_cheese_ = shelf_cheese;
    shelf_jam_ = shelf_jam;
}

Result<void> WorkshopSystem::ConsumeInputs(PlayerState& player, RecipeId recipe) const {
    if (recipe == RecipeId::ChickenFeed) {
        return player.TryRemoveItem(ItemId::Wheat, 2);
    }
    if (recipe == RecipeId::CowFeed) {
        return player.TryRemoveItems({ItemStack{ItemId::Corn, 2}, ItemStack{ItemId::Carrot, 1}});
    }
    if (recipe == RecipeId::Bread) {
        return player.TryRemoveItem(ItemId::Wheat, 3);
    }
    if (recipe == RecipeId::Cheese) {
        return player.TryRemoveItem(ItemId::Milk, 2);
    }
    if (recipe == RecipeId::Jam) {
        return player.TryRemoveItem(ItemId::Tomato, 3);
    }
    return Result<void>::failure(ErrorCode::RecipeUnavailable);
}

}  // namespace farm
