#include "farm/core/Game.h"

#include "farm/common/Constants.h"
#include "farm/persistence/SaveManager.h"

#include <algorithm>
#include <cstdlib>

namespace farm {

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

void WeatherSystem::Tick(int current_tick) {
    --remaining_ticks_;
    if (remaining_ticks_ <= 0) {
        ChooseNext(current_tick);
    }
}

void WeatherSystem::SetForLoad(WeatherType weather, int remaining_ticks) {
    weather_ = weather;
    remaining_ticks_ = std::max(1, remaining_ticks);
}

void WeatherSystem::ChooseNext(int current_tick) {
    const int pick = std::abs(current_tick * 37 + 13) % 4;
    weather_ = static_cast<WeatherType>(pick);
    remaining_ticks_ = 120 + (current_tick % 120);
}

Game::Game() = default;

Game Game::NewGame() { return Game{}; }

void Game::AdvanceTicks(int count) {
    for (int i = 0; i < count; ++i) {
        time_.AdvanceOneTick();
        weather_.Tick(time_.CurrentTick());
        const WeatherSnapshot weather = weather_.Snapshot();
        planting_.Tick(time_.CurrentTick(), weather.crop_multiplier);
        ranch_.Tick(time_.CurrentTick(), weather.ranch_multiplier);
        workshop_.Tick();
        orders_.Tick(time_.CurrentTick());
        MaybeTriggerRandomEvent();
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
    last_random_event_tick_ = time_.CurrentTick();
    const int pick = (time_.CurrentTick() * 17 + player_.Level() * 5) % 3;
    if (pick == 0) {
        player_.AddGold(12);
        last_event_message_ = "随机事件：路边小摊买走了一些农产品，获得 12 金币。";
    } else if (pick == 1) {
        (void)player_.TryAddItem(ItemId::Fertilizer, 1);
        last_event_message_ = "随机事件：邻居送来 1 份肥料。";
    } else {
        weather_.SetForLoad(WeatherType::Rainy, 120);
        last_event_message_ = "随机事件：一阵小雨经过，作物成长更快。";
    }
}

}  // namespace farm

