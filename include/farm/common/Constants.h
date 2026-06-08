#pragma once

namespace farm {

constexpr int kSaveVersion = 1;
constexpr int kTicksPerDay = 24;
constexpr int kInitialGold = 80;
constexpr int kInitialWarehouseCapacity = 40;
constexpr int kInitialPlotCount = 6;
constexpr int kMaxPlotCount = 18;
constexpr int kInitialWheatSeeds = 4;
constexpr int kInitialCornSeeds = 2;
constexpr int kInitialCarrotSeeds = 2;
constexpr int kInitialFertilizer = 1;
constexpr int kWheatGrowTicks = 3;
constexpr int kCornGrowTicks = 5;
constexpr int kCarrotGrowTicks = 4;
constexpr int kWaterGrowthBoostTicks = 1;
constexpr int kFertilizerCost = 6;
constexpr int kWarehouseUpgradeCost = 70;
constexpr int kWarehouseUpgradeSlots = 20;
constexpr int kPlotExpansionBaseCost = 45;
constexpr int kChickenCoopBaseCapacity = 3;
constexpr int kChickenCost = 25;
constexpr int kChickenEggTicks = 4;
constexpr int kFeedMillQueueCapacity = 3;
constexpr int kFeedMillShelfCapacity = 4;
constexpr int kChickenFeedTicks = 2;
constexpr int kOrderSlotCount = 4;
constexpr int kOrderCompleteCooldownTicks = 4;
constexpr int kOrderAbandonCooldownTicks = 8;
constexpr int kExpPerLevel = 30;
constexpr int kAutoSaveEveryTicks = 12;

}  // namespace farm

