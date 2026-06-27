#pragma once

#include "farm/common/Types.h"

#include <vector>

namespace farm {

class PlayerState;
class AchievementSystem;
class DailyTaskSystem;

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

    Result<void> TryPlant(PlayerState& player, ItemId seed, int current_tick, Season season);
    Result<void> TryPlantAt(PlayerState& player, int plot_id, ItemId seed, int current_tick, Season season);
    Result<void> WaterPlot(int plot_id, int current_tick);
    Result<void> ApplyFertilizer(PlayerState& player, int plot_id, int current_tick);
    Result<void> Harvest(PlayerState& player, int plot_id);
    Result<int> Expand(PlayerState& player);
    Result<int> BuildGreenhouse(PlayerState& player);

    // Greenhouse access
    const std::vector<PlotData>& GreenhousePlots() const { return greenhouse_plots_; }
    int GreenhousePlotCount() const { return static_cast<int>(greenhouse_plots_.size()); }
    std::vector<PlotView> GreenhouseView(int current_tick) const;

    // Batch operations — return count of affected plots
    Result<int> BatchPlant(PlayerState& player, ItemId seed, int current_tick, Season season);
    Result<int> BatchWater(int current_tick);
    Result<int> BatchFertilize(PlayerState& player, int current_tick);
    Result<int> BatchHarvest(PlayerState& player);
    void Tick(int current_tick, float weather_growth_multiplier);
    int WitherNonSeasonal(Season new_season);

    static bool CanPlantInSeason(ItemId seed, Season season);

    // Greenhouse operations (no season check, +10% speed)
    Result<void> TryPlantGreenhouse(PlayerState& player, ItemId seed, int current_tick);
    Result<void> TryPlantGreenhouseAt(PlayerState& player, int plot_id, ItemId seed, int current_tick);
    Result<void> WaterGreenhousePlot(int plot_id, int current_tick);
    Result<void> HarvestGreenhouse(PlayerState& player, int plot_id);
    Result<int> BatchPlantGreenhouse(PlayerState& player, ItemId seed, int current_tick);
    Result<int> BatchWaterGreenhouse(int current_tick);
    Result<int> BatchHarvestGreenhouse(PlayerState& player);

    // Called by Game after construction to wire up event systems
    void Setup(AchievementSystem* ach, DailyTaskSystem* dts);

    // Save/Load — called by SaveManager via Game
    void ClearForLoad();
    void SetPlotsForLoad(const std::vector<PlotData>& plots);
    void SetGreenhouseForLoad(const std::vector<PlotData>& plots);

private:
    friend class Game;  // for SetSummerDrought / SetAutumnDouble

    void SetSummerDrought(bool active);
    void SetAutumnDouble(bool active);
    void UpdateMaturity(int current_tick);
    void UpdateMaturity(std::vector<PlotData>& plots, int current_tick);

    std::vector<PlotData> plots_;
    std::vector<PlotData> greenhouse_plots_;
    bool summer_drought_ = false;
    bool autumn_double_ = false;

    // Event system pointers (non-owning)
    AchievementSystem* ach_ = nullptr;
    DailyTaskSystem* dts_ = nullptr;
};

}  // namespace farm
