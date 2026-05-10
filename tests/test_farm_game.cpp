#include "farm/ChickenCoop.h"
#include "farm/Constants.h"
#include "farm/FeedMill.h"
#include "farm/Game.h"
#include "farm/OrderSystem.h"
#include "farm/PlantingSystem.h"
#include "farm/PlayerState.h"
#include "farm/ShopSystem.h"
#include "farm/Types.h"
#include "farm/Warehouse.h"

#include <iostream>

namespace {

int g_failures = 0;

void ExpectTrue(bool condition, const char* expr, const char* file, int line) {
    if (!condition) {
        std::cerr << file << ":" << line << " EXPECT_TRUE failed: " << expr << "\n";
        ++g_failures;
    }
}

template <typename A, typename B>
void ExpectEq(const A& a, const B& b, const char* expr_a, const char* expr_b, const char* file,
              int line) {
    if (a != b) {
        std::cerr << file << ":" << line << " EXPECT_EQ failed: " << expr_a << " != " << expr_b << "\n";
        ++g_failures;
    }
}

#define EXPECT_TRUE(x) ExpectTrue(static_cast<bool>(x), #x, __FILE__, __LINE__)
#define EXPECT_EQ(a, b) ExpectEq((a), (b), #a, #b, __FILE__, __LINE__)

int FindFirstOrderSlotWithItem(const farm::OrderSystem& os, farm::ItemId id) {
    const auto& orders = os.GetOrders();
    for (std::size_t i = 0; i < orders.size(); ++i) {
        if (orders[i].item_id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// 1. 仓库容量限制测试
void Test_WarehouseCapacityLimit() {
    farm::Warehouse w(5);
    EXPECT_TRUE(w.TryAdd(farm::ItemId::Wheat, 3).ok());
    EXPECT_TRUE(w.TryAdd(farm::ItemId::Corn, 2).ok());
    EXPECT_EQ(w.UsedSlots(), 5);
    auto r = w.TryAdd(farm::ItemId::Carrot, 1);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::WarehouseFull);
}

// 2. 仓库添加/移除测试
void Test_WarehouseAddRemove() {
    farm::Warehouse w(20);
    EXPECT_TRUE(w.TryAdd(farm::ItemId::WheatSeed, 4).ok());
    EXPECT_TRUE(w.HasItem(farm::ItemId::WheatSeed, 4));
    EXPECT_EQ(w.GetCount(farm::ItemId::WheatSeed), 4);
    EXPECT_TRUE(w.TryRemove(farm::ItemId::WheatSeed, 2).ok());
    EXPECT_EQ(w.GetCount(farm::ItemId::WheatSeed), 2);
    auto fail = w.TryRemove(farm::ItemId::WheatSeed, 5);
    EXPECT_TRUE(!fail.ok());
    EXPECT_EQ(fail.code, farm::ErrorCode::InsufficientItem);
}

// 3. 仓库出售测试（通过 PlayerState：扣库存 + 加金币）
void Test_WarehouseSell() {
    farm::PlayerState p;
    const int gold_before = p.Gold();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 2).ok());
    EXPECT_TRUE(p.TrySellFromWarehouse(farm::ItemId::Wheat, 2).ok());
    EXPECT_EQ(p.GetItemCount(farm::ItemId::Wheat), 0);
    EXPECT_EQ(p.Gold(), gold_before + 2 * farm::kWheatSellPrice);

    auto prot = p.TrySellFromWarehouse(farm::ItemId::WheatSeed, 1);
    EXPECT_TRUE(!prot.ok());
    EXPECT_EQ(prot.code, farm::ErrorCode::ProtectedItem);
}

// 4. 种植播种成功测试
void Test_PlantSuccess() {
    farm::Game g;
    auto r = g.Planting().TryPlant(g.Player(), farm::ItemId::CornSeed, g.CurrentTick());
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(g.Player().GetItemCount(farm::ItemId::CornSeed), farm::kInitialCornSeedCount - 1);
    EXPECT_TRUE(g.Planting().Plots()[0].state == farm::PlotState::Growing);
}

// 5. 空田不足时播种失败测试
void Test_PlantNoIdlePlot() {
    farm::Game g;
    EXPECT_TRUE(g.Planting().TryPlant(g.Player(), farm::ItemId::WheatSeed, g.CurrentTick()).ok());
    EXPECT_TRUE(g.Planting().TryPlant(g.Player(), farm::ItemId::WheatSeed, g.CurrentTick()).ok());
    EXPECT_TRUE(g.Planting().TryPlant(g.Player(), farm::ItemId::WheatSeed, g.CurrentTick()).ok());
    auto r = g.Planting().TryPlant(g.Player(), farm::ItemId::CornSeed, g.CurrentTick());
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::NoIdlePlot);
}

