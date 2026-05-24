# zxc - 种植接口规划

## 模块职责

zxc 负责田地、播种、浇水、肥料加速、成熟收割。种植系统产出农作物，供仓库、订单、工厂和饲料生产使用。

## 对外提供接口

```cpp
enum class PlotWaterState {
    Dry,
    Watered,
};

struct PlotView {
    int plot_id;
    PlotState state;
    PlotWaterState water_state;
    ItemId crop_id;
    int planted_tick;
    int finish_tick;
    int remaining_ticks;
    bool fertilized;
};

struct CropProductionEstimate {
    ItemId crop_id;
    int mature_count;
    int growing_count;
    int estimated_daily_output;
};

class PlantingSystem {
public:
    void Init(int initial_plot_count);
    void Tick(int current_tick);
    void OnTick(int tick_delta, float weather_growth_multiplier);

    const std::vector<PlotView>& GetPlotsForDisplay() const;
    PlotView GetPlot(int plot_id) const;
    int GetMatureCropCount() const;
    int GetGrowingCount() const;
    std::vector<CropProductionEstimate> EstimateCropOutput() const;

    Result<void> TryPlant(PlayerState& player, ItemId seed_id, int current_tick);
    Result<void> TryPlantAt(PlayerState& player, int plot_id, ItemId seed_id, int current_tick);
    Result<void> WaterPlot(int plot_id, int current_tick);
    Result<void> ApplyFertilizer(PlayerState& player, int plot_id, ItemId fertilizer_id);
    Result<void> TryHarvest(PlayerState& player, int plot_id);
    Result<int> ExpandPlot(PlayerState& player, int player_level);

    bool IsCropUnlocked(ItemId crop_id) const;
    bool IsSeedUnlocked(ItemId seed_id) const;
    const std::vector<CropConfig>& GetAllCropConfigs() const;
};
```

## 需要其他人提供接口

- cdy：`CurrentTick()`、天气成长加成、离线成长结算、存档注册。
- gml：种子/肥料扣除、收获入库、商店购买种子。
- zjx：工厂读取农作物库存和产能，提示缺料来源。
- zgm：饲料生产依赖农作物，养殖系统需要知道饲料原料是否可生产。
- lly：订单生成读取 `IsCropUnlocked` 和 `EstimateCropOutput`，交付仍由仓库扣物品。

## 给其他模块的联调约定

- 种植系统不直接加金币，出售和订单奖励交给 gml/lly。
- 天气只影响成长时间或产量加成，不改变仓库逻辑。
- 收割失败时田地保持成熟状态，避免仓库满导致作物丢失。
- `TryPlant` 可继续兼容“自动找第一个空闲地块”；指定田地播种使用 `TryPlantAt`。
- 推荐状态流转：`Idle -> Planted/NeedsWater -> Growing -> Mature -> Idle`。
- 施肥只允许每轮作物一次，失败时不扣肥料。

## 当前实现约定

- `TryPlant` 保持兼容：自动选择第一个空闲田地；需要指定田地时使用 `TryPlantAt`。
- `WaterPlot` 每轮作物只允许一次，成功后成熟时间提前 `kWaterGrowthBoostTicks`。
- `ApplyFertilizer` 消耗仓库中的 `ItemId::Fertilizer`，每轮作物只允许一次，成功后把剩余成长时间减半。
- 收割后田地会重置为 `Idle`、`Dry`、未施肥状态；仓库满导致收割失败时，成熟作物保留在田地中。
