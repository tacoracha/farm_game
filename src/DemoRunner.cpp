#include "farm/DemoRunner.h"

#include "farm/Game.h"
#include "farm/Types.h"

#include <iostream>

namespace farm {

namespace {

bool LogStep(const char* step_name, const Result<void>& result) {
    if (!result.ok()) {
        std::cerr << "[demo] " << step_name << " failed: " << ErrorCodeMessage(result.code) << " (code "
                  << static_cast<int>(result.code) << ")\n";
        return false;
    }
    return true;
}

}  // namespace

void RunFarmDemo(Game& game) {
    game.WriteStatusSummary(std::cout);

    std::cout << "Farm game demo - tick " << game.CurrentTick() << ", gold "
              << game.Player().Gold() << "\n";

    if (!LogStep("Shop::BuySeed(WheatSeed x1)",
                  game.Shop().BuySeed(game.Player(), ItemId::WheatSeed, 1))) {
        return;
    }

    if (!LogStep("Planting::TryPlant(WheatSeed)",
                  game.Planting().TryPlant(game.Player(), ItemId::WheatSeed, game.CurrentTick()))) {
        return;
    }

    game.AdvanceTick();
    game.AdvanceTick();

    if (!LogStep("Planting::TryHarvest(plot 0)",
                  game.Planting().TryHarvest(game.Player(), 0))) {
        return;
    }

    std::cout << "After harvest - tick " << game.CurrentTick() << ", wheat count "
              << game.Player().GetItemCount(ItemId::Wheat) << "\n";

    game.WriteStatusSummary(std::cout);
}

}  // namespace farm