// 6. 种子不足时播种失败测试
void Test_PlantInsufficientSeed() {
    farm::Game g;
    auto& pl = g.Player();
    while (pl.HasItem(farm::ItemId::WheatSeed, 1)) {
        EXPECT_TRUE(pl.TryRemoveFromWarehouse(farm::ItemId::WheatSeed, 1).ok());
    }
    auto r = g.Planting().TryPlant(pl, farm::ItemId::WheatSeed, g.CurrentTick());
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::InsufficientItem);
}

// 7. 作物成长到成熟测试
void Test_CropBecomesMature() {
    farm::Game g;
    EXPECT_TRUE(
        g.Planting().TryPlant(g.Player(), farm::ItemId::CarrotSeed, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kCarrotGrowTicks; ++i) {
        g.AdvanceTick();
    }
    EXPECT_TRUE(g.Planting().Plots()[0].state == farm::PlotState::Mature);
}

// 8. 未成熟收割失败测试
void Test_HarvestNotMatureFails() {
    farm::Game g;
    EXPECT_TRUE(
        g.Planting().TryPlant(g.Player(), farm::ItemId::WheatSeed, g.CurrentTick()).ok());
    auto r = g.Planting().TryHarvest(g.Player(), 0);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::PlotNotMature);
}

// 9. 成熟收割成功并入仓测试
void Test_HarvestSuccessToWarehouse() {
    farm::Game g;
    auto& p = g.Player();
    const int wheat_before = p.GetItemCount(farm::ItemId::Wheat);
    EXPECT_TRUE(g.Planting().TryPlant(p, farm::ItemId::WheatSeed, g.CurrentTick()).ok());
    g.AdvanceTick();
    g.AdvanceTick();
    EXPECT_TRUE(g.Planting().Plots()[0].state == farm::PlotState::Mature);
    EXPECT_TRUE(g.Planting().TryHarvest(p, 0).ok());
    EXPECT_EQ(p.GetItemCount(farm::ItemId::Wheat), wheat_before + 1);
    EXPECT_TRUE(g.Planting().Plots()[0].state == farm::PlotState::Idle);
}

// 10. 仓库满时收割失败测试
void Test_HarvestFailsWhenWarehouseFull() {
    farm::Game g;
    auto& p = g.Player();
    EXPECT_TRUE(g.Planting().TryPlant(p, farm::ItemId::WheatSeed, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kWheatGrowTicks; ++i) {
        g.AdvanceTick();
    }
    for (int n = 0; n < 25; ++n) {
        EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 1).ok());
    }
    EXPECT_EQ(p.GetWarehouse().UsedSlots(), farm::kWarehouseCapacity);
    auto r = g.Planting().TryHarvest(p, 0);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::WarehouseFull);
    EXPECT_TRUE(g.Planting().Plots()[0].state == farm::PlotState::Mature);
}

// Seed shop: buy WheatSeed success
void Test_SeedShop_BuyWheatSuccess() {
    farm::PlayerState p;
    farm::ShopSystem shop;
    const int gold_before = p.Gold();
    const int seeds_before = p.GetItemCount(farm::ItemId::WheatSeed);
    auto r = shop.BuySeed(p, farm::ItemId::WheatSeed, 1);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(p.Gold(), gold_before - farm::kWheatSeedPrice);
    EXPECT_EQ(p.GetItemCount(farm::ItemId::WheatSeed), seeds_before + 1);
}

