#pragma once

#include "Constants.h"
#include "Types.h"

#include <string_view>

namespace farm {

struct ItemMeta {
    std::string_view name;
    bool is_seed = false;
    ItemId crop_from_seed = ItemId::Wheat;  // valid if is_seed
    int grow_ticks = 0;                     // crop grow duration; seed copies from crop
    int sell_price = 0;                   // crop sell price; 0 if not sellable as crop
    int seed_shop_price = 0;              // shop price for seed; 0 if not a seed
};

class ItemCatalog {
public:
    static const ItemMeta& Get(ItemId id);
    static bool IsSeed(ItemId id);
    static ItemId CropFromSeed(ItemId seed_id);
    static int GrowTicks(ItemId crop_id);
    static int SellPrice(ItemId crop_id);
    static int SeedShopPrice(ItemId seed_id);
};

}  // namespace farm
