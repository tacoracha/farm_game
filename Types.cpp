#include "farm/common/Types.h"

#include "farm/common/Constants.h"

#include <array>

namespace farm {
namespace {

constexpr std::array<ItemInfo, 23> kItems{{
    {ItemId::WheatSeed, "Wheat Seed", ItemCategory::Seed, 3, 0, ItemId::Wheat, kWheatGrowTicks},
    {ItemId::CornSeed, "Corn Seed", ItemCategory::Seed, 5, 0, ItemId::Corn, kCornGrowTicks},
    {ItemId::CarrotSeed, "Carrot Seed", ItemCategory::Seed, 4, 0, ItemId::Carrot, kCarrotGrowTicks},
    {ItemId::TomatoSeed, "Tomato Seed", ItemCategory::Seed, 7, 0, ItemId::Tomato, kTomatoGrowTicks},
    {ItemId::StrawberrySeed, "Strawberry Seed", ItemCategory::Seed, kStrawberrySeedBuyPrice, 0, ItemId::Strawberry, kStrawberryGrowTicks},
    {ItemId::PumpkinSeed, "Pumpkin Seed", ItemCategory::Seed, kPumpkinSeedBuyPrice, 0, ItemId::Pumpkin, kPumpkinGrowTicks},
    {ItemId::MushroomSeed, "Mushroom Spore", ItemCategory::Seed, kMushroomSeedBuyPrice, 0, ItemId::Mushroom, kMushroomGrowTicks},
    {ItemId::Wheat, "Wheat", ItemCategory::Crop, 0, 6, ItemId::Wheat, kWheatGrowTicks},
    {ItemId::Corn, "Corn", ItemCategory::Crop, 0, 8, ItemId::Corn, kCornGrowTicks},
    {ItemId::Carrot, "Carrot", ItemCategory::Crop, 0, 7, ItemId::Carrot, kCarrotGrowTicks},
    {ItemId::Tomato, "Tomato", ItemCategory::Crop, 0, 12, ItemId::Tomato, kTomatoGrowTicks},
    {ItemId::Strawberry, "Strawberry", ItemCategory::Crop, 0, kStrawberrySellPrice, ItemId::Wheat, 0},
    {ItemId::Pumpkin, "Pumpkin", ItemCategory::Crop, 0, kPumpkinSellPrice, ItemId::Wheat, 0},
    {ItemId::Mushroom, "Mushroom", ItemCategory::Crop, 0, kMushroomSellPrice, ItemId::Wheat, 0},
    {ItemId::ChickenFeed, "Chicken Feed", ItemCategory::Feed, 0, 4, ItemId::Wheat, 0},
    {ItemId::CowFeed, "Cow Feed", ItemCategory::Feed, 0, 7, ItemId::Corn, 0},
    {ItemId::Egg, "Egg", ItemCategory::AnimalProduct, 0, 12, ItemId::Wheat, 0},
    {ItemId::Milk, "Milk", ItemCategory::AnimalProduct, 0, 22, ItemId::Wheat, 0},
    {ItemId::Wool, "Wool", ItemCategory::AnimalProduct, 0, 28, ItemId::Wheat, 0},
    {ItemId::Fertilizer, "Fertilizer", ItemCategory::Consumable, kFertilizerCost, 0, ItemId::Wheat, 0},
    {ItemId::Bread, "Bread", ItemCategory::ProcessedGood, 0, 18, ItemId::Wheat, 0},
    {ItemId::Cheese, "Cheese", ItemCategory::ProcessedGood, 0, 35, ItemId::Wheat, 0},
    {ItemId::Jam, "Jam", ItemCategory::ProcessedGood, 0, 40, ItemId::Wheat, 0},
}};

}  // namespace

const ItemInfo& GetItemInfo(ItemId id) {
    for (const ItemInfo& item : kItems) {
        if (item.id == id) {
            return item;
        }
    }
    return kItems[3];
}

const char* ToString(ItemId id) { return GetItemInfo(id).name; }

const char* ToString(ItemCategory category) {
    switch (category) {
        case ItemCategory::Seed:
            return "Seed";
        case ItemCategory::Crop:
            return "Crop";
        case ItemCategory::Feed:
            return "Feed";
        case ItemCategory::AnimalProduct:
            return "Animal Product";
        case ItemCategory::Consumable:
            return "Consumable";
        case ItemCategory::ProcessedGood:
            return "Processed Good";
    }
    return "Unknown";
}

const char* ToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::Ok:
            return "Ok";
        case ErrorCode::InvalidItem:
            return "Invalid item";
        case ErrorCode::InvalidQuantity:
            return "Invalid quantity";
        case ErrorCode::InsufficientGold:
            return "Not enough gold";
        case ErrorCode::InsufficientItem:
            return "Not enough items";
        case ErrorCode::WarehouseFull:
            return "Warehouse is full";
        case ErrorCode::ProtectedItem:
            return "Item is locked";
        case ErrorCode::CannotSell:
            return "This item cannot be sold";
        case ErrorCode::NotASeed:
            return "Item is not a seed";
        case ErrorCode::NotFertilizer:
            return "Item is not fertilizer";
        case ErrorCode::SeedNotUnlocked:
            return "Seed is locked";
        case ErrorCode::ContentLocked:
            return "Content is locked";
        case ErrorCode::PlotOutOfRange:
            return "Plot does not exist";
        case ErrorCode::PlotNotIdle:
            return "Plot is already occupied";
        case ErrorCode::PlotNotGrowing:
            return "Plot is not growing";
        case ErrorCode::PlotAlreadyWatered:
            return "Plot was already watered";
        case ErrorCode::PlotAlreadyFertilized:
            return "Plot was already fertilized";
        case ErrorCode::PlotNotMature:
            return "Crop is not mature";
        case ErrorCode::NoIdlePlot:
            return "No empty plot";
        case ErrorCode::FacilityOutOfRange:
            return "Facility does not exist";
        case ErrorCode::FacilityFull:
            return "Facility is full";
        case ErrorCode::AnimalOutOfRange:
            return "Animal does not exist";
        case ErrorCode::AnimalNotIdle:
            return "Animal is busy";
        case ErrorCode::AnimalNotReady:
            return "Animal product is not ready";
        case ErrorCode::RecipeUnavailable:
            return "Recipe is unavailable";
        case ErrorCode::ProductionQueueFull:
            return "Production queue is full";
        case ErrorCode::ProductNotReady:
            return "No product ready";
        case ErrorCode::ShelfFull:
            return "Factory shelf is full";
        case ErrorCode::OrderSlotOutOfRange:
            return "Order slot does not exist";
        case ErrorCode::OrderCoolingDown:
            return "Order slot is cooling down";
        case ErrorCode::OrderLocked:
            return "Order is locked";
        case ErrorCode::SaveOpenFailed:
            return "Could not open save file";
        case ErrorCode::SaveWriteFailed:
            return "Could not write save file";
        case ErrorCode::SaveReadFailed:
            return "Could not read save file";
        case ErrorCode::SaveVersionMismatch:
            return "Save version is not compatible";
        case ErrorCode::SaveCorrupted:
            return "Save file is corrupted";
        case ErrorCode::GamePaused:
            return "Game is paused";
    }
    return "Unknown error";
}

bool IsSeed(ItemId id) { return GetItemInfo(id).category == ItemCategory::Seed; }

bool IsFertilizer(ItemId id) { return id == ItemId::Fertilizer; }

ItemId CropFromSeed(ItemId seed) { return GetItemInfo(seed).crop_from_seed; }

std::vector<ItemId> AllItems() {
    std::vector<ItemId> ids;
    ids.reserve(kItems.size());
    for (const ItemInfo& item : kItems) {
        ids.push_back(item.id);
    }
    return ids;
}

}  // namespace farm