// Seed shop: insufficient gold, no state change
void Test_SeedShop_InsufficientGoldNoChange() {
    farm::PlayerState p;
    farm::ShopSystem shop;
    EXPECT_TRUE(p.TrySpendGold(p.Gold() - 1).ok());
    const int gold_snap = p.Gold();
    const int used_snap = p.GetWarehouse().UsedSlots();
    auto r = shop.BuySeed(p, farm::ItemId::WheatSeed, 1);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::InsufficientGold);
    EXPECT_EQ(p.Gold(), gold_snap);
    EXPECT_EQ(p.GetWarehouse().UsedSlots(), used_snap);
}

// Seed shop: warehouse full, gold and warehouse unchanged
void Test_SeedShop_WarehouseFullNoChange() {
    farm::PlayerState p;
    farm::ShopSystem shop;
    while (p.GetWarehouse().UsedSlots() < farm::kWarehouseCapacity) {
        EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 1).ok());
    }
    const int gold_snap = p.Gold();
    const int used_snap = p.GetWarehouse().UsedSlots();
    auto r = shop.BuySeed(p, farm::ItemId::CarrotSeed, 1);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::WarehouseFull);
    EXPECT_EQ(p.Gold(), gold_snap);
    EXPECT_EQ(p.GetWarehouse().UsedSlots(), used_snap);
}

// Seed shop: non-seed item fails
void Test_SeedShop_NotASeed() {
    farm::PlayerState p;
    farm::ShopSystem shop;
    const int gold_snap = p.Gold();
    const int used_snap = p.GetWarehouse().UsedSlots();
    auto r = shop.BuySeed(p, farm::ItemId::Wheat, 1);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::NotASeed);
    EXPECT_EQ(p.Gold(), gold_snap);
    EXPECT_EQ(p.GetWarehouse().UsedSlots(), used_snap);
}

// Seed shop: invalid quantity
void Test_SeedShop_InvalidQuantity() {
    farm::PlayerState p;
    farm::ShopSystem shop;
    auto r = shop.BuySeed(p, farm::ItemId::WheatSeed, 0);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::InvalidQuantity);
}

// Seed shop: multi-quantity pricing
void Test_SeedShop_MultiQuantityTotals() {
    farm::PlayerState p;
    farm::ShopSystem shop;
    const int gold_before = p.Gold();
    const int corn_before = p.GetItemCount(farm::ItemId::CornSeed);
    const int qty = 3;
    auto r = shop.BuySeed(p, farm::ItemId::CornSeed, qty);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(p.Gold(), gold_before - qty * farm::kCornSeedPrice);
    EXPECT_EQ(p.GetItemCount(farm::ItemId::CornSeed), corn_before + qty);
}

void Test_Order_InitThreeSlots() {
    farm::Game g;
    EXPECT_EQ(g.Orders().GetOrders().size(), static_cast<std::size_t>(farm::kOrderBoardSlotCount));
}

void Test_Order_OnlySupportedPoolItems() {
    farm::Game g;
    for (const farm::Order& o : g.Orders().GetOrders()) {
        const bool ok_item = o.item_id == farm::ItemId::Wheat || o.item_id == farm::ItemId::Corn ||
                             o.item_id == farm::ItemId::Carrot || o.item_id == farm::ItemId::Egg;
        EXPECT_TRUE(ok_item);
    }
}

void Test_Order_QuantityRanges() {
    farm::Game g;
    for (const farm::Order& o : g.Orders().GetOrders()) {
        EXPECT_TRUE(farm::OrderSystem::IsValidOrderQuantity(o.item_id, o.quantity));
    }
}

void Test_Order_RewardGoldPositive() {
    farm::Game g;
    for (const farm::Order& o : g.Orders().GetOrders()) {
        EXPECT_TRUE(o.reward_gold > 0);
    }
}

void Test_Order_CompleteSuccessRefreshesSlot() {
    farm::Game g;
    farm::OrderSystem& orders = g.Orders();
    farm::PlayerState& p = g.Player();

    const farm::Order before = orders.GetOrders()[0];
    EXPECT_TRUE(p.TryAddToWarehouse(before.item_id, before.quantity).ok());

    const int gold_before = p.Gold();
    const int count_before = p.GetItemCount(before.item_id);

    auto r = orders.CompleteOrder(p, 0);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(p.Gold(), gold_before + before.reward_gold);
    EXPECT_EQ(p.GetItemCount(before.item_id), count_before - before.quantity);

    const farm::Order after = orders.GetOrders()[0];
    EXPECT_TRUE(after.id != before.id);
}

