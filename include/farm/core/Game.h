#pragma once

#include "farm/common/Types.h"
#include "farm/inventory/PlayerState.h"
#include "farm/order/OrderSystem.h"
#include "farm/planting/PlantingSystem.h"
#include "farm/ranch/RanchSystem.h"
#include "farm/shop/ShopSystem.h"
#include "farm/workshop/WorkshopSystem.h"

#include <string>

namespace farm {

struct TimeSnapshot {
    int tick = 0;
    int day = 1;
    int hour = 6;
    GameSpeed speed = GameSpeed::Normal;
    bool paused = false;
};

struct WeatherSnapshot {
    WeatherType weather = WeatherType::Sunny;
    int remaining_ticks = 0;
    float crop_multiplier = 1.0f;
    float ranch_multiplier = 1.0f;
};

class TimeSystem {
public:
    TimeSnapshot Snapshot() const;
    int CurrentTick() const { return tick_; }
    GameSpeed Speed() const { return speed_; }
    bool IsPaused() const { return speed_ == GameSpeed::Paused; }

    void SetSpeed(GameSpeed speed) { speed_ = speed; }
    void Pause() { speed_ = GameSpeed::Paused; }
    void Resume() { speed_ = GameSpeed::Normal; }
    int TicksPerFrame() const { return static_cast<int>(speed_); }
    void AdvanceOneTick() { ++tick_; }
    void SetTickForLoad(int tick) { tick_ = tick; }

private:
    int tick_ = 0;
    GameSpeed speed_ = GameSpeed::Normal;
};

class WeatherSystem {
public:
    WeatherSnapshot Snapshot() const;
    WeatherType Current() const { return weather_; }
    void Tick(int current_tick);
    void SetForLoad(WeatherType weather, int remaining_ticks);

private:
    void ChooseNext(int current_tick);

    WeatherType weather_ = WeatherType::Sunny;
    int remaining_ticks_ = 8;
};

class Game {
public:
    Game();

    static Game NewGame();

    TimeSystem& Time() { return time_; }
    const TimeSystem& Time() const { return time_; }
    WeatherSystem& Weather() { return weather_; }
    const WeatherSystem& Weather() const { return weather_; }
    PlayerState& Player() { return player_; }
    const PlayerState& Player() const { return player_; }
    PlantingSystem& Planting() { return planting_; }
    const PlantingSystem& Planting() const { return planting_; }
    RanchSystem& Ranch() { return ranch_; }
    const RanchSystem& Ranch() const { return ranch_; }
    WorkshopSystem& Workshop() { return workshop_; }
    const WorkshopSystem& Workshop() const { return workshop_; }
    OrderSystem& Orders() { return orders_; }
    const OrderSystem& Orders() const { return orders_; }
    ShopSystem& Shop() { return shop_; }
    const ShopSystem& Shop() const { return shop_; }

    void AdvanceTicks(int count);
    Result<void> AdvanceBySpeed();
    Result<void> ManualSave(const std::string& path) const;
    Result<void> Load(const std::string& path);
    Result<void> AutoSaveIfNeeded(const std::string& path);
    int LastAutoSaveTick() const { return last_auto_save_tick_; }
    void SetLastAutoSaveTickForLoad(int tick) { last_auto_save_tick_ = tick; }

private:
    TimeSystem time_;
    WeatherSystem weather_;
    PlayerState player_;
    PlantingSystem planting_;
    RanchSystem ranch_;
    WorkshopSystem workshop_;
    OrderSystem orders_;
    ShopSystem shop_;
    int last_auto_save_tick_ = 0;
};

}  // namespace farm

