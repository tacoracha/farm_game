#pragma once

#include "farm/common/Types.h"

#include <string>
#include <vector>

namespace farm {

class PlayerState;

struct MerchantItem {
    ItemId item = ItemId::WheatSeed;
    int price = 0;      // buy price (what player pays)
    int stock = 1;      // available quantity
    bool sold_out = false;
};

struct MerchantOffer {
    ItemId item = ItemId::Wheat;
    int price = 0;      // what merchant pays per unit
    int max_buy = 5;    // max units merchant will buy
    int bought = 0;     // how many already sold
};

class TravelingMerchantSystem {
public:
    static TravelingMerchantSystem& Instance();

    void Init();
    void Tick(int current_tick);

    bool IsPresent() const { return present_; }
    int DaysRemaining() const;  // in game days

    const std::vector<MerchantItem>& SaleItems() const { return sale_items_; }
    const std::vector<MerchantOffer>& BuyOffers() const { return buy_offers_; }

    Result<void> BuyFromMerchant(PlayerState& player, int index);
    Result<void> SellToMerchant(PlayerState& player, int index);

    // Save / Load
    void ClearForLoad();
    void SetForLoad(bool present, int appear_tick, const std::vector<MerchantItem>& items,
                    const std::vector<MerchantOffer>& offers);

    bool PresentForSave() const { return present_; }
    int AppearTickForSave() const { return appear_tick_; }
    const std::vector<MerchantItem>& ItemsForSave() const { return sale_items_; }
    const std::vector<MerchantOffer>& OffersForSave() const { return buy_offers_; }

private:
    TravelingMerchantSystem() = default;
    void GenerateSaleItems();
    void GenerateBuyOffers();
    void TryAppear(int current_tick);

    bool present_ = false;
    int appear_tick_ = 0;
    int next_check_day_ = 3;  // first appearance after 3 days

    std::vector<MerchantItem> sale_items_;
    std::vector<MerchantOffer> buy_offers_;
};

}  // namespace farm
