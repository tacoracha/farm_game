#pragma once

#include "Constants.h"
#include "Types.h"

#include <vector>

namespace farm {

class PlayerState;

enum class PlotWaterState {
    Dry,
    Watered,
};

struct Plot {
    PlotState state = PlotState::Idle;
    PlotWaterState water_state = PlotWaterState::Dry;
    ItemId crop = ItemId::Wheat;
    int planted_tick = 0;
    int mature_tick = 0;
    bool fertilized = false;
    float growth_remainder = 0.0f;
};

struct PlotView {
    int plot_id = -1;
    PlotState state = PlotState::Idle;
    PlotWaterState water_state = PlotWaterState::Dry;
    ItemId crop_id = ItemId::Wheat;
    int planted_tick = 0;
    int finish_tick = 0;
    int remaining_ticks = 0;
    bool fertilized = false;
};

struct CropConfig {
    ItemId seed_id = ItemId::WheatSeed;
    ItemId crop_id = ItemId::Wheat;
    int grow_ticks = 0;
    int seed_shop_price = 0;
    int sell_price = 0;
};

struct CropProductionEstimate {
    ItemId crop_id = ItemId::Wheat;
    int mature_count = 0;
    int growing_count = 0;
    int estimated_daily_output = 0;
};

class PlantingSystem {
public:
    PlantingSystem();

    const std::vector<Plot>& Plots() const { return plots_; }

    void Init(int initial_plot_count);
    void OnTick(int tick_delta, float weather_growth_multiplier);
    const std::vector<PlotView>& GetPlotsForDisplay() const;
    PlotView GetPlot(int plot_id) const;
    int GetMatureCropCount() const;
    int GetGrowingCount() const;
    std::vector<CropProductionEstimate> EstimateCropOutput() const;

    [[nodiscard]] Result<void> TryPlant(PlayerState& player, ItemId seed_id, int current_tick);
    [[nodiscard]] Result<void> TryPlantAt(PlayerState& player, int plot_id, ItemId seed_id,
                                          int current_tick);
    void Tick(int current_tick);
    [[nodiscard]] Result<void> WaterPlot(int plot_id, int current_tick);
    [[nodiscard]] Result<void> ApplyFertilizer(PlayerState& player, int plot_id,
                                               ItemId fertilizer_id);
    [[nodiscard]] Result<void> TryHarvest(PlayerState& player, int plot_index);
    [[nodiscard]] Result<int> ExpandPlot(PlayerState& player, int player_level);

    bool IsCropUnlocked(ItemId crop_id) const;
    bool IsSeedUnlocked(ItemId seed_id) const;
    const std::vector<CropConfig>& GetAllCropConfigs() const { return crop_configs_; }

private:
    void UpdateMaturity(int current_tick);
    void RefreshPlotViews() const;

    std::vector<Plot> plots_;
    std::vector<CropConfig> crop_configs_;
    mutable std::vector<PlotView> plot_views_;
    int last_tick_ = 0;
};

}  // namespace farm
