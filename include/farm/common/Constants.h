#pragma once

namespace farm {

constexpr int kSaveVersion = 2;
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
constexpr int kWarehouseUpgradeCost = 70;
constexpr int kWarehouseUpgradeSlots = 20;
constexpr int kPlotExpansionBaseCost = 60;
constexpr int kChickenCoopBaseCapacity = 3;
constexpr int kBarnBaseCapacity = 2;
constexpr int kChickenCost = 25;
constexpr int kCowCost = 80;
constexpr int kSheepCost = 95;
constexpr int kChickenEggTicks = 30;
constexpr int kCowMilkTicks = 60;
constexpr int kSheepWoolTicks = 90;
constexpr int kFeedMillQueueCapacity = 3;
constexpr int kFeedMillShelfCapacity = 4;
constexpr int kChickenFeedTicks = 10;
constexpr int kCowFeedTicks = 16;
constexpr int kOrderSlotCount = 4;
constexpr int kOrderCompleteCooldownTicks = 4;
constexpr int kOrderAbandonCooldownTicks = 8;
constexpr int kExpPerLevel = 30;
constexpr int kAutoSaveEveryTicks = 12;
constexpr int kRandomEventIntervalTicks = 90;
constexpr int kOfflineSecondsPerTick = 1;
constexpr int kMaxOfflineTicks = 48;

}  // namespace farm
