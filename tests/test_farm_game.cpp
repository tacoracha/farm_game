#include "farm/common/Constants.h"
#include "farm/core/Game.h"
#include "farm/persistence/SaveManager.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void Expect(bool ok, const char* expr, int line) {
    if (!ok) {
        std::cerr << "line " << line << " failed: " << expr << "\n";
        ++g_failures;
    }
}

#define EXPECT(x) Expect(static_cast<bool>(x), #x, __LINE__)
#define EXPECT_EQ(a, b) Expect((a) == (b), #a " == " #b, __LINE__)

std::string SavePath(const char* name) {
    return std::string("test_") + name + ".sav";
}

void FillWarehouse(farm::PlayerState& player) {
    while (player.WarehouseRemaining() > 0) {
        auto added = player.TryAddItem(farm::ItemId::Wheat, 1);
        if (!added.ok()) {
            break;
        }
    }
}

void TestInventoryAndEconomy() {
    farm::PlayerState player;
    const int gold = player.Gold();
    EXPECT(player.TryAddItem(farm::ItemId::Wheat, 2).ok());
    EXPECT_EQ(player.ItemCount(farm::ItemId::Wheat), 2);
    EXPECT(player.TryRemoveItem(farm::ItemId::Wheat, 1).ok());
    EXPECT_EQ(player.ItemCount(farm::ItemId::Wheat), 1);
    EXPECT(!player.TryRemoveItem(farm::ItemId::Wheat, 9).ok());
    player.SetItemLocked(farm::ItemId::Wheat, true);
    EXPECT_EQ(player.TrySellItem(farm::ItemId::Wheat, 1).code, farm::ErrorCode::ProtectedItem);
    player.SetItemLocked(farm::ItemId::Wheat, false);
    EXPECT(player.TrySellItem(farm::ItemId::Wheat, 1).ok());
    EXPECT(player.Gold() > gold);
    EXPECT(player.TrySpendGold(player.Gold()).ok());
    EXPECT_EQ(player.TrySpendGold(1).code, farm::ErrorCode::InsufficientGold);
}

void TestShopNoChangeOnFailure() {
    farm::Game game;
    game.Player().TrySpendGold(game.Player().Gold());
    const int seeds = game.Player().ItemCount(farm::ItemId::WheatSeed);
    EXPECT_EQ(game.Shop().BuyItem(game.Player(), farm::ItemId::WheatSeed, 1).code,
              farm::ErrorCode::InsufficientGold);
    EXPECT_EQ(game.Player().ItemCount(farm::ItemId::WheatSeed), seeds);
}

void TestPlanting() {
    farm::Game game;
    const int seeds = game.Player().ItemCount(farm::ItemId::WheatSeed);
    EXPECT(game.Planting().TryPlantAt(game.Player(), 0, farm::ItemId::WheatSeed,
                                      game.Time().CurrentTick()).ok());
    EXPECT_EQ(game.Player().ItemCount(farm::ItemId::WheatSeed), seeds - 1);
    EXPECT_EQ(game.Planting().TryPlantAt(game.Player(), 0, farm::ItemId::CornSeed,
                                         game.Time().CurrentTick()).code,
              farm::ErrorCode::PlotNotIdle);
    EXPECT(game.Planting().WaterPlot(0, game.Time().CurrentTick()).ok());
    EXPECT_EQ(game.Planting().WaterPlot(0, game.Time().CurrentTick()).code,
              farm::ErrorCode::PlotAlreadyWatered);
    EXPECT(game.Planting().ApplyFertilizer(game.Player(), 0, game.Time().CurrentTick()).ok());
    EXPECT_EQ(game.Planting().ApplyFertilizer(game.Player(), 0, game.Time().CurrentTick()).code,
              farm::ErrorCode::PlotAlreadyFertilized);
    game.AdvanceTicks(farm::kWheatGrowTicks);
    EXPECT_EQ(game.Planting().Plots()[0].state, farm::PlotState::Mature);
    EXPECT(game.Planting().Harvest(game.Player(), 0).ok());
    EXPECT(game.Player().ItemCount(farm::ItemId::Wheat) >= 1);
}

void TestHarvestWarehouseFullKeepsCrop() {
    farm::Game game;
    EXPECT(game.Planting().TryPlantAt(game.Player(), 0, farm::ItemId::WheatSeed,
                                      game.Time().CurrentTick()).ok());
    game.AdvanceTicks(farm::kWheatGrowTicks);
    FillWarehouse(game.Player());
    EXPECT_EQ(game.Planting().Harvest(game.Player(), 0).code, farm::ErrorCode::WarehouseFull);
    EXPECT_EQ(game.Planting().Plots()[0].state, farm::PlotState::Mature);
}

void TestWorkshopAndRanch() {
    farm::Game game;
    EXPECT(game.Player().TryAddItem(farm::ItemId::Wheat, 4).ok());
    EXPECT(game.Workshop().StartProduction(game.Player(), farm::RecipeId::ChickenFeed, 2).ok());
    EXPECT_EQ(game.Player().ItemCount(farm::ItemId::Wheat), 0);
    game.AdvanceTicks(farm::kChickenFeedTicks);
    EXPECT_EQ(game.Workshop().ShelfChickenFeed(), 1);
    EXPECT(game.Workshop().ClaimProduct(game.Player()).ok());
    EXPECT_EQ(game.Player().ItemCount(farm::ItemId::ChickenFeed), 1);

    auto chicken = game.Ranch().BuyAnimal(game.Player(), 1, farm::AnimalKind::Chicken);
    EXPECT(chicken.ok());
    EXPECT(game.Ranch().FeedAnimal(game.Player(), 1, chicken.value, game.Time().CurrentTick()).ok());
    game.AdvanceTicks(farm::kChickenEggTicks);
    EXPECT(game.Ranch().HarvestAnimal(game.Player(), 1, chicken.value).ok());
    EXPECT_EQ(game.Player().ItemCount(farm::ItemId::Egg), 1);
}