void Test_Order_CompleteInsufficientNoChange() {
    farm::Game g;
    farm::OrderSystem& orders = g.Orders();
    farm::PlayerState& p = g.Player();

    const farm::Order slot_order = orders.GetOrders()[0];
    const int gold_snap = p.Gold();
    const int item_snap = p.GetItemCount(slot_order.item_id);

    auto r = orders.CompleteOrder(p, 0);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::InsufficientItem);
    EXPECT_EQ(p.Gold(), gold_snap);
    EXPECT_EQ(p.GetItemCount(slot_order.item_id), item_snap);

    const farm::Order unchanged = orders.GetOrders()[0];
    EXPECT_EQ(unchanged.id, slot_order.id);
    EXPECT_EQ(unchanged.item_id, slot_order.item_id);
    EXPECT_EQ(unchanged.quantity, slot_order.quantity);
    EXPECT_EQ(unchanged.reward_gold, slot_order.reward_gold);
}

void Test_Order_InvalidSlotIndex() {
    farm::Game g;
    auto bad_low = g.Orders().CompleteOrder(g.Player(), -1);
    EXPECT_TRUE(!bad_low.ok());
    EXPECT_EQ(bad_low.code, farm::ErrorCode::OrderSlotOutOfRange);

    auto bad_high = g.Orders().CompleteOrder(g.Player(), farm::kOrderBoardSlotCount);
    EXPECT_TRUE(!bad_high.ok());
    EXPECT_EQ(bad_high.code, farm::ErrorCode::OrderSlotOutOfRange);
}

void Test_Order_MultipleCompletesStable() {
    farm::Game g;
    farm::OrderSystem& orders = g.Orders();
    farm::PlayerState& p = g.Player();

    for (int k = 0; k < 5; ++k) {
        const farm::Order o = orders.GetOrders()[0];
        EXPECT_TRUE(p.TryAddToWarehouse(o.item_id, o.quantity).ok());
        const int gold_before = p.Gold();
        const int cnt_before = p.GetItemCount(o.item_id);
        EXPECT_TRUE(orders.CompleteOrder(p, 0).ok());
        EXPECT_EQ(p.Gold(), gold_before + o.reward_gold);
        EXPECT_EQ(p.GetItemCount(o.item_id), cnt_before - o.quantity);
    }
}

void Test_Order_EggSlotInitialRangeAndReward() {
    farm::Game g;
    const int egg_slot = FindFirstOrderSlotWithItem(g.Orders(), farm::ItemId::Egg);
    EXPECT_TRUE(egg_slot >= 0);
    const farm::Order& egg_order = g.Orders().GetOrders()[static_cast<std::size_t>(egg_slot)];
    EXPECT_EQ(egg_order.item_id, farm::ItemId::Egg);
    EXPECT_TRUE(farm::OrderSystem::IsValidOrderQuantity(farm::ItemId::Egg, egg_order.quantity));
    const int expected_reward =
        (farm::kEggSellPrice * egg_order.quantity * farm::kOrderRewardBonusNumerator) /
        farm::kOrderRewardBonusDenominator;
    EXPECT_EQ(egg_order.reward_gold, expected_reward);
    EXPECT_TRUE(egg_order.reward_gold > 0);
}

