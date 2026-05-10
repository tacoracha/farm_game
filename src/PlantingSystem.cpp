#include "farm/PlantingSystem.h"

#include "farm/ItemCatalog.h"
#include "farm/PlayerState.h"

namespace farm {

PlantingSystem::PlantingSystem() : plots_(static_cast<std::size_t>(kInitialPlotCount)) {}

Result<void> PlantingSystem::TryPlant(PlayerState& player, ItemId seed_id, int current_tick) {
    if (!ItemCatalog::IsSeed(seed_id)) {
        return Result<void>::failure(ErrorCode::NotASeed);
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
    if (!player.HasItem(seed_id, 1)) {
        return Result<void>::failure(ErrorCode::InsufficientItem);
    }
    auto removed = player.TryRemoveFromWarehouse(seed_id, 1);
    if (!removed.ok()) {
        return Result<void>::failure(removed.code);
    }

    const ItemId crop = ItemCatalog::CropFromSeed(seed_id);
    const int grow = ItemCatalog::Get(seed_id).grow_ticks;

    Plot& p = plots_[static_cast<std::size_t>(idle_index)];
    p.state = PlotState::Growing;
    p.crop = crop;
    p.planted_tick = current_tick;
    p.mature_tick = current_tick + grow;
    return Result<void>::success();
}

void PlantingSystem::Tick(int current_tick) {
    for (Plot& p : plots_) {
        if (p.state == PlotState::Growing && current_tick >= p.mature_tick) {
            p.state = PlotState::Mature;
        }
    }
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
    p.crop = ItemId::Wheat;
    p.planted_tick = 0;
    p.mature_tick = 0;
    return Result<void>::success();
}

}  // namespace farm
