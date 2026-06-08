#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace farm {

enum class ItemId : std::uint8_t {
    WheatSeed,
    CornSeed,
    CarrotSeed,
    TomatoSeed,
    Wheat,
    Corn,
    Carrot,
    Tomato,
    ChickenFeed,
    CowFeed,
    Egg,
    Milk,
    Wool,
    Fertilizer,
};

enum class ItemCategory : std::uint8_t {
    Seed,
    Crop,
    Feed,
    AnimalProduct,
    Consumable,
};

enum class ErrorCode : std::uint16_t {
    Ok = 0,
    InvalidItem,
    InvalidQuantity,
    InsufficientGold,
    InsufficientItem,
    WarehouseFull,
    ProtectedItem,
    CannotSell,
    NotASeed,
    NotFertilizer,
    SeedNotUnlocked,
    ContentLocked,
    PlotOutOfRange,
    PlotNotIdle,
    PlotNotGrowing,
    PlotAlreadyWatered,
    PlotAlreadyFertilized,
    PlotNotMature,
    NoIdlePlot,
    FacilityOutOfRange,
    FacilityFull,
    AnimalOutOfRange,
    AnimalNotIdle,
    AnimalNotReady,
    RecipeUnavailable,
    ProductionQueueFull,
    ProductNotReady,
    ShelfFull,
    OrderSlotOutOfRange,
    OrderCoolingDown,
    OrderLocked,
    SaveOpenFailed,
    SaveWriteFailed,
    SaveReadFailed,
    SaveVersionMismatch,
    SaveCorrupted,
    GamePaused,
};

template <typename T>
struct Result {
    ErrorCode code = ErrorCode::Ok;
    T value{};

    bool ok() const { return code == ErrorCode::Ok; }
    static Result success(T v) { return Result{ErrorCode::Ok, std::move(v)}; }
    static Result failure(ErrorCode ec) { return Result{ec, {}}; }
};

template <>
struct Result<void> {
    ErrorCode code = ErrorCode::Ok;

    bool ok() const { return code == ErrorCode::Ok; }
    static Result success() { return Result{ErrorCode::Ok}; }
    static Result failure(ErrorCode ec) { return Result{ec}; }
};

struct ItemStack {
    ItemId item = ItemId::Wheat;
    int quantity = 0;
};

struct ItemInfo {
    ItemId id = ItemId::Wheat;
    const char* name = "";
    ItemCategory category = ItemCategory::Crop;
    int buy_price = 0;
    int sell_price = 0;
    ItemId crop_from_seed = ItemId::Wheat;
    int grow_ticks = 0;
};

const char* ToString(ItemId id);
const char* ToString(ErrorCode code);
const char* ToString(ItemCategory category);
const ItemInfo& GetItemInfo(ItemId id);
bool IsSeed(ItemId id);
bool IsFertilizer(ItemId id);
ItemId CropFromSeed(ItemId seed);
std::vector<ItemId> AllItems();

enum class PlotState : std::uint8_t {
    Idle,
    Growing,
    Mature,
};

enum class PlotWaterState : std::uint8_t {
    Dry,
    Watered,
};

enum class WeatherType : std::uint8_t {
    Sunny,
    Rainy,
    Cloudy,
    Drought,
};

enum class GameSpeed : std::uint8_t {
    Paused = 0,
    Normal = 1,
    Fast = 2,
    VeryFast = 4,
};

enum class AnimalKind : std::uint8_t {
    Chicken,
    Cow,
    Pig,
    Sheep,
};

enum class AnimalState : std::uint8_t {
    Idle,
    Producing,
    Ready,
};

enum class AnimalQuality : std::uint8_t {
    Common,
    Fine,
    Rare,
};

enum class RanchFacilityKind : std::uint8_t {
    ChickenCoop,
    CowBarn,
    PigPen,
    SheepPen,
};

enum class FactoryKind : std::uint8_t {
    FeedMill,
    Dairy,
    Bakery,
    Textile,
};

enum class RecipeId : std::uint8_t {
    ChickenFeed,
    CowFeed,
};

enum class UnlockId : std::uint8_t {
    WheatSeed,
    CornSeed,
    CarrotSeed,
    TomatoSeed,
    ExtraLand,
    ChickenCoop,
    CowBarn,
    SheepPen,
    Chicken,
    Cow,
    Sheep,
};

enum class UnlockCategory : std::uint8_t {
    Seed,
    Land,
    Ranch,
    Animal,
};

enum class OrderState : std::uint8_t {
    Available,
    CoolingDown,
    Locked,
};

}  // namespace farm

namespace std {
template <>
struct hash<farm::ItemId> {
    size_t operator()(farm::ItemId id) const noexcept {
        return hash<unsigned int>{}(static_cast<unsigned int>(id));
    }
};
}  // namespace std