void Test_Order_CompleteEggOrderSuccess() {
    farm::Game g;
    farm::OrderSystem& orders = g.Orders();
    farm::PlayerState& p = g.Player();

    const int egg_slot_idx = FindFirstOrderSlotWithItem(orders, farm::ItemId::Egg);
    EXPECT_TRUE(egg_slot_idx >= 0);
    const farm::Order egg_order = orders.GetOrders()[static_cast<std::size_t>(egg_slot_idx)];
    EXPECT_EQ(egg_order.item_id, farm::ItemId::Egg);

    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Egg, egg_order.quantity).ok());
    const int gold_before = p.Gold();
    const int eggs_before = p.GetItemCount(farm::ItemId::Egg);

    EXPECT_TRUE(orders.CompleteOrder(p, egg_slot_idx).ok());
    EXPECT_EQ(p.Gold(), gold_before + egg_order.reward_gold);
    EXPECT_EQ(p.GetItemCount(farm::ItemId::Egg), eggs_before - egg_order.quantity);

    const farm::Order after = orders.GetOrders()[static_cast<std::size_t>(egg_slot_idx)];
    EXPECT_TRUE(after.id != egg_order.id);
}

void Test_Order_CompleteEggInsufficientNoChange() {
    farm::Game g;
    farm::OrderSystem& orders = g.Orders();
    farm::PlayerState& p = g.Player();

    while (p.HasItem(farm::ItemId::Egg, 1)) {
        EXPECT_TRUE(p.TryRemoveFromWarehouse(farm::ItemId::Egg, 1).ok());
    }

    const int egg_slot_idx = FindFirstOrderSlotWithItem(orders, farm::ItemId::Egg);
    EXPECT_TRUE(egg_slot_idx >= 0);
    const farm::Order egg_order = orders.GetOrders()[static_cast<std::size_t>(egg_slot_idx)];
    EXPECT_EQ(egg_order.item_id, farm::ItemId::Egg);

    const int gold_snap = p.Gold();
    const int egg_snap = p.GetItemCount(farm::ItemId::Egg);

    auto r = orders.CompleteOrder(p, egg_slot_idx);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::InsufficientItem);
    EXPECT_EQ(p.Gold(), gold_snap);
    EXPECT_EQ(p.GetItemCount(farm::ItemId::Egg), egg_snap);

    const farm::Order unchanged = orders.GetOrders()[static_cast<std::size_t>(egg_slot_idx)];
    EXPECT_EQ(unchanged.id, egg_order.id);
    EXPECT_EQ(unchanged.quantity, egg_order.quantity);
    EXPECT_EQ(unchanged.reward_gold, egg_order.reward_gold);
}

void Test_Order_CropSlotStillCompletes() {
    farm::Game g;
    farm::OrderSystem& orders = g.Orders();
    farm::PlayerState& p = g.Player();

    const farm::Order corn_slot = orders.GetOrders()[0];
    EXPECT_EQ(corn_slot.item_id, farm::ItemId::Corn);

    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Corn, corn_slot.quantity).ok());
    EXPECT_TRUE(orders.CompleteOrder(p, 0).ok());
    EXPECT_TRUE(orders.GetOrders()[0].id != corn_slot.id);
}

void Test_FeedMill_ChickenRecipe_StartOk() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 2).ok());
    auto r = g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick());
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(g.Workshop().GetFeedMill().IsProducing());
}

void Test_FeedMill_CowRecipe_StartOk() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Corn, 2).ok());
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Carrot, 1).ok());
    auto r = g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::CowFeed, g.CurrentTick());
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(g.Workshop().GetFeedMill().IsProducing());
}

void Test_FeedMill_StartInsufficientIngredients() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    auto r = g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick());
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::InsufficientItem);
    EXPECT_TRUE(g.Workshop().GetFeedMill().IsIdle());
}

void Test_FeedMill_IngredientsRemovedOnStart() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 5).ok());
    const int wheat_before = p.GetItemCount(farm::ItemId::Wheat);
    EXPECT_TRUE(
        g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick()).ok());
    EXPECT_EQ(p.GetItemCount(farm::ItemId::Wheat), wheat_before - 2);
}

void Test_FeedMill_CollectWhileProducingFails() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 2).ok());
    EXPECT_TRUE(
        g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick()).ok());
    auto r = g.Workshop().GetFeedMill().Collect(p);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::FeedMillNotReadyToCollect);
}

void Test_FeedMill_BecomesReadyAfterTicks() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 2).ok());
    EXPECT_TRUE(
        g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kFeedMillChickenFeedTicks; ++i) {
        g.AdvanceTick();
    }
    EXPECT_TRUE(g.Workshop().GetFeedMill().IsReadyToCollect());
}

