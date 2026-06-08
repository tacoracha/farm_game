#include "farm/planting/PlantingSystem.h"

#include "farm/common/Constants.h"
#include "farm/inventory/PlayerState.h"

#include <algorithm>
#include <cmath>

namespace farm {

PlantingSystem::PlantingSystem()
    : plots_(static_cast<std::size_t>(kInitialPlotCount)) {}

std::vector<PlotView> PlantingSystem::View(int current_tick) const {
    std::vector<PlotView> view;
    view.reserve(plots_.size());
    for (std::size_t i = 0; i < plots_.size(); ++i) {
        const PlotData& plot = plots_[i];
        const int remaining = plot.state == PlotState::Growing
                                  ? std::max(0, plot.mature_tick - current_tick)
                                  : 0;
        view.push_back(PlotView{static_cast<int>(i), plot.state, plot.water, plot.crop, remaining,
                                plot.fertilized});
    }
    return view;
}

int PlantingSystem::MatureCount() const {
    int count = 0;
    for (const PlotData& plot : plots_) {
        if (plot.state == PlotState::Mature) {
            ++count;
        }
    }
    return count;
}

Result<void> PlantingSystem::TryPlant(PlayerState& player, ItemId seed, int current_tick) {
    for (std::size_t i = 0; i < plots_.size(); ++i) {
        if (plots_[i].state == PlotState::Idle) {
            return TryPlantAt(player, static_cast<int>(i), seed, current_tick);
        }
    }
    return Result<void>::failure(ErrorCode::NoIdlePlot);
}

Result<void> PlantingSystem::TryPlantAt(PlayerState& player, int plot_id, ItemId seed,
                                        int current_tick) {
    if (!IsSeed(seed)) {
        return Result<void>::failure(ErrorCode::NotASeed);
    }
    if (!player.IsSeedUnlocked(seed)) {
        return Result<void>::failure(ErrorCode::SeedNotUnlocked);
    }
    if (plot_id < 0 || plot_id >= static_cast<int>(plots_.size())) {
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    }
    UpdateMaturity(current_tick);
    PlotData& plot = plots_[static_cast<std::size_t>(plot_id)];
    if (plot.state != PlotState::Idle) {
        return Result<void>::failure(ErrorCode::PlotNotIdle);
    }
    auto removed = player.TryRemoveItem(seed, 1);
    if (!removed.ok()) {
        return removed;
    }
    const ItemInfo& seed_info = GetItemInfo(seed);
    plot.state = PlotState::Growing;
    plot.water = PlotWaterState::Dry;
    plot.crop = CropFromSeed(seed);
    plot.planted_tick = current_tick;
    plot.mature_tick = current_tick + seed_info.grow_ticks;
    plot.fertilized = false;
    plot.growth_remainder = 0.0f;
    return Result<void>::success();
}

Result<void> PlantingSystem::WaterPlot(int plot_id, int current_tick) {
    if (plot_id < 0 || plot_id >= static_cast<int>(plots_.size())) {
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    }
    UpdateMaturity(current_tick);
    PlotData& plot = plots_[static_cast<std::size_t>(plot_id)];
    if (plot.state != PlotState::Growing) {
        return Result<void>::failure(ErrorCode::PlotNotGrowing);
    }
    if (plot.water == PlotWaterState::Watered) {
        return Result<void>::failure(ErrorCode::PlotAlreadyWatered);
    }
    plot.water = PlotWaterState::Watered;
    plot.mature_tick = std::max(current_tick, plot.mature_tick - kWaterGrowthBoostTicks);
    UpdateMaturity(current_tick);
    return Result<void>::success();
}

Result<void> PlantingSystem::ApplyFertilizer(PlayerState& player, int plot_id, int current_tick) {
    if (plot_id < 0 || plot_id >= static_cast<int>(plots_.size())) {
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    }
    UpdateMaturity(current_tick);
    PlotData& plot = plots_[static_cast<std::size_t>(plot_id)];
    if (plot.state != PlotState::Growing) {
        return Result<void>::failure(ErrorCode::PlotNotGrowing);
    }
    if (plot.fertilized) {
        return Result<void>::failure(ErrorCode::PlotAlreadyFertilized);
    }
    auto removed = player.TryRemoveItem(ItemId::Fertilizer, 1);
    if (!removed.ok()) {
        return removed;
    }
    const int remaining = std::max(1, plot.mature_tick - current_tick);
    plot.mature_tick = current_tick + std::max(1, (remaining + 1) / 2);
    plot.fertilized = true;
    return Result<void>::success();
}

Result<void> PlantingSystem::Harvest(PlayerState& player, int plot_id) {
    if (plot_id < 0 || plot_id >= static_cast<int>(plots_.size())) {
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    }
    PlotData& plot = plots_[static_cast<std::size_t>(plot_id)];
    if (plot.state != PlotState::Mature) {
        return Result<void>::failure(ErrorCode::PlotNotMature);
    }
    auto added = player.TryAddItem(plot.crop, 1);
    if (!added.ok()) {
        return added;
    }
    plot = PlotData{};
    return Result<void>::success();
}

Result<int> PlantingSystem::Expand(PlayerState& player) {
    if (static_cast<int>(plots_.size()) >= kMaxPlotCount) {
        return Result<int>::failure(ErrorCode::PlotOutOfRange);
    }
    const int extra = static_cast<int>(plots_.size()) - kInitialPlotCount;
    auto spent = player.TrySpendGold(kPlotExpansionBaseCost + extra * 20);
    if (!spent.ok()) {
        return Result<int>::failure(spent.code);
    }
    plots_.push_back(PlotData{});
    return Result<int>::success(static_cast<int>(plots_.size()) - 1);
}

void PlantingSystem::Tick(int current_tick, float weather_growth_multiplier) {
    const float multiplier = std::max(0.1f, weather_growth_multiplier);
    for (PlotData& plot : plots_) {
        if (plot.state != PlotState::Growing) {
            continue;
        }
        plot.growth_remainder += multiplier - 1.0f;
        while (plot.growth_remainder >= 1.0f) {
            --plot.mature_tick;
            plot.growth_remainder -= 1.0f;
        }
        while (plot.growth_remainder <= -1.0f) {
            ++plot.mature_tick;
            plot.growth_remainder += 1.0f;
        }
    }
    UpdateMaturity(current_tick);
}

void PlantingSystem::ClearForLoad() { plots_.clear(); }

void PlantingSystem::SetPlotsForLoad(const std::vector<PlotData>& plots) { plots_ = plots; }

void PlantingSystem::UpdateMaturity(int current_tick) {
    for (PlotData& plot : plots_) {
        if (plot.state == PlotState::Growing && current_tick >= plot.mature_tick) {
            plot.state = PlotState::Mature;
        }
    }
}

}  // namespace farm

