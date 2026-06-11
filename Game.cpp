#include "farm/core/Game.h"

#include "farm/achievement/AchievementSystem.h"
#include "farm/common/Constants.h"
#include "farm/dailytask/DailyTaskSystem.h"
#include "farm/fishing/FishingSystem.h"
#include "farm/merchant/TravelingMerchantSystem.h"
#include "farm/persistence/SaveManager.h"

#include <algorithm>
#include <cstdlib>

namespace farm {

float CropSpeed(Season s) {
    switch (s) { case Season::Spring: return 1.15f; case Season::Summer: return 1.0f; case Season::Autumn: return 1.0f; case Season::Winter: return 0.6f; }
    return 1.0f;
}
float CropYield(Season s) {
    switch (s) { case Season::Spring: return 1.0f; case Season::Summer: return 1.0f; case Season::Autumn: return 1.2f; case Season::Winter: return 0.8f; }
    return 1.0f;
}
float RanchSpeed(Season s) {
    switch (s) { case Season::Spring: return 1.0f; case Season::Summer: return 0.9f; case Season::Autumn: return 1.0f; case Season::Winter: return 0.8f; }
    return 1.0f;
}

const char* SeasonName(Season s) {
    switch (s) { case Season::Spring: return "春季"; case Season::Summer: return "夏季"; case Season::Autumn: return "秋季"; case Season::Winter: return "冬季"; }
    return "";
}

ItemId SeasonCrop(Season s) {
    switch (s) { case Season::Spring: return ItemId::Strawberry; case Season::Summer: return ItemId::Tomato; case Season::Autumn: return ItemId::Pumpkin; case Season::Winter: return ItemId::Mushroom; }
    return ItemId::Wheat;
}

void SeasonSystem::Tick(int current_tick) {
    current_tick_ = current_tick;
    just_changed_ = false;
    if (season_start_tick_ == 0) season_start_tick_ = current_tick;
    if (current_tick - season_start_tick_ >= kSeasonDurationTicks) {
        int old = static_cast<int>(season_);
        season_ = static_cast<Season>((old + 1) % 4);
        season_start_tick_ = current_tick;
        just_changed_ = true;
    }
}

SeasonSnapshot SeasonSystem::Snapshot() const {
    int elapsed = current_tick_ - season_start_tick_;
    int remaining = std::max(0, kSeasonDurationTicks - elapsed);
    return SeasonSnapshot{season_, elapsed, remaining,
                          CropSpeed(season_), CropYield(season_), RanchSpeed(season_)};
}

void SeasonSystem::SetForLoad(Season season, int season_start_tick) {
    season_ = season;
    season_start_tick_ = season_start_tick;
    just_changed_ = false;
}

TimeSnapshot TimeSystem::Snapshot() const {
    const int day = tick_ / kTicksPerDay + 1;
    const int minute_of_day = (tick_ % kTicksPerDay) * kGameMinutesPerTick;
    const int shifted_minutes = (minute_of_day + 6 * 60) % (24 * 60);
    const int hour = shifted_minutes / 60;
    const int minute = shifted_minutes % 60;
    return TimeSnapshot{tick_, day, hour, minute, speed_, IsPaused()};
}

WeatherSnapshot WeatherSystem::Snapshot() const {
    float crop = 1.0f;
    float ranch = 1.0f;
    switch (weather_) {
        case WeatherType::Sunny:
            crop = 1.0f;
            ranch = 1.0f;
            break;
        case WeatherType::Rainy:
            crop = 1.35f;
            ranch = 1.0f;
            break;
        case WeatherType::Cloudy:
            crop = 1.1f;
            ranch = 1.0f;
            break;
        case WeatherType::Drought:
            crop = 0.65f;
            ranch = 0.9f;
            break;
    }
    return WeatherSnapshot{weather_, remaining_ticks_, crop, ranch};
}

void WeatherSystem::Tick(int current_tick, Season season) {
    --remaining_ticks_;
    if (remaining_ticks_ <= 0) {
        ChooseNext(current_tick, season);
    }
}

void WeatherSystem::SetForLoad(WeatherType weather, int remaining_ticks) {
    weather_ = weather;
    remaining_ticks_ = std::max(1, remaining_ticks);
}

void WeatherSystem::ChooseNext(int current_tick, Season season) {
    // Season-based weather distribution
    const int r = std::abs(current_tick * 37 + 13) % 100;
    switch (season) {
        case Season::Spring:
            if (r < 40) weather_ = WeatherType::Sunny;
            else if (r < 75) weather_ = WeatherType::Rainy;
            else if (r < 95) weather_ = WeatherType::Cloudy;
            else weather_ = WeatherType::Drought;
            break;
        case Season::Summer:
            if (r < 30) weather_ = WeatherType::Sunny;
            else if (r < 50) weather_ = WeatherType::Rainy;
            else if (r < 70) weather_ = WeatherType::Cloudy;
            else weather_ = WeatherType::Drought;
            break;
        case Season::Autumn:
            if (r < 50) weather_ = WeatherType::Sunny;
            else if (r < 75) weather_ = WeatherType::Rainy;
            else if (r < 100) weather_ = WeatherType::Cloudy;
            else weather_ = WeatherType::Drought;
            break;
        case Season::Winter:
            if (r < 60) weather_ = WeatherType::Sunny;
            else if (r < 70) weather_ = WeatherType::Rainy;
            else if (r < 90) weather_ = WeatherType::Cloudy;
            else weather_ = WeatherType::Drought;
            break;
    }
    remaining_ticks_ = 120 + (current_tick % 120);
}

Game::Game() = default;

Game Game::NewGame() {
    Game g;
    AchievementSystem::Instance().Init();
    DailyTaskSystem::Instance().Init();
    TravelingMerchantSystem::Instance().Init();
    FishingSystem::Instance().Init();
    return g;
}

void Game::AdvanceTicks(int count) {
    const int mature_before = planting_.MatureCount();
    const int ready_before = ranch_.ReadyProductCount();
    // Track visible order IDs to detect refreshes
    std::vector<int> order_ids_before;
    for (const OrderData& o : orders_.Orders()) {
        if (o.id != 0) order_ids_before.push_back(o.id);
    }

    for (int i = 0; i < count; ++i) {
        time_.AdvanceOneTick();
        season_.Tick(time_.CurrentTick());
        weather_.Tick(time_.CurrentTick(), season_.Current());
        const WeatherSnapshot weather = weather_.Snapshot();
        const SeasonSnapshot ssnap = season_.Snapshot();
        // Combine weather + season modifiers + event effects
        float crop_mod = weather.crop_multiplier * ssnap.crop_speed;
        float ranch_mod = weather.ranch_multiplier * ssnap.ranch_speed;
        if (time_.CurrentTick() < event_until_tick_) {
            if (event_ranch_penalty_ > 0) ranch_mod *= (1.0f - event_ranch_penalty_ / 100.0f);
        }
        // Summer drought: double watering effect
        planting_.SetSummerDrought(weather.weather == WeatherType::Drought &&
                                   season_.Current() == Season::Summer);
        // Autumn: 10% double harvest
        planting_.SetAutumnDouble(season_.Current() == Season::Autumn);
        planting_.Tick(time_.CurrentTick(), crop_mod);
        ranch_.Tick(time_.CurrentTick(), ranch_mod);
        // Auto-water during spring rain event
        if (event_auto_water_ && time_.CurrentTick() < event_until_tick_) {
            planting_.BatchWater(time_.CurrentTick());
        }
        workshop_.Tick(player_);
        orders_.Tick(time_.CurrentTick(), player_, season_.Current());
        DailyTaskSystem::Instance().Tick(time_.CurrentTick(), player_);
        TravelingMerchantSystem::Instance().Tick(time_.CurrentTick());
        FishingSystem::Instance().Tick(time_.CurrentTick());
        MaybeTriggerRandomEvent();
    }

    // Season change toast + withering + achievement hook
    if (season_.JustChanged()) {
        AchievementSystem::Instance().OnSeasonChange(season_.Current());
        std::string msg = std::string("季节更替：进入") + SeasonName(season_.Current()) + "！";
        int withered = planting_.WitherNonSeasonal(season_.Current());
        if (withered > 0) {
            msg += " " + std::to_string(withered) + " 块地作物枯萎了。";
        }
        EnqueueToast(msg, 2, time_.CurrentTick() + 80);
        season_.ClearJustChanged();
    }

    // Achievement periodic checks
    int total_animals = 0, chicken_count = 0;
    for (const auto& f : ranch_.Facilities()) {
        for (const auto& a : f.animals) {
            ++total_animals;
            if (a.kind == AnimalKind::Chicken) ++chicken_count;
        }
    }
    AchievementSystem::Instance().CheckPeriodic(time_.CurrentTick(), total_animals,
                                                  chicken_count, player_.WarehouseCapacity(),
                                                  player_.WarehouseUsed());

    // Pre-winter warning: 12 ticks before season change
    const SeasonSnapshot ssnap = season_.Snapshot();
    if (ssnap.ticks_remaining == 12) {
        farm::Season next = static_cast<farm::Season>((static_cast<int>(season_.Current()) + 1) % 4);
        EnqueueToast(std::string("距离") + SeasonName(next) + "还有 12 刻，请及时收获作物！", 2, time_.CurrentTick() + 40);
    }

    const int current = time_.CurrentTick();

    // Crop mature detection
    const int mature_after = planting_.MatureCount();
    if (mature_after > mature_before) {
        EnqueueToast("有 " + std::to_string(mature_after) + " 块地作物成熟了！", 0, current + 25);
    }

    // Animal product ready detection
    const int ready_after = ranch_.ReadyProductCount();
    if (ready_after > ready_before) {
        EnqueueToast("有 " + std::to_string(ready_after) + " 个动物产品可以收获了！", 0, current + 25);
    }

    // Order refresh detection — new order IDs appeared that weren't there before
    int new_order_count = 0;
    for (const OrderData& o : orders_.Orders()) {
        if (o.id == 0) continue;
        bool existed = false;
        for (int old_id : order_ids_before) {
            if (old_id == o.id) { existed = true; break; }
        }
        if (!existed) ++new_order_count;
    }
    if (new_order_count > 0) {
        EnqueueToast("有 " + std::to_string(new_order_count) + " 个新订单已刷新！", 2, current + 25);
    }
}

Result<void> Game::AdvanceBySpeed() {
    if (time_.IsPaused()) {
        return Result<void>::failure(ErrorCode::GamePaused);
    }
    AdvanceTicks(time_.TicksPerFrame());
    return Result<void>::success();
}

Result<void> Game::ManualSave(const std::string& path) const {
    return SaveManager::Save(*this, path);
}

Result<void> Game::Load(const std::string& path) {
    Game loaded;
    auto result = SaveManager::Load(path, loaded);
    if (!result.ok()) {
        return result;
    }
    *this = loaded;
    return Result<void>::success();
}

Result<void> Game::AutoSaveIfNeeded(const std::string& path) {
    if (time_.CurrentTick() - last_auto_save_tick_ < kAutoSaveEveryTicks) {
        return Result<void>::success();
    }
    auto saved = ManualSave(path);
    if (saved.ok()) {
        last_auto_save_tick_ = time_.CurrentTick();
    }
    return saved;
}

int Game::ProcessOfflineSeconds(int offline_seconds) {
    if (offline_seconds <= 0) {
        return 0;
    }
    const int offline_ticks =
        std::min(kMaxOfflineTicks, offline_seconds / kOfflineSecondsPerTick);
    if (offline_ticks > 0) {
        AdvanceTicks(offline_ticks);
        last_event_message_ = "离线进度结算：" + std::to_string(offline_ticks) + " 刻。";
    }
    return offline_ticks;
}

void Game::MaybeTriggerRandomEvent() {
    if (time_.CurrentTick() - last_random_event_tick_ < kRandomEventIntervalTicks) {
        return;
    }
    // Clear expired effects
    if (time_.CurrentTick() >= event_until_tick_) {
        event_crop_yield_bonus_ = 0;
        event_ranch_penalty_ = 0;
        event_auto_water_ = false;
    }

    last_random_event_tick_ = time_.CurrentTick();
    const int pick = (time_.CurrentTick() * 17 + player_.Level() * 5) % 4;
    farm::Season s = season_.Current();
    int dur = 24;

    switch (s) {
        case Season::Spring:
            if (pick == 0) {
                event_auto_water_ = true; event_until_tick_ = time_.CurrentTick() + 6;
                last_event_message_ = "春雨绵绵：接下来 6 刻所有作物自动浇水！";
            } else {
                player_.AddGold(12);
                last_event_message_ = "路边小摊买走了一些农产品，获得 12 金币。";
            }
            break;
        case Season::Summer:
            if (pick == 0) {
                event_ranch_penalty_ = 20; event_until_tick_ = time_.CurrentTick() + dur;
                last_event_message_ = "酷暑：动物生产速度 -20%，持续 24 刻。";
            } else if (pick == 1) {
                player_.AddGold(25);
                last_event_message_ = "西瓜商人高价收购！获得 25 金币。";
            } else {
                player_.AddGold(12);
                last_event_message_ = "路边小摊买走了一些农产品，获得 12 金币。";
            }
            break;
        case Season::Autumn:
            if (pick == 0) {
                event_crop_yield_bonus_ = 50; event_until_tick_ = time_.CurrentTick() + dur;
                last_event_message_ = "丰收节：所有作物产量 +50%，持续 24 刻！";
            } else {
                (void)player_.TryAddItem(ItemId::Fertilizer, 2);
                last_event_message_ = "落叶时节：邻居送来 2 份肥料。";
            }
            break;
        case Season::Winter:
            if (pick == 0) {
                player_.AddGold(100);
                (void)player_.TryAddItem(ItemId::Fertilizer, 5);
                last_event_message_ = "圣诞节：获得 100 金币和 5 个肥料！";
            } else {
                player_.AddGold(23);
                last_event_message_ = "暴风雪后清理：获得 23 金币补偿。";
            }
            break;
    }
    EnqueueToast(last_event_message_, 2, time_.CurrentTick() + 50);
}

void Game::EnqueueToast(const std::string& text, int color, int until_tick) {
    toast_queue_.push_back({text, color, until_tick});
    // Cap toast queue to prevent unbounded growth
    if (toast_queue_.size() > 8) {
        toast_queue_.erase(toast_queue_.begin());
    }
}

void Game::PruneToasts(int current_tick) {
    toast_queue_.erase(
        std::remove_if(toast_queue_.begin(), toast_queue_.end(),
                       [current_tick](const ToastMessage& t) { return t.until_tick <= current_tick; }),
        toast_queue_.end());
}

}  // namespace farm

