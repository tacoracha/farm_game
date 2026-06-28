#include "farm/planting/PlantingSystem.h"

#include "farm/achievement/AchievementSystem.h"
#include "farm/common/Constants.h"
#include "farm/dailytask/DailyTaskSystem.h"
#include "farm/core/UnlockGraph.h"
#include "farm/inventory/PlayerState.h"

#include <algorithm>
#include <cmath>

namespace farm {

PlantingSystem::PlantingSystem()
    : plots_(static_cast<std::size_t>(kInitialPlotCount)) {}

void PlantingSystem::Setup(AchievementSystem* ach, DailyTaskSystem* dts) {
    ach_ = ach;
    dts_ = dts;
}

std::vector<PlotView> PlantingSystem::GreenhouseView(int current_tick) const {
    std::vector<PlotView> view;
    for (std::size_t i = 0; i < greenhouse_plots_.size(); ++i) {
        const PlotData& plot = greenhouse_plots_[i];
        const int remaining = plot.state == PlotState::Growing
                                  ? std::max(0, plot.mature_tick - current_tick) : 0;
        view.push_back(PlotView{static_cast<int>(i), plot.state, plot.water, plot.crop, remaining, plot.fertilized});
    }
    return view;
}

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

bool PlantingSystem::CanPlantInSeason(ItemId seed, Season season) {
    switch (seed) {
        case ItemId::WheatSeed: case ItemId::CornSeed: case ItemId::CarrotSeed:
            return season != Season::Winter;
        case ItemId::StrawberrySeed: return season == Season::Spring;
        case ItemId::TomatoSeed:     return season == Season::Summer;
        case ItemId::PumpkinSeed:    return season == Season::Autumn;
        case ItemId::MushroomSeed:   return season == Season::Winter;
        default: return true;
    }
}

Result<void> PlantingSystem::TryPlant(PlayerState& player, ItemId seed, int current_tick, Season season) {
    for (std::size_t i = 0; i < plots_.size(); ++i) {
        if (plots_[i].state == PlotState::Idle) {
            return TryPlantAt(player, static_cast<int>(i), seed, current_tick, season);
        }
    }
    return Result<void>::failure(ErrorCode::NoIdlePlot);
}

Result<void> PlantingSystem::TryPlantAt(PlayerState& player, int plot_id, ItemId seed,
                                        int current_tick, Season season) {
    if (!IsSeed(seed)) {
        return Result<void>::failure(ErrorCode::NotASeed);
    }
    if (!player.IsSeedUnlocked(seed)) {
        return Result<void>::failure(ErrorCode::SeedNotUnlocked);
    }
    if (!CanPlantInSeason(seed, season)) {
        return Result<void>::failure(ErrorCode::ContentLocked);
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
    int boost = summer_drought_ ? kWaterGrowthBoostTicks * 2 : kWaterGrowthBoostTicks;
    plot.mature_tick = std::max(current_tick, plot.mature_tick - boost);
    UpdateMaturity(current_tick);
    if (dts_) dts_->OnWaterCrop(1);
    return Result<void>::success();
}

void PlantingSystem::SetSummerDrought(bool active) { summer_drought_ = active; }
void PlantingSystem::SetAutumnDouble(bool active) { autumn_double_ = active; }

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
    if (dts_) dts_->OnFertilizeCrop(1);
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
    int qty = 1;
    // Autumn: 10% chance double harvest
    if (autumn_double_ && (plot_id * 17 + plot.mature_tick) % 10 == 0) qty = 2;
    auto added = player.TryAddItem(plot.crop, qty);
    if (!added.ok()) {
        // Try single if double fails (warehouse might be near full)
        if (qty > 1) added = player.TryAddItem(plot.crop, 1);
        if (!added.ok()) return added;
    }
    ItemId harvested_crop = plot.crop;
    plot = PlotData{};
    if (ach_) ach_->OnHarvestCrop(harvested_crop);
    if (dts_) dts_->OnHarvestCrop(1);
    return Result<void>::success();
}

Result<int> PlantingSystem::BatchPlant(PlayerState& player, ItemId seed, int current_tick, Season season) {
    if (!IsSeed(seed)) {
        return Result<int>::failure(ErrorCode::NotASeed);
    }
    if (!player.IsSeedUnlocked(seed)) {
        return Result<int>::failure(ErrorCode::SeedNotUnlocked);
    }
    int count = 0;
    for (std::size_t i = 0; i < plots_.size(); ++i) {
        if (plots_[i].state != PlotState::Idle) continue;
        if (!player.HasItem(seed, 1)) break;
        auto result = TryPlantAt(player, static_cast<int>(i), seed, current_tick, season);
        if (result.ok()) {
            ++count;
        } else if (result.code == ErrorCode::InsufficientItem) {
            break;
        }
    }
    return Result<int>::success(count);
}

int PlantingSystem::WitherNonSeasonal(Season new_season) {
    int withered = 0;
    for (PlotData& plot : plots_) {
        if (plot.state != PlotState::Growing && plot.state != PlotState::Mature) continue;
        ItemId seed = ItemId::WheatSeed;
        switch (plot.crop) {
            case ItemId::Wheat:      seed = ItemId::WheatSeed;      break;
            case ItemId::Corn:       seed = ItemId::CornSeed;       break;
            case ItemId::Carrot:     seed = ItemId::CarrotSeed;     break;
            case ItemId::Tomato:     seed = ItemId::TomatoSeed;     break;
            case ItemId::Strawberry: seed = ItemId::StrawberrySeed; break;
            case ItemId::Pumpkin:    seed = ItemId::PumpkinSeed;    break;
            case ItemId::Mushroom:   seed = ItemId::MushroomSeed;   break;
            default: break;
        }
        if (!CanPlantInSeason(seed, new_season)) {
            plot = PlotData{};
            ++withered;
        }
    }
    return withered;
}

Result<int> PlantingSystem::BatchWater(int current_tick) {
    int count = 0;
    for (std::size_t i = 0; i < plots_.size(); ++i) {
        const PlotData& plot = plots_[i];
        if (plot.state != PlotState::Growing) continue;
        if (plot.water == PlotWaterState::Watered) continue;
        auto result = WaterPlot(static_cast<int>(i), current_tick);
        if (result.ok()) ++count;
    }
    return Result<int>::success(count);
}

Result<int> PlantingSystem::BatchFertilize(PlayerState& player, int current_tick) {
    int count = 0;
    for (std::size_t i = 0; i < plots_.size(); ++i) {
        const PlotData& plot = plots_[i];
        if (plot.state != PlotState::Growing) continue;
        if (plot.fertilized) continue;
        if (!player.HasItem(ItemId::Fertilizer, 1)) break;
        auto result = ApplyFertilizer(player, static_cast<int>(i), current_tick);
        if (result.ok()) {
            ++count;
        } else if (result.code == ErrorCode::InsufficientItem) {
            break;
        }
    }
    return Result<int>::success(count);
}

Result<int> PlantingSystem::BatchHarvest(PlayerState& player) {
    int count = 0;
    for (std::size_t i = 0; i < plots_.size(); ++i) {
        if (plots_[i].state != PlotState::Mature) continue;
        auto result = Harvest(player, static_cast<int>(i));
        if (result.ok()) {
            ++count;
        } else if (result.code == ErrorCode::WarehouseFull) {
            break;
        }
    }
    return Result<int>::success(count);
}

Result<int> PlantingSystem::BuildGreenhouse(PlayerState& player) {
    if (!player.IsUnlocked(UnlockId::GreenhouseTech)) {
        return Result<int>::failure(ErrorCode::ContentLocked);
    }
    if (static_cast<int>(greenhouse_plots_.size()) >= kGreenhouseMaxPlots) {
        return Result<int>::failure(ErrorCode::PlotOutOfRange);
    }
    auto spent = player.TrySpendGold(kGreenhouseBuildCost);
    if (!spent.ok()) {
        return Result<int>::failure(spent.code);
    }
    greenhouse_plots_.push_back(PlotData{});
    return Result<int>::success(static_cast<int>(greenhouse_plots_.size()) - 1);
}

// Greenhouse planting - no season check, +10% base speed
static Result<void> PlantGreenhouseAt(PlayerState& player, std::vector<PlotData>& plots,
                                       int plot_id, ItemId seed, int current_tick) {
    if (!IsSeed(seed)) return Result<void>::failure(ErrorCode::NotASeed);
    if (!player.IsSeedUnlocked(seed)) return Result<void>::failure(ErrorCode::SeedNotUnlocked);
    if (plot_id < 0 || plot_id >= static_cast<int>(plots.size()))
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    PlotData& plot = plots[static_cast<std::size_t>(plot_id)];
    if (plot.state != PlotState::Idle) return Result<void>::failure(ErrorCode::PlotNotIdle);
    auto removed = player.TryRemoveItem(seed, 1);
    if (!removed.ok()) return removed;
    const ItemInfo& info = GetItemInfo(seed);
    plot.state = PlotState::Growing;
    plot.water = PlotWaterState::Dry;
    plot.crop = CropFromSeed(seed);
    plot.planted_tick = current_tick;
    plot.mature_tick = current_tick + static_cast<int>(info.grow_ticks / kGreenhouseGrowthBonus);
    plot.fertilized = false;
    plot.growth_remainder = 0.0f;
    return Result<void>::success();
}

Result<int> PlantingSystem::Expand(PlayerState& player) {
    if (!player.IsUnlocked(UnlockId::ExtraLand)) {
        return Result<int>::failure(ErrorCode::ContentLocked);
    }
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

// ---------- Greenhouse operations ----------
Result<void> PlantingSystem::TryPlantGreenhouse(PlayerState& player, ItemId seed, int current_tick) {
    for (std::size_t i = 0; i < greenhouse_plots_.size(); ++i) {
        if (greenhouse_plots_[i].state == PlotState::Idle)
            return TryPlantGreenhouseAt(player, static_cast<int>(i), seed, current_tick);
    }
    return Result<void>::failure(ErrorCode::NoIdlePlot);
}
Result<void> PlantingSystem::TryPlantGreenhouseAt(PlayerState& player, int plot_id, ItemId seed, int current_tick) {
    return PlantGreenhouseAt(player, greenhouse_plots_, plot_id, seed, current_tick);
}
Result<void> PlantingSystem::WaterGreenhousePlot(int plot_id, int current_tick) {
    if (plot_id < 0 || plot_id >= static_cast<int>(greenhouse_plots_.size()))
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    PlotData& plot = greenhouse_plots_[static_cast<std::size_t>(plot_id)];
    if (plot.state != PlotState::Growing) return Result<void>::failure(ErrorCode::PlotNotGrowing);
    if (plot.water == PlotWaterState::Watered) return Result<void>::failure(ErrorCode::PlotAlreadyWatered);
    plot.water = PlotWaterState::Watered;
    plot.mature_tick = std::max(current_tick, plot.mature_tick - kWaterGrowthBoostTicks);
    if (dts_) dts_->OnWaterCrop(1);
    return Result<void>::success();
}
Result<void> PlantingSystem::HarvestGreenhouse(PlayerState& player, int plot_id) {
    if (plot_id < 0 || plot_id >= static_cast<int>(greenhouse_plots_.size()))
        return Result<void>::failure(ErrorCode::PlotOutOfRange);
    PlotData& plot = greenhouse_plots_[static_cast<std::size_t>(plot_id)];
    if (plot.state != PlotState::Mature) return Result<void>::failure(ErrorCode::PlotNotMature);
    auto added = player.TryAddItem(plot.crop, 1);
    if (!added.ok()) return added;
    plot = PlotData{};
    return Result<void>::success();
}
Result<int> PlantingSystem::BatchPlantGreenhouse(PlayerState& player, ItemId seed, int current_tick) {
    if (!IsSeed(seed)) return Result<int>::failure(ErrorCode::NotASeed);
    int count = 0;
    for (std::size_t i = 0; i < greenhouse_plots_.size(); ++i) {
        if (greenhouse_plots_[i].state != PlotState::Idle) continue;
        if (!player.HasItem(seed, 1)) break;
        auto r = PlantGreenhouseAt(player, greenhouse_plots_, static_cast<int>(i), seed, current_tick);
        if (r.ok()) ++count; else if (r.code == ErrorCode::InsufficientItem) break;
    }
    return Result<int>::success(count);
}
Result<int> PlantingSystem::BatchWaterGreenhouse(int current_tick) {
    int count = 0;
    for (std::size_t i = 0; i < greenhouse_plots_.size(); ++i) {
        auto r = WaterGreenhousePlot(static_cast<int>(i), current_tick);
        if (r.ok()) ++count;
    }
    return Result<int>::success(count);
}
Result<int> PlantingSystem::BatchHarvestGreenhouse(PlayerState& player) {
    int count = 0;
    for (std::size_t i = 0; i < greenhouse_plots_.size(); ++i) {
        auto r = HarvestGreenhouse(player, static_cast<int>(i));
        if (r.ok()) ++count; else if (r.code == ErrorCode::WarehouseFull) break;
    }
    return Result<int>::success(count);
}

void PlantingSystem::Tick(int current_tick, float weather_growth_multiplier) {
    const float multiplier = std::max(0.1f, weather_growth_multiplier);
    for (PlotData& plot : plots_) {
        if (plot.state != PlotState::Growing) continue;
        plot.growth_remainder += multiplier - 1.0f;
        while (plot.growth_remainder >= 1.0f) { --plot.mature_tick; plot.growth_remainder -= 1.0f; }
        while (plot.growth_remainder <= -1.0f) { ++plot.mature_tick; plot.growth_remainder += 1.0f; }
    }
    const float gh_mult = std::max(0.1f, kGreenhouseGrowthBonus);
    for (PlotData& plot : greenhouse_plots_) {
        if (plot.state != PlotState::Growing) continue;
        plot.growth_remainder += gh_mult - 1.0f;
        while (plot.growth_remainder >= 1.0f) { --plot.mature_tick; plot.growth_remainder -= 1.0f; }
    }
    UpdateMaturity(current_tick);
}

void PlantingSystem::ClearForLoad() { plots_.clear(); greenhouse_plots_.clear(); }

void PlantingSystem::SetPlotsForLoad(const std::vector<PlotData>& plots) { plots_ = plots; }
void PlantingSystem::SetGreenhouseForLoad(const std::vector<PlotData>& plots) { greenhouse_plots_ = plots; }

void PlantingSystem::UpdateMaturity(int current_tick) {
    UpdateMaturity(plots_, current_tick);
    UpdateMaturity(greenhouse_plots_, current_tick);
}

void PlantingSystem::UpdateMaturity(std::vector<PlotData>& plots, int current_tick) {
    for (PlotData& plot : plots) {
        if (plot.state == PlotState::Growing && current_tick >= plot.mature_tick) {
            plot.state = PlotState::Mature;
        }
    }
}

}  // namespace farm