void Test_FeedMill_CollectAddsProduct() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 2).ok());
    EXPECT_TRUE(
        g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kFeedMillChickenFeedTicks; ++i) {
        g.AdvanceTick();
    }
    const int before = p.GetItemCount(farm::ItemId::ChickenFeed);
    EXPECT_TRUE(g.Workshop().GetFeedMill().Collect(p).ok());
    EXPECT_EQ(p.GetItemCount(farm::ItemId::ChickenFeed), before + 1);
}

void Test_FeedMill_CollectWarehouseFullStaysReady() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 2).ok());
    EXPECT_TRUE(
        g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kFeedMillChickenFeedTicks; ++i) {
        g.AdvanceTick();
    }
    while (p.GetWarehouse().UsedSlots() < farm::kWarehouseCapacity) {
        EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 1).ok());
    }
    auto r = g.Workshop().GetFeedMill().Collect(p);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::WarehouseFull);
    EXPECT_TRUE(g.Workshop().GetFeedMill().IsReadyToCollect());
}

void Test_FeedMill_SecondStartWhileBusyFails() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 4).ok());
    EXPECT_TRUE(
        g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick()).ok());
    auto r = g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick());
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::FeedMillBusy);
}

void Test_FeedMill_AfterCollectIdle() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 2).ok());
    EXPECT_TRUE(
        g.Workshop().GetFeedMill().StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kFeedMillChickenFeedTicks; ++i) {
        g.AdvanceTick();
    }
    EXPECT_TRUE(g.Workshop().GetFeedMill().Collect(p).ok());
    EXPECT_TRUE(g.Workshop().GetFeedMill().IsIdle());
}

void Test_Chicken_InitTwoIdleSlots() {
    farm::Game g;
    const farm::ChickenCoop& coop = g.Ranch().GetChickenCoop();
    EXPECT_EQ(coop.GetSlotCount(), farm::kChickenCoopSlotCount);
    for (int i = 0; i < coop.GetSlotCount(); ++i) {
        EXPECT_TRUE(coop.GetState(i) == farm::AnimalState::Idle);
    }
}

void Test_Chicken_FeedSuccess() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::ChickenFeed, 1).ok());
    auto r = g.Ranch().GetChickenCoop().FeedChicken(p, 0, g.CurrentTick());
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(g.Ranch().GetChickenCoop().GetState(0) == farm::AnimalState::Producing);
}

void Test_Chicken_FeedConsumesFeedImmediately() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::ChickenFeed, 3).ok());
    const int before = p.GetItemCount(farm::ItemId::ChickenFeed);
    EXPECT_TRUE(g.Ranch().GetChickenCoop().FeedChicken(p, 1, g.CurrentTick()).ok());
    EXPECT_EQ(p.GetItemCount(farm::ItemId::ChickenFeed), before - 1);
}

void Test_Chicken_FeedInsufficientFeed() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    auto r = g.Ranch().GetChickenCoop().FeedChicken(p, 0, g.CurrentTick());
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::InsufficientItem);
    EXPECT_TRUE(g.Ranch().GetChickenCoop().GetState(0) == farm::AnimalState::Idle);
}

void Test_Chicken_FeedAgainWhenNotIdleFails() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::ChickenFeed, 2).ok());
    EXPECT_TRUE(g.Ranch().GetChickenCoop().FeedChicken(p, 0, g.CurrentTick()).ok());
    auto r = g.Ranch().GetChickenCoop().FeedChicken(p, 0, g.CurrentTick());
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::ChickenNotIdleForFeed);
}

void Test_Chicken_TickBecomesReady() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::ChickenFeed, 1).ok());
    EXPECT_TRUE(g.Ranch().GetChickenCoop().FeedChicken(p, 0, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kChickenEggProductionTicks; ++i) {
        g.AdvanceTick();
    }
    EXPECT_TRUE(g.Ranch().GetChickenCoop().GetState(0) == farm::AnimalState::Ready);
}

