# Ranch Extension Interface Plan

本文档只规划接口与模块边界，不落地具体玩法实现。目标是为后续新增的两个养殖扩展模块提供可分配、可联调的接口契约：

- [动物品质等级系统](#动物品质等级系统)
- [养殖场独立等级系统](#养殖场独立等级系统)

现有落地点参考：

- 养殖入口：[include/farm/RanchSystem.h](../../include/farm/RanchSystem.h)
- 鸡圈当前实现：[include/farm/ChickenCoop.h](../../include/farm/ChickenCoop.h)
- 玩家资产/仓库入口：[include/farm/PlayerState.h](../../include/farm/PlayerState.h)
- 订单入口：[include/farm/OrderSystem.h](../../include/farm/OrderSystem.h)
- 工厂/饲料入口：[include/farm/WorkshopSystem.h](../../include/farm/WorkshopSystem.h)

## 动物品质等级系统

### 模块职责

为每只动物附加品质：普通、优良、珍稀。品质不改变动物基础生命周期，只影响产出物、产出数量、经验或订单价值系数。

### 建议数据结构

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

struct AnimalQualityRule {
    AnimalQuality quality;
    int spawn_weight;             // Common 70, Fine 25, Rare 5
    int output_bonus_percent;     // Common 0, Fine 20, Rare 50
    int exp_bonus_percent;        // Common 0, Fine 20, Rare 50
    int order_value_bonus_percent;
};

struct AnimalInstanceView {
    int animal_id;
    AnimalKind kind;
    AnimalQuality quality;
    AnimalState state;
    int finish_tick;
    int remaining_ticks;
    bool fed;
};
```

### 对外接口

这些接口由养殖模块提供，供订单、工厂、UI、存档等模块读取。

```cpp
class AnimalQualityService {
public:
    const AnimalQualityRule& GetRule(AnimalQuality quality) const;
    AnimalQuality RollQuality(AnimalKind kind, int ranch_level, int random_seed) const;

    int ApplyOutputBonus(int base_quantity, AnimalQuality quality) const;
    int ApplyExpBonus(int base_exp, AnimalQuality quality) const;
    float GetQualityMultiplier(AnimalQuality quality) const;

    ItemId GetPrimaryProduct(AnimalKind kind, AnimalQuality quality) const;
    int GetOrderValueBonusPercent(AnimalQuality quality) const;
};
```

### 需要其他模块提供的接口

- 仓库/玩家资产：[PlayerState](../../include/farm/PlayerState.h)

```cpp
Result<void> PlayerState::TryAddToWarehouse(ItemId id, int quantity);
int PlayerState::GetItemCount(ItemId id) const;
```

- 物品配置：[ItemCatalog](../../include/farm/ItemCatalog.h)

```cpp
const ItemMeta& ItemCatalog::Get(ItemId id);
int ItemCatalog::SellPrice(ItemId item_id);
```

- 订单系统：[OrderSystem](../../include/farm/OrderSystem.h)

```cpp
// 规划接口：订单生成时读取品质产物是否已解锁，以及品质价值系数。
bool IsAnimalProductUnlocked(ItemId product_id) const;
int ComputeQualityOrderReward(ItemId product_id, AnimalQuality quality, int quantity) const;
```

### 提供给其他模块的接口链接

- 给订单系统：`GetPrimaryProduct`、`GetOrderValueBonusPercent`、`ApplyOutputBonus`
- 给工厂系统：`GetPrimaryProduct`，用于判断高级加工配方材料，如优良蛋、珍稀牛奶
- 给 UI：`GetRule`、`AnimalInstanceView::quality`
- 给存档系统：`AnimalInstanceView::quality`

## 养殖场独立等级系统

### 模块职责

为每个养殖场维护独立等级，范围 1-10。升级消耗金币和对应养殖工具，提升容量、生产速度，并逐步解锁批量操作、自动收获、高品质动物概率等功能。

### 建议数据结构

```cpp
enum class RanchFacilityKind {
    ChickenCoop,
    CowBarn,
    PigPen,
    SheepPen,
};

struct RanchUpgradeCost {
    int gold;
    ItemId tool_id;
    int tool_count;
};

struct RanchFacilityLevelRule {
    RanchFacilityKind kind;
    int level;
    int animal_capacity;
    int production_speed_bonus_percent;
    int fine_quality_unlock_level;
    int rare_quality_unlock_level;
    bool batch_feed_unlocked;
    bool batch_harvest_unlocked;
    bool auto_harvest_unlocked;
    RanchUpgradeCost next_upgrade_cost;
};

struct RanchFacilityView {
    int facility_id;
    RanchFacilityKind kind;
    int level;
    int animal_capacity;
    int idle_count;
    int producing_count;
    int harvestable_count;
    int proficiency_exp;
};
```

### 对外接口

这些接口由养殖等级模块提供，供养殖建筑、UI、订单产能评估、存档读取。

```cpp
class RanchFacilityLevelService {
public:
    const RanchFacilityLevelRule& GetRule(RanchFacilityKind kind, int level) const;
    RanchFacilityView GetFacilityView(int facility_id) const;
    std::vector<RanchFacilityView> GetAllFacilities() const;

    Result<void> BuildFacility(PlayerState& player, RanchFacilityKind kind);
    Result<void> UpgradeFacility(PlayerState& player, int facility_id);
    Result<int> BuyAnimal(PlayerState& player, int facility_id, AnimalKind kind,
                          AnimalQuality quality);

    int GetProductionTicks(RanchFacilityKind kind, int level, int base_ticks) const;
    bool CanSpawnQuality(RanchFacilityKind kind, int level, AnimalQuality quality) const;
    bool IsBatchFeedUnlocked(int facility_id) const;
    bool IsBatchHarvestUnlocked(int facility_id) const;
    bool IsAutoHarvestUnlocked(int facility_id) const;
};
```

### 需要其他模块提供的接口

- 玩家经济与仓库：[PlayerState](../../include/farm/PlayerState.h)

```cpp
Result<void> PlayerState::TrySpendGold(int amount);
Result<void> PlayerState::TryRemoveFromWarehouse(ItemId id, int quantity);
bool PlayerState::HasItem(ItemId id, int minimum_count) const;
```

- 时间系统：当前项目以 [Game::AdvanceTick](../../include/farm/Game.h) 推进，升级后的生产速度只需要在喂养开始时折算 `finish_tick`。

```cpp
int Game::CurrentTick() const;
void RanchSystem::Tick(int current_tick);
```

- 工厂系统：[WorkshopSystem](../../include/farm/WorkshopSystem.h)

```cpp
// 规划接口：缺饲料时 UI 可跳转饲料坊，或订单/推荐系统计算饲料供给能力。
bool HasAvailableFeedRecipe(ItemId feed_id) const;
```

### 提供给其他模块的接口链接

- 给订单系统：`GetFacilityView`、`GetRule`，用于估算当前产能，避免生成超产能订单
- 给工厂系统：`GetFacilityView`，用于推荐饲料生产优先级
- 给 UI：`GetFacilityView`、`GetRule`、`UpgradeFacility`
- 给存档系统：`facility_id`、`kind`、`level`、动物槽位列表
- 给总体调控：`OnTick(tick_delta, weather_multiplier)`、养殖场/动物快照、离线结算入口

## 联调边界

1. 养殖模块只通过 `PlayerState` 改金币和仓库，不直接操作 `Warehouse`。
2. 品质系统只给出规则、产物映射、倍率，不自行发放物品。
3. 养殖场等级系统只计算等级属性、升级成本、能力开关，不直接生成订单或工厂配方。
4. 订单系统读取养殖产能和品质产物解锁情况，但订单完成仍通过 `PlayerState::TryRemoveFromWarehouse` 扣物品。
5. 工厂系统读取动物产品作为高级配方材料，但不修改动物品质或养殖场等级。

## 最小验收接口

后续真正实现时，建议先完成这些接口即可进入联调：

```cpp
AnimalQualityService::GetRule
AnimalQualityService::ApplyOutputBonus
AnimalQualityService::GetPrimaryProduct

RanchFacilityLevelService::GetRule
RanchFacilityLevelService::UpgradeFacility
RanchFacilityLevelService::GetProductionTicks
RanchFacilityLevelService::GetFacilityView
RanchFacilityLevelService::BuyAnimal
```
