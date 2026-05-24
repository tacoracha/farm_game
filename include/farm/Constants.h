#pragma once

#include "Types.h"

namespace farm {

inline constexpr int kInitialGold = 100;
inline constexpr int kWarehouseCapacity = 30;
inline constexpr int kInitialPlotCount = 3;

inline constexpr int kWheatGrowTicks = 2;
inline constexpr int kCornGrowTicks = 4;
inline constexpr int kCarrotGrowTicks = 6;

inline constexpr int kWheatSellPrice = 8;
inline constexpr int kCornSellPrice = 14;
inline constexpr int kCarrotSellPrice = 20;

inline constexpr int kWheatSeedPrice = 5;
inline constexpr int kCornSeedPrice = 8;
inline constexpr int kCarrotSeedPrice = 12;

inline constexpr int kInitialWheatSeedCount = 3;
inline constexpr int kInitialCornSeedCount = 2;
inline constexpr int kInitialCarrotSeedCount = 1;
inline constexpr int kInitialFertilizerCount = 0;

inline constexpr int kWaterGrowthBoostTicks = 1;
inline constexpr int kPlotExpansionBaseCost = 40;
inline constexpr int kPlotExpansionCostStep = 20;
inline constexpr int kMaxPlotCount = 9;
inline constexpr int kTicksPerDay = 24;

// Order board: fixed slots, single-item orders (crops + Egg), gold rewards only.
// reward_gold = sell_price(item) * quantity * kOrderRewardBonusNumerator /
//               kOrderRewardBonusDenominator (truncating integer division).
inline constexpr int kOrderBoardSlotCount = 3;
inline constexpr int kOrderItemPoolCount = 4;

inline constexpr int kOrderWheatQtyMin = 2;
inline constexpr int kOrderWheatQtyMax = 5;
inline constexpr int kOrderCornQtyMin = 2;
inline constexpr int kOrderCornQtyMax = 4;
inline constexpr int kOrderCarrotQtyMin = 1;
inline constexpr int kOrderCarrotQtyMax = 3;
inline constexpr int kOrderEggQtyMin = 1;
inline constexpr int kOrderEggQtyMax = 3;

inline constexpr int kOrderRewardBonusNumerator = 3;
inline constexpr int kOrderRewardBonusDenominator = 2;

// FeedMill V1 recipes (durations in whole ticks, same convention as planting).
inline constexpr int kFeedMillChickenFeedTicks = 2;
inline constexpr int kFeedMillCowFeedTicks = 3;

// Chicken coop V1: fixed slots, ChickenFeed -> Egg after production ticks.
inline constexpr int kChickenCoopSlotCount = 2;
inline constexpr int kChickenEggProductionTicks = 3;

inline constexpr int kEggSellPrice = 6;

}  // namespace farm
