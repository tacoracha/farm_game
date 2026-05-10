#pragma once

#include "Constants.h"
#include "Types.h"

#include <vector>

namespace farm {

class PlayerState;

struct Plot {
    PlotState state = PlotState::Idle;
    ItemId crop = ItemId::Wheat;
    int planted_tick = 0;
    int mature_tick = 0;
};

class PlantingSystem {
public:
    PlantingSystem();

    const std::vector<Plot>& Plots() const { return plots_; }

    [[nodiscard]] Result<void> TryPlant(PlayerState& player, ItemId seed_id, int current_tick);
    void Tick(int current_tick);
    [[nodiscard]] Result<void> TryHarvest(PlayerState& player, int plot_index);

private:
    std::vector<Plot> plots_;
};

}  // namespace farm