void Test_Chicken_CollectWhileProducingFails() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::ChickenFeed, 1).ok());
    EXPECT_TRUE(g.Ranch().GetChickenCoop().FeedChicken(p, 0, g.CurrentTick()).ok());
    auto r = g.Ranch().GetChickenCoop().CollectEgg(p, 0);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::ChickenNotReadyToCollectEgg);
}

void Test_Chicken_CollectAddsEgg() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::ChickenFeed, 1).ok());
    EXPECT_TRUE(g.Ranch().GetChickenCoop().FeedChicken(p, 0, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kChickenEggProductionTicks; ++i) {
        g.AdvanceTick();
    }
    const int eggs_before = p.GetItemCount(farm::ItemId::Egg);
    EXPECT_TRUE(g.Ranch().GetChickenCoop().CollectEgg(p, 0).ok());
    EXPECT_EQ(p.GetItemCount(farm::ItemId::Egg), eggs_before + 1);
}

void Test_Chicken_CollectWarehouseFullStaysReady() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::ChickenFeed, 1).ok());
    EXPECT_TRUE(g.Ranch().GetChickenCoop().FeedChicken(p, 0, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kChickenEggProductionTicks; ++i) {
        g.AdvanceTick();
    }
    while (p.GetWarehouse().UsedSlots() < farm::kWarehouseCapacity) {
        EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Wheat, 1).ok());
    }
    auto r = g.Ranch().GetChickenCoop().CollectEgg(p, 0);
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::WarehouseFull);
    EXPECT_TRUE(g.Ranch().GetChickenCoop().GetState(0) == farm::AnimalState::Ready);
}

void Test_Chicken_AfterCollectIdle() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::ChickenFeed, 1).ok());
    EXPECT_TRUE(g.Ranch().GetChickenCoop().FeedChicken(p, 0, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kChickenEggProductionTicks; ++i) {
        g.AdvanceTick();
    }
    EXPECT_TRUE(g.Ranch().GetChickenCoop().CollectEgg(p, 0).ok());
    EXPECT_TRUE(g.Ranch().GetChickenCoop().GetState(0) == farm::AnimalState::Idle);
}

void Test_Chicken_InvalidSlotIndex() {
    farm::Game g;
    farm::PlayerState& p = g.Player();
    EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::ChickenFeed, 1).ok());
    auto r = g.Ranch().GetChickenCoop().FeedChicken(p, -1, g.CurrentTick());
    EXPECT_TRUE(!r.ok());
    EXPECT_EQ(r.code, farm::ErrorCode::ChickenSlotOutOfRange);
    auto r2 = g.Ranch().GetChickenCoop().FeedChicken(p, farm::kChickenCoopSlotCount, g.CurrentTick());
    EXPECT_TRUE(!r2.ok());
    EXPECT_EQ(r2.code, farm::ErrorCode::ChickenSlotOutOfRange);
}

