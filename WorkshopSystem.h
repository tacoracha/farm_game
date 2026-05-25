#pragma once

#include "Types.h"

#include <memory>
#include <string_view>
#include <vector>

namespace farm {

struct ItemStack {
    ItemId item_id;
    int quantity;
};

struct RecipeView {
    RecipeId recipe_id;
    FactoryKind factory_kind;
    std::string_view name;
    std::vector<ItemStack> inputs;
    std::vector<ItemStack> outputs;
    int production_ticks;
    int unlock_level;
    int exp_reward;
};

struct ProductionSlotView {
    int slot_id;
    RecipeId recipe_id;
    int remaining_ticks;
    int batch_remaining;
    bool running;
};

struct FactoryView {
    int factory_id;
    FactoryKind kind;
    int level;
    int queue_size;
    int shelf_used;
    int shelf_capacity;
    int max_queue_length;
    int proficiency_exp;
};

class PlayerState;

class WorkshopSystem {
public:
    WorkshopSystem();
    ~WorkshopSystem();

    // Original interface (for backward compatibility)
    void Tick(int current_tick);
    void WriteStatus(std::ostream& os) const;

    // New enhanced interface
    void Init();
    void OnTick(int tick_delta);
    Result<void> ProcessOfflineTicks(PlayerState& player, int offline_ticks);

    std::vector<FactoryView> GetFactories() const;
    std::vector<RecipeView> GetAvailableRecipes(int factory_id) const;
    std::vector<ProductionSlotView> GetProductionSlots(int factory_id) const;
    int GetShelfItemCount(int factory_id) const;
    float GetSpeedMultiplier(int factory_id) const;   // deprecated, always 1.0f
    int GetOutputMultiplier(int factory_id) const;    // output quantity = recipe output × level

    Result<void> BuildFactory(PlayerState& player, FactoryKind kind);
    Result<void> UpgradeFactory(PlayerState& player, int factory_id);
    Result<void> StartProduction(PlayerState& player, int factory_id, RecipeId recipe_id,
                                 int times, int current_tick);
    Result<void> CancelQueuedProduction(PlayerState& player, int factory_id, int queue_id);
    Result<void> ClaimProduct(PlayerState& player, int factory_id, int shelf_slot);
    Result<int> ClaimAllProducts(PlayerState& player, int factory_id);

    bool HasAvailableFeedRecipe(ItemId feed_id) const;
    bool IsFactoryProductUnlocked(ItemId product_id) const;

    // Additional interfaces for enhanced functionality
    Result<void> InsertProductionAtFront(PlayerState& player, int factory_id, RecipeId recipe_id,
                                         int times, int current_tick);
    Result<void> ReplaceQueuedProduction(PlayerState& player, int factory_id, int queue_id,
                                         RecipeId new_recipe_id, int new_times);
    Result<void> QuickStartProduction(PlayerState& player, int factory_id, int current_tick);
    bool IsHighValueMaterial(ItemId item_id) const;
    bool IsMaterialProtected(PlayerState& player, ItemId item_id) const;
    int GetFactoryLevel(int factory_id) const;
    int GetProficiencyExp(int factory_id) const;
    Result<void> AddProficiencyExp(int factory_id, int exp);
    bool CanAutoCollect(int factory_id) const;
    void SetAutoCollect(int factory_id, bool enabled);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace farm