void TestOrderCooldownAndReward() {
    farm::Game game;
    const farm::OrderData order = game.Orders().Orders()[0];
    EXPECT(game.Player().TryAddItem(order.item, order.quantity).ok());
    const int gold = game.Player().Gold();
    EXPECT(game.Orders().CompleteOrder(game.Player(), 0, game.Time().CurrentTick()).ok());
    EXPECT_EQ(game.Player().Gold(), gold + order.reward_gold);
    EXPECT_EQ(game.Orders().Orders()[0].state, farm::OrderState::CoolingDown);
    game.AdvanceTicks(farm::kOrderCompleteCooldownTicks);
    EXPECT_EQ(game.Orders().Orders()[0].state, farm::OrderState::Available);
    EXPECT(game.Orders().Orders()[0].id != order.id);
}

void TestTimeWeatherPause() {
    farm::Game game;
    const int tick = game.Time().CurrentTick();
    game.Time().Pause();
    EXPECT_EQ(game.AdvanceBySpeed().code, farm::ErrorCode::GamePaused);
    EXPECT_EQ(game.Time().CurrentTick(), tick);
    game.Time().SetSpeed(farm::GameSpeed::VeryFast);
    EXPECT(game.AdvanceBySpeed().ok());
    EXPECT_EQ(game.Time().CurrentTick(), tick + 4);
    game.Weather().SetForLoad(farm::WeatherType::Rainy, 2);
    game.AdvanceTicks(2);
    EXPECT(game.Weather().Snapshot().remaining_ticks > 0);
}

void TestSaveRoundTripAndErrors() {
    const std::string path = SavePath("roundtrip");
    std::remove(path.c_str());
    farm::Game game;
    EXPECT(game.Player().TryAddItem(farm::ItemId::Wheat, 3).ok());
    EXPECT(game.Planting().TryPlantAt(game.Player(), 0, farm::ItemId::CornSeed,
                                      game.Time().CurrentTick()).ok());
    game.AdvanceTicks(2);
    EXPECT(game.ManualSave(path).ok());

    farm::Game loaded;
    EXPECT(loaded.Load(path).ok());
    EXPECT_EQ(loaded.Time().CurrentTick(), game.Time().CurrentTick());
    EXPECT_EQ(loaded.Player().ItemCount(farm::ItemId::Wheat),
              game.Player().ItemCount(farm::ItemId::Wheat));
    EXPECT_EQ(loaded.Planting().Plots()[0].crop, farm::ItemId::Corn);

    farm::Game before;
    const int original_gold = before.Player().Gold();
    EXPECT_EQ(before.Load("missing_save_file.sav").code, farm::ErrorCode::SaveOpenFailed);
    EXPECT_EQ(before.Player().Gold(), original_gold);

    const std::string broken = SavePath("broken");
    {
        std::ofstream out(broken);
        out << "not a save";
    }
    EXPECT_EQ(before.Load(broken).code, farm::ErrorCode::SaveCorrupted);
    EXPECT_EQ(before.Player().Gold(), original_gold);

    const std::string wrong_version = SavePath("wrong_version");
    {
        std::ofstream out(wrong_version);
        out << "FARM_SAVE 999\n";
    }
    EXPECT_EQ(before.Load(wrong_version).code, farm::ErrorCode::SaveVersionMismatch);
    std::remove(path.c_str());
    std::remove(broken.c_str());
    std::remove(wrong_version.c_str());
}

void TestFullLoop() {
    farm::Game game;
    EXPECT(game.Shop().BuyItem(game.Player(), farm::ItemId::WheatSeed, 2).ok());
    for (int i = 0; i < 2; ++i) {
        EXPECT(game.Planting().TryPlant(game.Player(), farm::ItemId::WheatSeed,
                                        game.Time().CurrentTick()).ok());
    }
    game.AdvanceTicks(farm::kWheatGrowTicks);
    EXPECT(game.Planting().Harvest(game.Player(), 0).ok());
    EXPECT(game.Planting().Harvest(game.Player(), 1).ok());
    EXPECT(game.Workshop().StartProduction(game.Player(), farm::RecipeId::ChickenFeed, 1).ok());
    game.AdvanceTicks(farm::kChickenFeedTicks);
    EXPECT(game.Workshop().ClaimProduct(game.Player()).ok());
    auto chicken = game.Ranch().BuyAnimal(game.Player(), 1, farm::AnimalKind::Chicken);
    EXPECT(chicken.ok());
    EXPECT(game.Ranch().FeedAnimal(game.Player(), 1, chicken.value, game.Time().CurrentTick()).ok());
    game.AdvanceTicks(farm::kChickenEggTicks);
    EXPECT(game.Ranch().HarvestAnimal(game.Player(), 1, chicken.value).ok());
    EXPECT(game.Player().ItemCount(farm::ItemId::Egg) > 0);
}

}  // namespace

int main() {
    TestInventoryAndEconomy();
    TestShopNoChangeOnFailure();
    TestPlanting();
    TestHarvestWarehouseFullKeepsCrop();
    TestWorkshopAndRanch();
    TestOrderCooldownAndReward();
    TestTimeWeatherPause();
    TestSaveRoundTripAndErrors();
    TestFullLoop();

    if (g_failures != 0) {
        std::cerr << g_failures << " assertion(s) failed\n";
        return 1;
    }
    std::cout << "All farm_game core tests passed.\n";
    return 0;
}
