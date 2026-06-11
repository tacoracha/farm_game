#pragma once

#include "farm/common/Types.h"

#include <vector>

namespace farm {

class PlayerState;

struct ProductionJob {
    RecipeId recipe = RecipeId::ChickenFeed;
    int remaining_ticks = 0;
};

struct WorkshopView {
    int queue_count = 0;
    int queue_capacity = 0;
    int shelf_count = 0;
    int chicken_feed_shelf = 0;
    int cow_feed_shelf = 0;
    int bread_shelf = 0;
    int cheese_shelf = 0;
    int jam_shelf = 0;
    int shelf_capacity = 0;
    int active_remaining_ticks = 0;
};

class WorkshopSystem {
public:
    WorkshopView View() const;
    const std::vector<ProductionJob>& Queue() const { return queue_; }
    int ShelfChickenFeed() const { return shelf_chicken_feed_; }
    int ShelfCowFeed() const { return shelf_cow_feed_; }
    int ShelfBread() const { return shelf_bread_; }
    int ShelfCheese() const { return shelf_cheese_; }
    int ShelfJam() const { return shelf_jam_; }

    Result<void> StartProduction(PlayerState& player, RecipeId recipe, int times, int player_level);
    Result<void> ClaimProduct(PlayerState& player);
    Result<int> ClaimAllProducts(PlayerState& player);
    void Tick(PlayerState& player);

    void ClearForLoad();
    void SetForLoad(const std::vector<ProductionJob>& queue, int shelf_chicken_feed,
                    int shelf_cow_feed, int shelf_bread = 0, int shelf_cheese = 0,
                    int shelf_jam = 0);

private:
    Result<void> ConsumeInputs(PlayerState& player, RecipeId recipe) const;
    static int MinLevelFor(RecipeId recipe);
    static ItemId ProductFor(RecipeId recipe);
    static int TicksFor(RecipeId recipe);

    std::vector<ProductionJob> queue_;
    int shelf_chicken_feed_ = 0;
    int shelf_cow_feed_ = 0;
    int shelf_bread_ = 0;
    int shelf_cheese_ = 0;
    int shelf_jam_ = 0;
};

}  // namespace farm
