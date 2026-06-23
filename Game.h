#pragma once

#include "farm/achievement/AchievementSystem.h"
#include "farm/common/Types.h"
#include "farm/dailytask/DailyTaskSystem.h"
#include "farm/fishing/FishingSystem.h"
#include "farm/inventory/PlayerState.h"
#include "farm/merchant/TravelingMerchantSystem.h"
#include "farm/order/OrderSystem.h"
#include "farm/planting/PlantingSystem.h"
#include "farm/ranch/RanchSystem.h"
#include "farm/shop/ShopSystem.h"
#include "farm/workshop/WorkshopSystem.h"

#include <string>
#include <vector>

namespace farm {

struct ToastMessage {
    std::string text;
    int color = 0;  // 0=green success, 1=red error, 2=orange event
    int until_tick = 0;
};

struct TimeSnapshot {
    int tick = 0;
    int day = 1;
    int hour = 6;
    int minute = 0;
    GameSpeed speed = GameSpeed::Normal;
    bool paused = false;
};

struct SeasonSnapshot {
    Season season = Season::Spring;
    int ticks_elapsed = 0;    // ticks into current season
    int ticks_remaining = 0;  // ticks until next season
    float crop_speed = 1.0f;
    float crop_yield = 1.0f;
    float ranch_speed = 1.0f;
};

class SeasonSystem {
public:
    SeasonSnapshot Snapshot() const;
    Season Current() const { return season_; }
    void Tick(int current_tick);
    bool JustChanged() const { return just_changed_; }
    void ClearJustChanged() { just_changed_ = false; }
    void SetForLoad(Season season, int season_start_tick);
    int SeasonStartTickForSave() const { return season_start_tick_; }

private:
    Season season_ = Season::Spring;
    int season_start_tick_ = 0;
    int current_tick_ = 0;
    bool just_changed_ = false;
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
    void Tick(int current_tick, Season season);
    void SetForLoad(WeatherType weather, int remaining_ticks);

private:
    void ChooseNext(int current_tick, Season season);

    WeatherType weather_ = WeatherType::Sunny;
    int remaining_ticks_ = 8;
};

class Game {
public:
    Game();

    static Game NewGame();

    TimeSystem& Time() { return time_; }
    const TimeSystem& Time() const { return time_; }
    SeasonSystem& Season() { return season_; }
    const SeasonSystem& Season() const { return season_; }
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
    FishingSystem& Fishing() { return fishing_; }
    const FishingSystem& Fishing() const { return fishing_; }
    AchievementSystem& Achievements() { return achievements_; }
    const AchievementSystem& Achievements() const { return achievements_; }
    DailyTaskSystem& DailyTasks() { return daily_tasks_; }
    const DailyTaskSystem& DailyTasks() const { return daily_tasks_; }
    TravelingMerchantSystem& Merchant() { return merchant_; }
    const TravelingMerchantSystem& Merchant() const { return merchant_; }

    void AdvanceTicks(int count);
    Result<void> AdvanceBySpeed();
    Result<void> ManualSave(const std::string& path) const;
    Result<void> Load(const std::string& path);
    Result<void> AutoSaveIfNeeded(const std::string& path);
    int ProcessOfflineSeconds(int offline_seconds);
    int LastAutoSaveTick() const { return last_auto_save_tick_; }
    void SetLastAutoSaveTickForLoad(int tick) { last_auto_save_tick_ = tick; }
    int LastRandomEventTick() const { return last_random_event_tick_; }
    void SetLastRandomEventTickForLoad(int tick) { last_random_event_tick_ = tick; }
    const std::string& LastEventMessage() const { return last_event_message_; }
    void ClearLastEventMessage() { last_event_message_.clear(); }

    // Toast notification system
    const std::vector<ToastMessage>& Toasts() const { return toast_queue_; }
    void EnqueueToast(const std::string& text, int color, int until_tick);
    void PruneToasts(int current_tick);

private:
    friend class SaveManager;

    void MaybeTriggerRandomEvent();

    TimeSystem time_;
    SeasonSystem season_;
    WeatherSystem weather_;
    PlayerState player_;
    PlantingSystem planting_;
    RanchSystem ranch_;
    WorkshopSystem workshop_;
    OrderSystem orders_;
    ShopSystem shop_;
    FishingSystem fishing_;
    AchievementSystem achievements_;
    DailyTaskSystem daily_tasks_;
    TravelingMerchantSystem merchant_;
    int last_auto_save_tick_ = 0;
    int last_random_event_tick_ = 0;
    std::string last_event_message_;

    // Active season event effects
    int event_crop_yield_bonus_ = 0;   // percent
    int event_ranch_penalty_ = 0;      // percent
    int event_until_tick_ = 0;
    bool event_auto_water_ = false;

    std::vector<ToastMessage> toast_queue_;
};

float CropSpeed(Season s);
float CropYield(Season s);
float RanchSpeed(Season s);
const char* SeasonName(Season s);
ItemId SeasonCrop(Season s);

}  // namespace farm
