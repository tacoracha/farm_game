#include "farm/FeedMill.h"

#include "farm/Constants.h"
#include "farm/ItemCatalog.h"
#include "farm/PlayerState.h"

#include <ostream>

namespace farm {

namespace {

int RecipeDurationTicks(RecipeId id) {
    switch (id) {
        case RecipeId::ChickenFeed:
            return kFeedMillChickenFeedTicks;
        case RecipeId::CowFeed:
            return kFeedMillCowFeedTicks;
    }
    return 0;
}

ItemId RecipeOutputItem(RecipeId id) {
    switch (id) {
        case RecipeId::ChickenFeed:
            return ItemId::ChickenFeed;
        case RecipeId::CowFeed:
            return ItemId::CowFeed;
    }
    return ItemId::ChickenFeed;
}

}  // namespace

Result<void> FeedMill::ConsumeChickenFeedIngredients(PlayerState& player) {
    return player.TryRemoveFromWarehouse(ItemId::Wheat, 2);
}

Result<void> FeedMill::ConsumeCowFeedIngredients(PlayerState& player) {
    auto corn_removed = player.TryRemoveFromWarehouse(ItemId::Corn, 2);
    if (!corn_removed.ok()) {
        return corn_removed;
    }
    auto carrot_removed = player.TryRemoveFromWarehouse(ItemId::Carrot, 1);
    if (!carrot_removed.ok()) {
        (void)player.TryAddToWarehouse(ItemId::Corn, 2);
        return carrot_removed;
    }
    return Result<void>::success();
}

Result<void> FeedMill::StartProduction(PlayerState& player, RecipeId recipe_id, int current_tick) {
    if (state_ != FeedMillState::Idle) {
        return Result<void>::failure(ErrorCode::FeedMillBusy);
    }

    Result<void> consumed = Result<void>::failure(ErrorCode::InternalError);
    switch (recipe_id) {
        case RecipeId::ChickenFeed:
            consumed = ConsumeChickenFeedIngredients(player);
            break;
        case RecipeId::CowFeed:
            consumed = ConsumeCowFeedIngredients(player);
            break;
    }
    if (!consumed.ok()) {
        return consumed;
    }

    finish_tick_ = current_tick + RecipeDurationTicks(recipe_id);
    output_item_ = RecipeOutputItem(recipe_id);
    output_quantity_ = 1;
    state_ = FeedMillState::Producing;
    return Result<void>::success();
}

void FeedMill::Tick(int current_tick) {
    if (state_ == FeedMillState::Producing && current_tick >= finish_tick_) {
        state_ = FeedMillState::ReadyToCollect;
    }
}

Result<void> FeedMill::Collect(PlayerState& player) {
    if (state_ != FeedMillState::ReadyToCollect) {
        return Result<void>::failure(ErrorCode::FeedMillNotReadyToCollect);
    }

    const ItemId out_item = output_item_;
    const int out_qty = output_quantity_;

    auto added = player.TryAddToWarehouse(out_item, out_qty);
    if (!added.ok()) {
        return added;
    }

    state_ = FeedMillState::Idle;
    finish_tick_ = 0;
    output_quantity_ = 0;
    output_item_ = ItemId::ChickenFeed;
    return Result<void>::success();
}

void FeedMill::WriteStatus(std::ostream& os) const {
    os << "FeedMill: ";
    switch (state_) {
        case FeedMillState::Idle:
            os << "Idle\n";
            return;
        case FeedMillState::Producing:
            os << "Producing until tick " << finish_tick_ << " -> "
               << ItemCatalog::Get(output_item_).name << " x" << output_quantity_ << "\n";
            return;
        case FeedMillState::ReadyToCollect:
            os << "Ready -> " << ItemCatalog::Get(output_item_).name << " x" << output_quantity_
               << "\n";
            return;
    }
}

}  // namespace farm
