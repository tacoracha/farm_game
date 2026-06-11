#include "farm/common/Constants.h"
#include "farm/core/Game.h"
#include "farm/persistence/SaveManager.h"

#include <cstdio>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
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
                                      game.Time().CurrentTick(), farm::Season::Spring).ok());
    EXPECT_EQ(game.Player().ItemCount(farm::ItemId::WheatSeed), seeds - 1);
    EXPECT_EQ(game.Planting().TryPlantAt(game.Player(), 1, farm::ItemId::CornSeed,
                                         game.Time().CurrentTick(), farm::Season::Spring).code,
              farm::ErrorCode::SeedNotUnlocked);
    EXPECT_EQ(game.Planting().TryPlantAt(game.Player(), 0, farm::ItemId::WheatSeed,
                                         game.Time().CurrentTick(), farm::Season::Spring).code,
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
                                      game.Time().CurrentTick(), farm::Season::Spring).ok());
    game.AdvanceTicks(farm::kWheatGrowTicks);
    FillWarehouse(game.Player());
    EXPECT_EQ(game.Planting().Harvest(game.Player(), 0).code, farm::ErrorCode::WarehouseFull);
    EXPECT_EQ(game.Planting().Plots()[0].state, farm::PlotState::Mature);
}

void TestWorkshopAndRanch() {
    farm::Game game;
    EXPECT(game.Player().TryAddItem(farm::ItemId::Wheat, 4).ok());
    EXPECT(game.Workshop().StartProduction(game.Player(), farm::RecipeId::ChickenFeed, 2,
                                           game.Player().Level()).ok());
    EXPECT_EQ(game.Player().ItemCount(farm::ItemId::Wheat), 0);
    game.AdvanceTicks(farm::kChickenFeedTicks);
    // Auto-collected to warehouse directly
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
    // Advance one tick to initialize orders
    game.AdvanceTicks(1);
    const farm::OrderData order = game.Orders().Orders()[0];
    EXPECT(!order.requirements.empty());
    // Add all required items
    for (const auto& req : order.requirements) {
        EXPECT(game.Player().TryAddItem(req.item, req.quantity).ok());
    }
    const int gold = game.Player().Gold();
    EXPECT(game.Orders().CompleteOrder(game.Player(), 0, game.Time().CurrentTick()).ok());
    EXPECT_EQ(game.Player().Gold(), gold + order.reward_gold);
    EXPECT_EQ(game.Orders().Orders()[0].state, farm::OrderState::CoolingDown);
    game.AdvanceTicks(farm::kOrderCompleteCooldownTicks);
    EXPECT_EQ(game.Orders().Orders()[0].state, farm::OrderState::Available);
    // New order should have different id
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
    game.Player().AddExperience(farm::kExpPerLevel);
    EXPECT(game.Player().UnlockContent(farm::UnlockId::CornSeed).ok());
    EXPECT(game.Player().TryAddItem(farm::ItemId::CornSeed, 1).ok());
    EXPECT(game.Planting().TryPlantAt(game.Player(), 0, farm::ItemId::CornSeed,
                                      game.Time().CurrentTick(), farm::Season::Spring).ok());
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

void TestRandomEventAndOfflineProgress() {
    farm::Game game;
    game.AdvanceTicks(farm::kRandomEventIntervalTicks);
    EXPECT(!game.LastEventMessage().empty());
    EXPECT_EQ(game.LastRandomEventTick(), farm::kRandomEventIntervalTicks);

    const std::string path = SavePath("offline");
    std::remove(path.c_str());
    EXPECT(game.ManualSave(path).ok());

    std::ifstream in(path);
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string text = buffer.str();
    const auto now = std::chrono::system_clock::now();
    const long long old_time =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() -
        farm::kOfflineSecondsPerTick * 3;
    const std::size_t pos = text.find("REALTIME ");
    EXPECT(pos != std::string::npos);
    const std::size_t end = text.find('\n', pos);
    text.replace(pos, end - pos, "REALTIME " + std::to_string(old_time));
    std::ofstream out(path, std::ios::trunc);
    out << text;
    out.close();

    farm::Game loaded;
    EXPECT(loaded.Load(path).ok());
    EXPECT(loaded.Time().CurrentTick() >= game.Time().CurrentTick() + 3);
    std::remove(path.c_str());
}

void TestUnlockDagLandAndAdvancedAnimals() {
    farm::Game game;
    game.Player().AddGold(1000);
    EXPECT(!game.Player().CanUnlock(farm::UnlockId::CarrotSeed));
    game.Player().AddExperience(farm::kExpPerLevel);
    EXPECT(game.Player().CanUnlock(farm::UnlockId::CornSeed));
    EXPECT(game.Player().UnlockContent(farm::UnlockId::CornSeed).ok());
    EXPECT(game.Player().IsSeedUnlocked(farm::ItemId::CornSeed));
    EXPECT_EQ(game.Planting().Expand(game.Player()).code, farm::ErrorCode::ContentLocked);
    EXPECT(game.Player().UnlockContent(farm::UnlockId::ExtraLand).ok());
    EXPECT(game.Planting().Expand(game.Player()).ok());

    game.Player().AddExperience(farm::kExpPerLevel);
    EXPECT(game.Player().UnlockContent(farm::UnlockId::CarrotSeed).ok());
    EXPECT(game.Player().UnlockContent(farm::UnlockId::CowBarn).ok());
    EXPECT(game.Player().UnlockContent(farm::UnlockId::Cow).ok());
    auto barn = game.Ranch().BuildFacility(game.Player(), farm::RanchFacilityKind::CowBarn);
    EXPECT(barn.ok());
    auto cow = game.Ranch().BuyAnimal(game.Player(), barn.value, farm::AnimalKind::Cow);
    EXPECT(cow.ok());
    EXPECT(game.Player().TryAddItem(farm::ItemId::Corn, 2).ok());
    EXPECT(game.Player().TryAddItem(farm::ItemId::Carrot, 1).ok());
    EXPECT(game.Workshop().StartProduction(game.Player(), farm::RecipeId::CowFeed, 1,
                                           game.Player().Level()).ok());
    game.AdvanceTicks(farm::kCowFeedTicks);
    // Auto-collected to warehouse
    EXPECT_EQ(game.Player().ItemCount(farm::ItemId::CowFeed), 1);
    EXPECT(game.Ranch().FeedAnimal(game.Player(), barn.value, cow.value, game.Time().CurrentTick()).ok());
    game.AdvanceTicks(farm::kCowMilkTicks);
    EXPECT(game.Ranch().HarvestAnimal(game.Player(), barn.value, cow.value).ok());
    EXPECT_EQ(game.Player().ItemCount(farm::ItemId::Milk), 1);
}

void TestFullLoop() {
    farm::Game game;
    EXPECT(game.Shop().BuyItem(game.Player(), farm::ItemId::WheatSeed, 2).ok());
    for (int i = 0; i < 2; ++i) {
        EXPECT(game.Planting().TryPlant(game.Player(), farm::ItemId::WheatSeed,
                                        game.Time().CurrentTick(), farm::Season::Spring).ok());
    }
    game.AdvanceTicks(farm::kWheatGrowTicks);
    EXPECT(game.Planting().Harvest(game.Player(), 0).ok());
    EXPECT(game.Planting().Harvest(game.Player(), 1).ok());
    EXPECT(game.Workshop().StartProduction(game.Player(), farm::RecipeId::ChickenFeed, 1,
                                           game.Player().Level()).ok());
    game.AdvanceTicks(farm::kChickenFeedTicks);
    // Auto-collected to warehouse
    EXPECT_EQ(game.Player().ItemCount(farm::ItemId::ChickenFeed), 1);
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
    TestRandomEventAndOfflineProgress();
    TestUnlockDagLandAndAdvancedAnimals();
    TestFullLoop();

    if (g_failures != 0) {
        std::cerr << g_failures << " assertion(s) failed\n";
        return 1;
    }
    std::cout << "All farm_game core tests passed.\n";
    return 0;
}
