#pragma once

#include "farm/common/Types.h"

#include <vector>

namespace farm {

class PlayerState;

struct PlotData {
    PlotState state = PlotState::Idle;
    PlotWaterState water = PlotWaterState::Dry;
    ItemId crop = ItemId::Wheat;
    int planted_tick = 0;
    int mature_tick = 0;
    bool fertilized = false;
    float growth_remainder = 0.0f;
};

struct PlotView {
    int plot_id = 0;
    PlotState state = PlotState::Idle;
    PlotWaterState water = PlotWaterState::Dry;
    ItemId crop = ItemId::Wheat;
    int remaining_ticks = 0;
    bool fertilized = false;
};

class PlantingSystem {
public:
    PlantingSystem();

    const std::vector<PlotData>& Plots() const { return plots_; }
    std::vector<PlotView> View(int current_tick) const;
    int MatureCount() const;

    Result<void> TryPlant(PlayerState& player, ItemId seed, int current_tick);
    Result<void> TryPlantAt(PlayerState& player, int plot_id, ItemId seed, int current_tick);
    Result<void> WaterPlot(int plot_id, int current_tick);
    Result<void> ApplyFertilizer(PlayerState& player, int plot_id, int current_tick);
    Result<void> Harvest(PlayerState& player, int plot_id);
    Result<int> Expand(PlayerState& player);
    void Tick(int current_tick, float weather_growth_multiplier);

    void ClearForLoad();
    void SetPlotsForLoad(const std::vector<PlotData>& plots);

private:
    void UpdateMaturity(int current_tick);

    std::vector<PlotData> plots_;
};

}  // namespace farm

