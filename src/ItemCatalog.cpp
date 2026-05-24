#include "farm/ItemCatalog.h"

namespace farm {

namespace {

ItemMeta MakeCrop(std::string_view name, int grow_ticks, int sell_price) {
    return ItemMeta{name, false, ItemId::Wheat, grow_ticks, sell_price, 0};
}

ItemMeta MakeSeed(std::string_view name, ItemId crop, int grow_ticks, int shop_price) {
    return ItemMeta{name, true, crop, grow_ticks, 0, shop_price};
}

ItemMeta MakeCraftedGood(std::string_view name) {
    return ItemMeta{name, false, ItemId::Wheat, 0, 0, 0};
}

ItemMeta MakeAnimalProduct(std::string_view name, int sell_price) {
    return ItemMeta{name, false, ItemId::Wheat, 0, sell_price, 0};
}

ItemMeta MakeConsumable(std::string_view name) {
    return ItemMeta{name, false, ItemId::Wheat, 0, 0, 0};
}

}  // namespace

const ItemMeta& ItemCatalog::Get(ItemId id) {
    switch (id) {
        case ItemId::Wheat: {
            static const ItemMeta wheat = MakeCrop("Wheat", kWheatGrowTicks, kWheatSellPrice);
            return wheat;
        }
        case ItemId::Corn: {
            static const ItemMeta corn = MakeCrop("Corn", kCornGrowTicks, kCornSellPrice);
            return corn;
        }
        case ItemId::Carrot: {
            static const ItemMeta carrot = MakeCrop("Carrot", kCarrotGrowTicks, kCarrotSellPrice);
            return carrot;
        }
        case ItemId::WheatSeed: {
            static const ItemMeta wheat_seed =
                MakeSeed("WheatSeed", ItemId::Wheat, kWheatGrowTicks, kWheatSeedPrice);
            return wheat_seed;
        }
        case ItemId::CornSeed: {
            static const ItemMeta corn_seed =
                MakeSeed("CornSeed", ItemId::Corn, kCornGrowTicks, kCornSeedPrice);
            return corn_seed;
        }
        case ItemId::CarrotSeed: {
            static const ItemMeta carrot_seed =
                MakeSeed("CarrotSeed", ItemId::Carrot, kCarrotGrowTicks, kCarrotSeedPrice);
            return carrot_seed;
        }
        case ItemId::ChickenFeed: {
            static const ItemMeta chicken_feed = MakeCraftedGood("ChickenFeed");
            return chicken_feed;
        }
        case ItemId::CowFeed: {
            static const ItemMeta cow_feed = MakeCraftedGood("CowFeed");
            return cow_feed;
        }
        case ItemId::Egg: {
            static const ItemMeta egg = MakeAnimalProduct("Egg", kEggSellPrice);
            return egg;
        }
        case ItemId::Fertilizer: {
            static const ItemMeta fertilizer = MakeConsumable("Fertilizer");
            return fertilizer;
        }
    }
    static const ItemMeta invalid{};
    return invalid;
}

bool ItemCatalog::IsSeed(ItemId id) { return Get(id).is_seed; }

ItemId ItemCatalog::CropFromSeed(ItemId seed_id) { return Get(seed_id).crop_from_seed; }

int ItemCatalog::GrowTicks(ItemId crop_id) { return Get(crop_id).grow_ticks; }

int ItemCatalog::SellPrice(ItemId crop_id) { return Get(crop_id).sell_price; }

int ItemCatalog::SeedShopPrice(ItemId seed_id) { return Get(seed_id).seed_shop_price; }

}  // namespace farm
