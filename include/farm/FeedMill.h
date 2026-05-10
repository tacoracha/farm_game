#pragma once

#include "Types.h"

#include <iosfwd>

namespace farm {

class PlayerState;

enum class RecipeId : std::uint8_t {
    ChickenFeed,
    CowFeed,
};

enum class FeedMillState : std::uint8_t {
    Idle,
    Producing,
    ReadyToCollect,
};

class FeedMill {
public:
    [[nodiscard]] Result<void> StartProduction(PlayerState& player, RecipeId recipe_id,
                                                 int current_tick);

    void Tick(int current_tick);

    [[nodiscard]] Result<void> Collect(PlayerState& player);

    [[nodiscard]] FeedMillState State() const { return state_; }

    [[nodiscard]] bool IsIdle() const { return state_ == FeedMillState::Idle; }

    [[nodiscard]] bool IsProducing() const { return state_ == FeedMillState::Producing; }

    [[nodiscard]] bool IsReadyToCollect() const {
        return state_ == FeedMillState::ReadyToCollect;
    }

    void WriteStatus(std::ostream& os) const;

private:
    FeedMillState state_ = FeedMillState::Idle;
    int finish_tick_ = 0;
    ItemId output_item_ = ItemId::ChickenFeed;
    int output_quantity_ = 0;

    [[nodiscard]] static Result<void> ConsumeChickenFeedIngredients(PlayerState& player);
    [[nodiscard]] static Result<void> ConsumeCowFeedIngredients(PlayerState& player);
};

}  // namespace farm
