#include "farm/PlantingSystem.h"

#include "farm/ItemCatalog.h"
#include "farm/PlayerState.h"

#include <algorithm>

namespace farm {

namespace {

bool IsFertilizer(ItemId item_id) {
    return item_id == ItemId::Fertilizer;
}

int RemainingTicks(const Plot& plot, int current_tick) {
    if (plot.state != PlotState::Growing) {
        return 0;
    }
    return std::max(0, plot.mature_tick - current_tick);
}

}  // namespace

PlantingSystem::PlantingSystem()
    : plots_(static_cast<std::size_t>(kInitialPlotCount)),
      crop_configs_{
          CropConfig{ItemId::WheatSeed, ItemId::Wheat, kWheatGrowTicks, kWheatSeedPrice,
                     kWheatSellPrice},
          CropConfig{ItemId::CornSeed, ItemId::Corn, kCornGrowTicks, kCornSeedPrice,
                     kCornSellPrice},
          CropConfig{ItemId::CarrotSeed, ItemId::Carrot, kCarrotGrowTicks, kCarrotSeedPrice,
                     kCarrotSellPrice},
      } {
    RefreshPlotViews();
}

void PlantingSystem::Init(int initial_plot_count) {
    if (initial_plot_count < 0) {
        initial_plot_count = 0;
    }
    plots_.assign(static_cast<std::size_t>(initial_plot_count), Plot{});
    last_tick_ = 0;
    RefreshPlotViews();
}

void PlantingSystem::UpdateMaturity(int current_tick) {
    last_tick_ = std::max(last_tick_, current_tick);
    for (Plot& plot : plots_) {
        if (plot.state == PlotState::Growing && current_tick >= plot.mature_tick) {
            plot.state = PlotState::Mature;
        }
    }
}

void PlantingSystem::RefreshPlotViews() const {
    plot_views_.clear();
    plot_views_.reserve(plots_.size());
    for (std::size_t i = 0; i < plots_.size(); ++i) {
        const Plot& plot = plots_[i];
        PlotView view;
        view.plot_id = static_cast<int>(i);
        view.state = plot.state;
        view.water_state = plot.water_state;
        view.crop_id = plot.crop;
        view.planted_tick = plot.planted_tick;
        view.finish_tick = plot.mature_tick;
        view.remaining_ticks = RemainingTicks(plot, last_tick_);
        view.fertilized = plot.fertilized;
        plot_views_.push_back(view);
    }
}

void PlantingSystem::OnTick(int tick_delta, float weather_growth_multiplier) {
    if (tick_delta <= 0) {
        return;
    }

    const int current_tick = last_tick_ + tick_delta;
    const float safe_multiplier = std::max(0.0f, weather_growth_multiplier);
    const int effective_growth = static_cast<int>(static_cast<float>(tick_delta) * safe_multiplier);
    const int bonus_growth = std::max(0, effective_growth - tick_delta);
    if (bonus_growth > 0) {
        for (Plot& plot : plots_) {
            if (plot.state == PlotState::Growing) {
                plot.mature_tick = std::max(current_tick, plot.mature_tick - bonus_growth);
            }
        }
    }

    Tick(current_tick);
}

const std::vector<PlotView>& PlantingSystem::GetPlotsForDisplay() const {
    RefreshPlotViews();
    return plot_views_;
}

PlotView PlantingSystem::GetPlot(int plot_id) const {
    if (plot_id < 0 || plot_id >= static_cast<int>(plots_.size())) {
        return PlotView{};
    }
    RefreshPlotViews();
    return plot_views_[static_cast<std::size_t>(plot_id)];
}

int PlantingSystem::GetMatureCropCount() const {
    int count = 0;
    for (const Plot& plot : plots_) {
        if (plot.state == PlotState::Mature) {
            ++count;
        }
    }
    return count;
}

int PlantingSystem::GetGrowingCount() const {
    int count = 0;
    for (const Plot& plot : plots_) {
        if (plot.state == PlotState::Growing) {
            ++count;
        }
    }
    return count;
}

std::vector<CropProductionEstimate> PlantingSystem::EstimateCropOutput() const {
    std::vector<CropProductionEstimate> estimates;
    estimates.reserve(crop_configs_.size());
    for (const CropConfig& config : crop_configs_) {
        CropProductionEstimate estimate;
        estimate.crop_id = config.crop_id;
        if (config.grow_ticks > 0) {
            estimate.estimated_daily_output =
                static_cast<int>(plots_.size()) * kTicksPerDay / config.grow_ticks;
        }
        estimates.push_back(estimate);
    }

    for (const Plot& plot : plots_) {
        if (plot.state == PlotState::Idle) {
            continue;
        }
        for (CropProductionEstimate& estimate : estimates) {
            if (estimate.crop_id != plot.crop) {
                continue;
            }
            if (plot.state == PlotState::Mature) {
                ++estimate.mature_count;
            } else if (plot.state == PlotState::Growing) {
                ++estimate.growing_count;
            }
        }
    }
    return estimates;
}

Result<void> PlantingSystem::TryPlant(PlayerState& player, ItemId seed_id, int current_tick) {
    if (!ItemCatalog::IsSeed(seed_id)) {
        return Result<void>::failure(ErrorCode::NotASeed);
    }
    if (!player.IsSeedUnlocked(seed_id)) {
        return Result<void>::failure(ErrorCode::SeedNotUnlocked);
    }
    int idle_index = -1;
    for (std::size_t i = 0; i < plots_.size(); ++i) {
        if (plots_[i].state == PlotState::Idle) {
            idle_index = static_cast<int>(i);
            break;
        }
    }
    if (idle_index < 0) {
        return Result<void>::failure(ErrorCode::NoIdlePlot);
    }
    return TryPlantAt(player, idle_index, seed_id, current_tick);
}

Result<void> PlantingSystem::TryPlantAt(PlayerState& player, int plot_id, ItemId seed_id,
                                        int current_tick) {
    if (!ItemCatalog::IsSeed(seed_id)) {
        return Result<void>::failure(ErrorCode::NotASeed);
    }
    if (!player.IsSeedUnlocked(seed_id)) {
        return Result<void>::failure(ErrorCode::SeedNotUnlocked);
    }
    if (plot_id < 0 || plot_id >= static_cast<int>(plots_.size())) {
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    }
    UpdateMaturity(current_tick);

    Plot& p = plots_[static_cast<std::size_t>(plot_id)];
    if (p.state != PlotState::Idle) {
        return Result<void>::failure(ErrorCode::PlotNotIdle);
    }
    if (!player.HasItem(seed_id, 1)) {
        return Result<void>::failure(ErrorCode::InsufficientItem);
    }
    auto removed = player.TryRemoveFromWarehouse(seed_id, 1);
    if (!removed.ok()) {
        return Result<void>::failure(removed.code);
    }

    const ItemId crop = ItemCatalog::CropFromSeed(seed_id);
    const int grow = ItemCatalog::Get(seed_id).grow_ticks;

    p.state = PlotState::Growing;
    p.water_state = PlotWaterState::Dry;
    p.crop = crop;
    p.planted_tick = current_tick;
    p.mature_tick = current_tick + grow;
    p.fertilized = false;
    RefreshPlotViews();
    return Result<void>::success();
}

void PlantingSystem::Tick(int current_tick) {
    UpdateMaturity(current_tick);
    RefreshPlotViews();
}

Result<void> PlantingSystem::WaterPlot(int plot_id, int current_tick) {
    if (plot_id < 0 || plot_id >= static_cast<int>(plots_.size())) {
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    }
    UpdateMaturity(current_tick);

    Plot& p = plots_[static_cast<std::size_t>(plot_id)];
    if (p.state != PlotState::Growing) {
        return Result<void>::failure(ErrorCode::PlotNotGrowing);
    }
    if (p.water_state == PlotWaterState::Watered) {
        return Result<void>::failure(ErrorCode::PlotAlreadyWatered);
    }

    p.water_state = PlotWaterState::Watered;
    p.mature_tick = std::max(current_tick, p.mature_tick - kWaterGrowthBoostTicks);
    UpdateMaturity(current_tick);
    RefreshPlotViews();
    return Result<void>::success();
}

Result<void> PlantingSystem::ApplyFertilizer(PlayerState& player, int plot_id,
                                             ItemId fertilizer_id) {
    if (!IsFertilizer(fertilizer_id)) {
        return Result<void>::failure(ErrorCode::NotFertilizer);
    }
    if (plot_id < 0 || plot_id >= static_cast<int>(plots_.size())) {
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    }
    UpdateMaturity(last_tick_);

    Plot& p = plots_[static_cast<std::size_t>(plot_id)];
    if (p.state != PlotState::Growing) {
        return Result<void>::failure(ErrorCode::PlotNotGrowing);
    }
    if (p.fertilized) {
        return Result<void>::failure(ErrorCode::PlotAlreadyFertilized);
    }
    if (!player.HasItem(fertilizer_id, 1)) {
        return Result<void>::failure(ErrorCode::InsufficientItem);
    }

    auto removed = player.TryRemoveFromWarehouse(fertilizer_id, 1);
    if (!removed.ok()) {
        return Result<void>::failure(removed.code);
    }

    const int remaining = RemainingTicks(p, last_tick_);
    if (remaining > 0) {
        const int new_remaining = std::max(1, (remaining + 1) / 2);
        p.mature_tick = last_tick_ + new_remaining;
    }
    p.fertilized = true;
    RefreshPlotViews();
    return Result<void>::success();
}

Result<void> PlantingSystem::TryHarvest(PlayerState& player, int plot_index) {
    if (plot_index < 0 || plot_index >= static_cast<int>(plots_.size())) {
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    }
    Plot& p = plots_[static_cast<std::size_t>(plot_index)];
    if (p.state == PlotState::Idle || p.state == PlotState::Growing) {
        return Result<void>::failure(ErrorCode::PlotNotMature);
    }
    if (p.state != PlotState::Mature) {
        return Result<void>::failure(ErrorCode::InternalError);
    }

    const ItemId crop = p.crop;
    auto added = player.TryAddToWarehouse(crop, 1);
    if (!added.ok()) {
        return Result<void>::failure(added.code);
    }

    p.state = PlotState::Idle;
    p.water_state = PlotWaterState::Dry;
    p.crop = ItemId::Wheat;
    p.planted_tick = 0;
    p.mature_tick = 0;
    p.fertilized = false;
    RefreshPlotViews();
    return Result<void>::success();
}

Result<int> PlantingSystem::ExpandPlot(PlayerState& player, int player_level) {
    if (static_cast<int>(plots_.size()) >= kMaxPlotCount) {
        return Result<int>::failure(ErrorCode::PlotExpansionLocked);
    }

    const int extra_plots = std::max(0, static_cast<int>(plots_.size()) - kInitialPlotCount);
    const int required_level = extra_plots + 1;
    if (player_level < required_level) {
        return Result<int>::failure(ErrorCode::PlotExpansionLocked);
    }

    const int cost = kPlotExpansionBaseCost + extra_plots * kPlotExpansionCostStep;
    auto spent = player.TrySpendGold(cost);
    if (!spent.ok()) {
        return Result<int>::failure(spent.code);
    }

    plots_.push_back(Plot{});
    RefreshPlotViews();
    return Result<int>::success(static_cast<int>(plots_.size()) - 1);
}

bool PlantingSystem::IsCropUnlocked(ItemId crop_id) const {
    for (const CropConfig& config : crop_configs_) {
        if (config.crop_id == crop_id) {
            return true;
        }
    }
    return false;
}

bool PlantingSystem::IsSeedUnlocked(ItemId seed_id) const {
    for (const CropConfig& config : crop_configs_) {
        if (config.seed_id == seed_id) {
            return true;
        }
    }
    return false;
}

}  // namespace farm
