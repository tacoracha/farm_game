#include "farm/merchant/TravelingMerchantSystem.h"

#include "farm/common/Constants.h"
#include "farm/inventory/PlayerState.h"

#include <algorithm>
#include <cstdlib>

namespace farm {

TravelingMerchantSystem::TravelingMerchantSystem() {
    Init();
}

void TravelingMerchantSystem::Init() {
    present_ = false;
    appear_tick_ = 0;
    next_check_day_ = 3;
    sale_items_.clear();
    buy_offers_.clear();
}

int TravelingMerchantSystem::DaysRemaining() const {
    if (!present_) return 0;
    return 1;  // stays 1 full day
}

void TravelingMerchantSystem::TryAppear(int current_tick) {
    int day = current_tick / kTicksPerDay + 1;
    if (day >= next_check_day_ && !present_) {
        int seed = day * 37 + 13;
        if ((seed % 100) < 40) {
            present_ = true;
            appear_tick_ = current_tick;
            GenerateSaleItems();
            GenerateBuyOffers();
        }
        next_check_day_ = day + 2 + ((day * 17) % 4);
    }
    if (present_ && day > (appear_tick_ / kTicksPerDay + 1)) {
        present_ = false;
        sale_items_.clear();
        buy_offers_.clear();
    }
}

void TravelingMerchantSystem::Tick(int current_tick) {
    TryAppear(current_tick);
}

void TravelingMerchantSystem::GenerateSaleItems() {
    sale_items_.clear();
    int seed = appear_tick_ * 41 + 7;

    struct SaleTemplate { ItemId item; int base_price; int max_stock; };
    static const SaleTemplate pool[] = {
        {ItemId::Fertilizer, 4, 10},
        {ItemId::WheatSeed, 2, 15},
        {ItemId::CornSeed, 4, 10},
        {ItemId::CarrotSeed, 3, 10},
        {ItemId::TomatoSeed, 5, 8},
        {ItemId::StrawberrySeed, 8, 5},
        {ItemId::PumpkinSeed, 10, 5},
        {ItemId::MushroomSeed, 12, 5},
        {ItemId::ChickenFeed, 3, 12},
        {ItemId::Egg, 10, 6},
        {ItemId::Milk, 18, 4},
        {ItemId::Bread, 15, 5},
    };
    constexpr int pool_sz = sizeof(pool) / sizeof(pool[0]);

    int count = 4 + (seed % 3);
    for (int i = 0; i < count; ++i) {
        int idx = (seed + i * 11) % pool_sz;
        const SaleTemplate& t = pool[idx];
        int stock = 1 + ((seed + i * 7) % t.max_stock);
        int price = t.base_price + (seed + i * 13) % std::max(1, t.base_price / 2) - t.base_price / 4;
        if (price < 1) price = 1;
        sale_items_.push_back({t.item, price, stock, false});
    }
}

void TravelingMerchantSystem::GenerateBuyOffers() {
    buy_offers_.clear();
    int seed = appear_tick_ * 73 + 23;

    struct BuyTemplate { ItemId item; int base_price; int max_qty; };
    static const BuyTemplate pool[] = {
        {ItemId::Wheat, 8, 10},
        {ItemId::Corn, 12, 8},
        {ItemId::Carrot, 11, 8},
        {ItemId::Tomato, 18, 6},
        {ItemId::Egg, 18, 6},
        {ItemId::Milk, 33, 4},
        {ItemId::Wool, 40, 4},
        {ItemId::Bread, 27, 5},
        {ItemId::Cheese, 50, 3},
        {ItemId::Jam, 58, 3},
    };
    constexpr int pool_sz = sizeof(pool) / sizeof(pool[0]);

    int count = 2 + (seed % 2);
    for (int i = 0; i < count; ++i) {
        int idx = (seed + i * 19) % pool_sz;
        const BuyTemplate& t = pool[idx];
        int qty = 2 + ((seed + i * 5) % t.max_qty);
        int price = t.base_price + (seed + i * 3) % std::max(1, t.base_price / 3);
        buy_offers_.push_back({t.item, price, qty, 0});
    }
}

Result<void> TravelingMerchantSystem::BuyFromMerchant(PlayerState& player, int index) {
    if (index < 0 || index >= static_cast<int>(sale_items_.size()))
        return Result<void>::failure(ErrorCode::InvalidItem);
    MerchantItem& mi = sale_items_[static_cast<std::size_t>(index)];
    if (mi.sold_out) return Result<void>::failure(ErrorCode::InvalidQuantity);
    auto spent = player.TrySpendGold(mi.price);
    if (!spent.ok()) return spent;
    auto added = player.TryAddItem(mi.item, 1);
    if (!added.ok()) {
        player.AddGold(mi.price);
        return added;
    }
    --mi.stock;
    if (mi.stock <= 0) mi.sold_out = true;
    return Result<void>::success();
}

Result<void> TravelingMerchantSystem::SellToMerchant(PlayerState& player, int index) {
    if (index < 0 || index >= static_cast<int>(buy_offers_.size()))
        return Result<void>::failure(ErrorCode::InvalidItem);
    MerchantOffer& mo = buy_offers_[static_cast<std::size_t>(index)];
    if (mo.bought >= mo.max_buy) return Result<void>::failure(ErrorCode::InvalidQuantity);
    if (player.IsItemLocked(mo.item))
        return Result<void>::failure(ErrorCode::ProtectedItem);
    auto removed = player.TryRemoveItem(mo.item, 1);
    if (!removed.ok()) return removed;
    player.AddGold(mo.price);
    ++mo.bought;
    return Result<void>::success();
}

void TravelingMerchantSystem::ClearForLoad() {
    present_ = false;
    appear_tick_ = 0;
    sale_items_.clear();
    buy_offers_.clear();
}

void TravelingMerchantSystem::SetForLoad(bool present, int appear_tick,
                                          const std::vector<MerchantItem>& items,
                                          const std::vector<MerchantOffer>& offers) {
    present_ = present;
    appear_tick_ = appear_tick;
    sale_items_ = items;
    buy_offers_ = offers;
}

}  // namespace farm
