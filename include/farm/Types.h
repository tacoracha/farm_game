#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

namespace farm {

enum class ItemId : std::uint8_t {
    WheatSeed,
    CornSeed,
    CarrotSeed,
    Wheat,
    Corn,
    Carrot,
    ChickenFeed,
    CowFeed,
    Egg,
};

enum class PlotState : std::uint8_t {
    Idle,
    Growing,
    Mature,
};

enum class ErrorCode : std::uint16_t {
    Ok = 0,

    InsufficientGold,
    InsufficientItem,
    WarehouseFull,
    WarehouseEmpty,
    InvalidItem,
    InvalidQuantity,

    PlotNotIdle,
    PlotNotMature,
    PlotOutOfRange,
    NoIdlePlot,
    NotASeed,

    ProtectedItem,
    CannotSell,

    SeedNotUnlocked,

    OrderSlotOutOfRange,

    FeedMillBusy,
    FeedMillNotReadyToCollect,

    ChickenSlotOutOfRange,
    ChickenNotIdleForFeed,
    ChickenNotReadyToCollectEgg,

    InternalError,
    NotImplemented,
};

inline const char* ErrorCodeMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::Ok:
            return "Ok";
        case ErrorCode::InsufficientGold:
            return "Insufficient gold";
        case ErrorCode::InsufficientItem:
            return "Insufficient item";
        case ErrorCode::WarehouseFull:
            return "Warehouse full";
        case ErrorCode::WarehouseEmpty:
            return "Warehouse empty";
        case ErrorCode::InvalidItem:
            return "Invalid item";
        case ErrorCode::InvalidQuantity:
            return "Invalid quantity";
        case ErrorCode::PlotNotIdle:
            return "Plot is not idle";
        case ErrorCode::PlotNotMature:
            return "Crop is not mature";
        case ErrorCode::PlotOutOfRange:
            return "Plot index out of range";
        case ErrorCode::NoIdlePlot:
            return "No idle plot available";
        case ErrorCode::NotASeed:
            return "Item is not a seed";
        case ErrorCode::ProtectedItem:
            return "Item is protected from selling";
        case ErrorCode::CannotSell:
            return "Cannot sell this item";
        case ErrorCode::SeedNotUnlocked:
            return "Seed is not unlocked";
        case ErrorCode::OrderSlotOutOfRange:
            return "Order slot index out of range";
        case ErrorCode::FeedMillBusy:
            return "Feed mill is busy";
        case ErrorCode::FeedMillNotReadyToCollect:
            return "Feed mill has nothing ready to collect";
        case ErrorCode::ChickenSlotOutOfRange:
            return "Chicken coop slot index out of range";
        case ErrorCode::ChickenNotIdleForFeed:
            return "Chicken is not idle for feeding";
        case ErrorCode::ChickenNotReadyToCollectEgg:
            return "Chicken has no egg ready to collect";
        case ErrorCode::InternalError:
            return "Internal error";
        case ErrorCode::NotImplemented:
            return "Not implemented";
    }
    return "Unknown error";
}

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

}  // namespace farm

namespace std {
template <>
struct hash<farm::ItemId> {
    size_t operator()(farm::ItemId v) const noexcept {
        return hash<std::uint8_t>{}(static_cast<std::uint8_t>(v));
    }
};
}  // namespace std
