# cdy - 时间、天气、存档、总体调控接口规划

## 模块职责

cdy 负责全局底层能力：统一时间推进、天气状态、存档读写、总体调控与跨模块事件调度。其他模块不应自己维护真实时间或独立存档格式。

## 对外提供接口

```cpp
enum class WeatherType {
    Sunny,
    Rainy,
    Drought,
    Windy,
    Snowy,
    Cloudy,
    Storm,
};

enum class GameEventType {
    TickAdvanced,
    WeatherChanged,
    SaveRequested,
    LoadCompleted,
    LevelUnlocked,
};

struct TimeSnapshot {
    int current_tick;
    int64_t server_time;
    int offline_seconds;
};

struct WeatherSnapshot {
    WeatherType current;
    int remaining_ticks;
    int crop_growth_bonus_percent;
    int ranch_production_bonus_percent;
    int market_refresh_bonus_percent;
};

class WorldControlService {
public:
    void Init(PlantingSystem& planting, RanchSystem& ranch, OrderSystem& orders,
              WorkshopSystem& workshop, PlayerState& player, ShopSystem& shop);

    TimeSnapshot GetTime() const;
    int CurrentTick() const;
    void AdvanceTick();
    void AdvanceTicks(int count);
    void SetSpeed(float multiplier);
    void Pause();
    void Resume();
    bool IsPaused() const;

    WeatherSnapshot GetWeather() const;
    float GetWeatherMultiplier(std::string_view system_tag) const;
    void SetWeatherForDebug(WeatherType weather, int duration_ticks);

    Result<void> AddExp(int amount, std::string_view source_type, int source_id = 0);
    int GetPlayerLevel() const;
    int GetCurrentExp() const;
    int GetExpToNextLevel() const;

    void RegisterEvent(GameEventType type, std::function<void()> callback);
    void BroadcastEvent(GameEventType type);
    Result<int> TriggerRandomEvent();

    Result<void> SaveGame(const Game& game, std::string_view path);
    Result<void> LoadGame(Game& game, std::string_view path);
    Result<void> AutoSave();
    Result<void> ProcessOfflineProgress(int offline_seconds);

    bool IsUnlocked(UnlockType type, int id) const;
    bool IsContentUnlocked(std::string_view content_id) const;
    void NotifyPlayerLevelChanged(int new_level);
};
```

## 需要其他人提供接口

- zxc 种植：`Tick/OnTick`，天气成长系数，地块快照，存档快照。
- zgm 养殖：`Tick/OnTick`，天气生产系数，养殖场/动物快照，存档快照。
- zjx 工厂：`Tick/OnTick`，离线生产结算，工厂队列和货架快照。
- lly 订单：订单冷却/刷新 Tick，订单统计，经验奖励回调。
- gml 仓库、商店：玩家金币、仓库、商店货架、锁定物品、存档快照。

## 给其他模块的联调约定

- 时间推进顺序由 cdy 统一维护，其他模块只实现 `Tick(int current_tick)`。
- 天气只返回加成数值，具体如何影响成长由各业务模块自行折算。
- 存档格式由 cdy 统一汇总，各模块提供 `Serialize/Deserialize` 或快照结构。
- `Update/AdvanceTick` 中可以统一调用：种植、养殖、工厂、订单、商店；模块内部不要自己推进全局时间。
- 离线收益最多保留 24 小时的规则由 cdy 统一裁剪，再把离线 tick 分发给种植/养殖/工厂/商店。