void Test_Integration_MinFarmLoop() {
    farm::Game g;
    farm::PlayerState& p = g.Player();

    EXPECT_TRUE(g.Shop().BuySeed(p, farm::ItemId::WheatSeed, 1).ok());

    EXPECT_TRUE(g.Planting().TryPlant(p, farm::ItemId::WheatSeed, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kWheatGrowTicks; ++i) {
        g.AdvanceTick();
    }
    EXPECT_TRUE(g.Planting().TryHarvest(p, 0).ok());

    // Second sow uses the first Idle plot again (plot 0), not plot 1 — TryPlant does not target a plot index.
    EXPECT_TRUE(g.Planting().TryPlant(p, farm::ItemId::WheatSeed, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kWheatGrowTicks; ++i) {
        g.AdvanceTick();
    }
    EXPECT_TRUE(g.Planting().TryHarvest(p, 0).ok());

    EXPECT_TRUE(p.GetItemCount(farm::ItemId::Wheat) >= 2);

    EXPECT_TRUE(g.Workshop()
                    .GetFeedMill()
                    .StartProduction(p, farm::RecipeId::ChickenFeed, g.CurrentTick())
                    .ok());
    for (int i = 0; i < farm::kFeedMillChickenFeedTicks; ++i) {
        g.AdvanceTick();
    }
    EXPECT_TRUE(g.Workshop().GetFeedMill().Collect(p).ok());
    EXPECT_TRUE(p.GetItemCount(farm::ItemId::ChickenFeed) >= 1);

    EXPECT_TRUE(g.Ranch().GetChickenCoop().FeedChicken(p, 0, g.CurrentTick()).ok());
    for (int i = 0; i < farm::kChickenEggProductionTicks; ++i) {
        g.AdvanceTick();
    }
    EXPECT_TRUE(g.Ranch().GetChickenCoop().CollectEgg(p, 0).ok());
    EXPECT_TRUE(p.GetItemCount(farm::ItemId::Egg) >= 1);

    const int egg_slot = FindFirstOrderSlotWithItem(g.Orders(), farm::ItemId::Egg);
    EXPECT_TRUE(egg_slot >= 0);
    const farm::Order egg_contract = g.Orders().GetOrders()[static_cast<std::size_t>(egg_slot)];
    EXPECT_EQ(egg_contract.item_id, farm::ItemId::Egg);

    while (p.GetItemCount(farm::ItemId::Egg) < egg_contract.quantity) {
        EXPECT_TRUE(p.TryAddToWarehouse(farm::ItemId::Egg, 1).ok());
    }
    const int gold_before_order = p.Gold();
    EXPECT_TRUE(g.Orders().CompleteOrder(p, egg_slot).ok());
    EXPECT_EQ(p.Gold(), gold_before_order + egg_contract.reward_gold);
}

}  // namespace

int main() {
    Test_WarehouseCapacityLimit();
    Test_WarehouseAddRemove();
    Test_WarehouseSell();
    Test_PlantSuccess();
    Test_PlantNoIdlePlot();
    Test_PlantInsufficientSeed();
    Test_CropBecomesMature();
    Test_HarvestNotMatureFails();
    Test_HarvestSuccessToWarehouse();
    Test_HarvestFailsWhenWarehouseFull();
    Test_SeedShop_BuyWheatSuccess();
    Test_SeedShop_InsufficientGoldNoChange();
    Test_SeedShop_WarehouseFullNoChange();
    Test_SeedShop_NotASeed();
    Test_SeedShop_InvalidQuantity();
    Test_SeedShop_MultiQuantityTotals();
    Test_Order_InitThreeSlots();
    Test_Order_OnlySupportedPoolItems();
    Test_Order_QuantityRanges();
    Test_Order_RewardGoldPositive();
    Test_Order_CompleteSuccessRefreshesSlot();
    Test_Order_CompleteInsufficientNoChange();
    Test_Order_InvalidSlotIndex();
    Test_Order_MultipleCompletesStable();
    Test_Order_EggSlotInitialRangeAndReward();
    Test_Order_CompleteEggOrderSuccess();
    Test_Order_CompleteEggInsufficientNoChange();
    Test_Order_CropSlotStillCompletes();
    Test_FeedMill_ChickenRecipe_StartOk();
    Test_FeedMill_CowRecipe_StartOk();
    Test_FeedMill_StartInsufficientIngredients();
    Test_FeedMill_IngredientsRemovedOnStart();
    Test_FeedMill_CollectWhileProducingFails();
    Test_FeedMill_BecomesReadyAfterTicks();
    Test_FeedMill_CollectAddsProduct();
    Test_FeedMill_CollectWarehouseFullStaysReady();
    Test_FeedMill_SecondStartWhileBusyFails();
    Test_FeedMill_AfterCollectIdle();
    Test_Chicken_InitTwoIdleSlots();
    Test_Chicken_FeedSuccess();
    Test_Chicken_FeedConsumesFeedImmediately();
    Test_Chicken_FeedInsufficientFeed();
    Test_Chicken_FeedAgainWhenNotIdleFails();
    Test_Chicken_TickBecomesReady();
    Test_Chicken_CollectWhileProducingFails();
    Test_Chicken_CollectAddsEgg();
    Test_Chicken_CollectWarehouseFullStaysReady();
    Test_Chicken_AfterCollectIdle();
    Test_Chicken_InvalidSlotIndex();
    Test_Integration_MinFarmLoop();

    if (g_failures != 0) {
        std::cerr << g_failures << " test assertion(s) failed.\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    return 0;
}
