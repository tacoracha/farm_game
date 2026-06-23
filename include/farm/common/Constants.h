#pragma once

namespace farm {

constexpr int kSaveVersion = 3;
constexpr int kGameMinutesPerTick = 2;
constexpr int kTicksPerHour = 60 / kGameMinutesPerTick;
constexpr int kTicksPerDay = 24 * kTicksPerHour;
constexpr int kInitialGold = 120;
constexpr int kInitialWarehouseCapacity = 40;
constexpr int kInitialPlotCount = 6;
constexpr int kMaxPlotCount = 18;
constexpr int kInitialWheatSeeds = 4;
constexpr int kInitialCornSeeds = 0;
constexpr int kInitialCarrotSeeds = 0;
constexpr int kInitialFertilizer = 1;
constexpr int kWheatGrowTicks = 15;
constexpr int kCornGrowTicks = 30;
constexpr int kCarrotGrowTicks = 22;
constexpr int kTomatoGrowTicks = 40;
constexpr int kWaterGrowthBoostTicks = 3;
constexpr int kFertilizerCost = 6;
constexpr int kWarehouseMaxLevel = 5;
inline constexpr int kWarehouseCapacities[] = {40, 60, 80, 100, 150};
inline constexpr int kWarehouseUpgradeCosts[] = {0, 70, 120, 180, 250};
constexpr int kPlotExpansionBaseCost = 60;
constexpr int kChickenCoopBaseCapacity = 3;
constexpr int kBarnBaseCapacity = 2;
constexpr int kChickenCost = 25;
constexpr int kCowCost = 80;
constexpr int kSheepCost = 95;
constexpr int kChickenEggTicks = 30;
constexpr int kCowMilkTicks = 60;
constexpr int kSheepWoolTicks = 90;
constexpr int kFeedMillQueueCapacity = 5;
constexpr int kFeedMillShelfCapacity = 8;
constexpr int kChickenFeedTicks = 10;
constexpr int kCowFeedTicks = 16;
constexpr int kBreadTicks = 15;
constexpr int kCheeseTicks = 25;
constexpr int kJamTicks = 20;
constexpr int kOrderSlotCount = 4;
constexpr int kOrderCompleteCooldownTicks = 4;
constexpr int kOrderAbandonCooldownTicks = 8;
constexpr int kExpPerLevel = 30;
constexpr int kAutoSaveEveryTicks = 12;
constexpr int kRandomEventIntervalTicks = 90;
constexpr int kOfflineSecondsPerTick = 1;
constexpr int kMaxOfflineTicks = 48;

// Animal mood & breeding
constexpr int kMoodFedValue = 100;
constexpr int kMoodDropAfterTicks = 48;     // 2 days without food → mood drops
constexpr int kMoodHighThreshold = 80;      // +20% production above this
constexpr int kMoodLowThreshold = 30;       // -30% production below this
constexpr int kMoodDropPerTick = 2;
constexpr int kBreedingIntervalTicks = 72;  // check breeding every 72 ticks
constexpr int kBreedingChancePercent = 30;
constexpr int kBabyGrowTicks = 36;          // baby grows up in 36 ticks
constexpr int kChickenLifespan = 360;       // 5 game days
constexpr int kCowLifespan = 720;            // 10 game days
constexpr int kSheepLifespan = 720;

// Season system
constexpr int kSeasonDurationTicks = 288;  // 4 game days per season
constexpr int kStrawberryGrowTicks = 50;
constexpr int kPumpkinGrowTicks = 55;
constexpr int kMushroomGrowTicks = 35;
constexpr int kStrawberrySeedBuyPrice = 10;
constexpr int kPumpkinSeedBuyPrice = 12;
constexpr int kMushroomSeedBuyPrice = 15;
constexpr int kStrawberrySellPrice = 25;
constexpr int kPumpkinSellPrice = 30;
constexpr int kMushroomSellPrice = 40;
constexpr int kGreenhouseMaxPlots = 6;
constexpr int kGreenhouseBuildCost = 60;
constexpr float kGreenhouseGrowthBonus = 1.10f;

}  // namespace farm
