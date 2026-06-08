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
    int shelf_capacity = 0;
    int active_remaining_ticks = 0;
};

class WorkshopSystem {
public:
    WorkshopView View() const;
    const std::vector<ProductionJob>& Queue() const { return queue_; }
    int ShelfChickenFeed() const { return shelf_chicken_feed_; }

    Result<void> StartProduction(PlayerState& player, RecipeId recipe, int times);
    Result<void> ClaimProduct(PlayerState& player);
    Result<int> ClaimAllProducts(PlayerState& player);
    void Tick();

    void ClearForLoad();
    void SetForLoad(const std::vector<ProductionJob>& queue, int shelf_chicken_feed);

private:
    Result<void> ConsumeInputs(PlayerState& player, RecipeId recipe) const;

    std::vector<ProductionJob> queue_;
    int shelf_chicken_feed_ = 0;
};

}  // namespace farm

