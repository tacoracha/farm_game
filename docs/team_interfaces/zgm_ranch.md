# zgm - 养殖接口规划

## 模块职责

zgm 负责鸡圈、牛棚、猪圈、羊圈等养殖场，以及动物品质等级和养殖场独立等级。养殖系统产出动物产品，供仓库、订单和工厂使用。

更多细分接口可参考：[docs/ranch/README.md](../ranch/README.md)。

## 对外提供接口

```cpp
enum class AnimalKind {
    Chicken,
    Cow,
    Pig,
    Sheep,
};

enum class AnimalQuality {
    Common,
    Fine,
    Rare,
};

enum class RanchFacilityKind {
    ChickenCoop,
    CowBarn,
    PigPen,
    SheepPen,
};

struct AnimalView {
    int animal_id;
    AnimalKind kind;
    AnimalQuality quality;
    AnimalState state;
    int finish_tick;
    int remaining_ticks;
    bool fed;
    ItemId product_id;
};

struct RanchFacilityView {
    int facility_id;
    RanchFacilityKind kind;
    int level;
    int capacity;
    int idle_count;
    int producing_count;
    int harvestable_count;
    int proficiency_exp;
};

class RanchSystem {
public:
    void Init();
    void Tick(int current_tick);
    void OnTick(int tick_delta, float weather_multiplier);

    std::vector<RanchFacilityView> GetAllFacilities() const;
    RanchFacilityView GetFacilityView(int facility_id) const;
    std::vector<AnimalView> GetAnimals(int facility_id) const;

    Result<void> BuildFacility(PlayerState& player, RanchFacilityKind kind);
    Result<void> UpgradeFacility(PlayerState& player, int facility_id);
    bool IsRanchUnlocked(RanchFacilityKind kind, int player_level) const;

    Result<int> BuyAnimal(PlayerState& player, int facility_id, AnimalKind kind,
                          AnimalQuality quality);
    Result<void> FeedAnimal(PlayerState& player, int facility_id, int animal_id, int current_tick);
    Result<void> HarvestAnimal(PlayerState& player, int facility_id, int animal_id);
    Result<int> BatchFeed(PlayerState& player, int facility_id, int current_tick);
    Result<int> BatchHarvest(PlayerState& player, int facility_id);

    bool IsAnimalProductUnlocked(ItemId product_id) const;
    int EstimateDailyOutput(ItemId product_id) const;
    int GetReadyHarvestCount() const;
    float GetQualityMultiplier(AnimalQuality quality) const;
};
```

## 需要其他人提供接口

- cdy：`CurrentTick()`、`Tick` 调度、天气生产加成、存档注册。
- gml：`TryAddToWarehouse`、`TryRemoveFromWarehouse`、`TrySpendGold`、`HasItem`，用于喂养、收获、升级。
- zjx：饲料配方与饲料生产状态，用于缺饲料提示和快捷跳转。
- lly：订单读取 `IsAnimalProductUnlocked`、`EstimateDailyOutput`，避免生成超产能订单。
- zxc：农作物作为饲料原料的产能查询。

## 给其他模块的联调约定

- 收获只把动物产品写入仓库，不直接发订单奖励。
- 品质只影响产出数量、产品类型或价值系数，不改变动物生命周期。
- 养殖场等级为每个设施独立维护，等级范围建议 1-10。
- 建造/升级/购买动物需要金币和工具时，统一通过 gml/`PlayerState` 扣除。
- `BuyAnimal` 只把动物加入指定养殖场，不直接开始生产；喂食后才进入倒计时。
- 养殖熟练度可以由收获或生产完成增加，具体经验发放由 cdy 总控统一记录。
