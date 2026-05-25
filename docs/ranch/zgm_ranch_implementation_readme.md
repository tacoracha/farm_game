# zgm 养殖模块实现说明

## 模块定位

本模块由 zgm 负责，代码入口集中在：

- `include/farm/RanchSystem.h`
- `src/RanchSystem.cpp`

当前实现遵循团队接口规划中的养殖边界：养殖模块负责养殖场、动物、动物品质、设施等级、喂养、收获和产能估算；金币和仓库变化统一通过 `PlayerState` 完成，不直接修改仓库、订单、工厂、种植等其他模块内部数据。

## 当前完成度

本版本已经完成一版基础可联调实现：

- 提供统一的 `RanchSystem` 对外接口。
- 支持养殖场类型：鸡圈、牛棚、猪圈、羊圈。
- 支持动物类型：鸡、牛、猪、羊。
- 支持动物品质：普通、优秀、稀有。
- 支持设施视图和动物视图查询。
- 支持建造养殖场、升级养殖场。
- 支持购买动物、喂养动物、收获动物产品。
- 支持批量喂养、批量收获的等级解锁。
- 支持动物品质对产出、经验、订单价值的倍率计算。
- 支持动物产品解锁判断和日均产能估算。
- 保留旧版 `ChickenCoop` 接口，兼容已有鸡圈 UI 和原有测试。

## 对外核心类型

### 动物类型

```cpp
enum class AnimalKind {
    Chicken,
    Cow,
    Pig,
    Sheep,
};
```

### 动物品质

```cpp
enum class AnimalQuality {
    Common,
    Fine,
    Rare,
};
```

品质当前规则：

| 品质 | 产出加成 | 经验加成 | 订单价值加成 |
| --- | ---: | ---: | ---: |
| Common | 0% | 0% | 0% |
| Fine | 20% | 20% | 20% |
| Rare | 50% | 50% | 50% |

### 养殖场类型

```cpp
enum class RanchFacilityKind {
    ChickenCoop,
    CowBarn,
    PigPen,
    SheepPen,
};
```

### 动物视图

```cpp
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
```

### 养殖场视图

```cpp
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
```

## RanchSystem 对外接口

本节分为两类：第一类是 `docs/team_interfaces/zgm_ranch.md` 中列出的主接口；第二类是根据 `docs/ranch/README.md` 中的品质服务和等级服务细化出来的辅助接口。辅助接口仍然属于养殖模块内部能力对外暴露，不要求其他模块立即调用。

### 时间推进

```cpp
void Init();
void Tick(int current_tick);
void OnTick(int tick_delta, float weather_multiplier);
```

- `Init` 初始化养殖系统，默认创建一个 1 级鸡圈。
- `Tick` 根据当前 tick 推进动物生产状态。
- `OnTick` 给总控/天气系统使用，支持天气倍率。

### 查询接口

```cpp
std::vector<RanchFacilityView> GetAllFacilities() const;
RanchFacilityView GetFacilityView(int facility_id) const;
std::vector<AnimalView> GetAnimals(int facility_id) const;
int GetReadyHarvestCount() const;
```

这些接口主要给 UI、订单、工厂、存档和总控读取养殖状态。

### 养殖场操作

```cpp
Result<void> BuildFacility(PlayerState& player, RanchFacilityKind kind);
Result<void> UpgradeFacility(PlayerState& player, int facility_id);
bool IsRanchUnlocked(RanchFacilityKind kind, int player_level) const;
```

- 建造和升级会通过 `PlayerState::TrySpendGold` 扣金币。
- 设施等级范围按 1-10 设计。
- 当前 2 级解锁批量喂养和批量收获，5 级解锁自动收获标记。

### 动物操作

```cpp
Result<int> BuyAnimal(PlayerState& player, int facility_id, AnimalKind kind,
                      AnimalQuality quality);
Result<void> FeedAnimal(PlayerState& player, int facility_id, int animal_id,
                        int current_tick);
Result<void> HarvestAnimal(PlayerState& player, int facility_id, int animal_id);
Result<int> BatchFeed(PlayerState& player, int facility_id, int current_tick);
Result<int> BatchHarvest(PlayerState& player, int facility_id);
```

- `BuyAnimal` 只把动物加入指定设施，不会直接开始生产。
- `FeedAnimal` 会消耗饲料并让动物进入生产倒计时。
- `HarvestAnimal` 会把动物产品加入仓库，并让动物回到空闲状态。
- 批量操作需要设施等级达到解锁要求。

### 品质和产能接口

```cpp
bool IsAnimalProductUnlocked(ItemId product_id) const;
int EstimateDailyOutput(ItemId product_id) const;
float GetQualityMultiplier(AnimalQuality quality) const;
const AnimalQualityRule& GetQualityRule(AnimalQuality quality) const;
AnimalQuality RollQuality(AnimalKind kind, int ranch_level, int random_seed) const;
int ApplyOutputBonus(int base_quantity, AnimalQuality quality) const;
int ApplyExpBonus(int base_exp, AnimalQuality quality) const;
ItemId GetPrimaryProduct(AnimalKind kind, AnimalQuality quality) const;
int GetOrderValueBonusPercent(AnimalQuality quality) const;
```

这些接口用于订单生成、工厂推荐、UI 展示和后续存档联调。

其中前三个接口来自团队总分工文档：

- `IsAnimalProductUnlocked`
- `EstimateDailyOutput`
- `GetQualityMultiplier`

其余接口来自 `docs/ranch/README.md` 中的动物品质服务规划，用于把品质规则、产出倍率和产物映射集中放在养殖模块里。

### 设施等级规则接口

```cpp
const RanchFacilityLevelRule& GetLevelRule(RanchFacilityKind kind, int level) const;
int GetProductionTicks(RanchFacilityKind kind, int level, int base_ticks) const;
bool CanSpawnQuality(RanchFacilityKind kind, int level, AnimalQuality quality) const;
bool IsBatchFeedUnlocked(int facility_id) const;
bool IsBatchHarvestUnlocked(int facility_id) const;
bool IsAutoHarvestUnlocked(int facility_id) const;
```

这些是 `docs/ranch/README.md` 中养殖场等级服务规划的落地接口，用于让 UI、总控和后续订单产能估算读取等级规则。

当前等级效果：

- 设施等级越高，容量越高。
- 每升 1 级增加 5% 生产速度加成。
- 2 级允许优秀品质和批量操作。
- 4 级允许稀有品质。
- 5 级开放自动收获标记。

## 与其他模块的边界

### 依赖其他模块

- `PlayerState`：扣金币、加仓库、扣仓库。
- `ItemId`：读取当前已有物品类型。
- `AnimalState`：复用已有动物状态。

### 不直接修改的模块

本版本没有修改：

- 种植系统
- 工厂系统
- 订单系统
- 仓库底层实现
- 商店系统
- UI 面板
- 公共物品目录

## 当前限制

由于公共 `ItemId` 目前只有 `Egg`、`ChickenFeed`、`CowFeed` 等已有物品，还没有正式加入 `Milk`、`Wool`、`Pork` 等动物产品，本版本没有修改公共物品表。

因此：

- 鸡的完整闭环已经可以使用：买鸡、喂鸡饲料、等待、收获鸡蛋。
- 牛、猪、羊的设施和动物接口已经预留，可以购买、喂养、进入生产流程。
- 牛、猪、羊的真实产物需要等团队统一扩展公共 `ItemId` 和 `ItemCatalog` 后再接入。
- 当前 `GetPrimaryProduct` 暂时返回已有动物产品 `Egg`，避免越界修改其他同学负责的公共物品系统。
- 当前没有新增专门的养殖错误码。为了不修改公共 `ErrorCode`，统一动物接口暂时复用已有鸡圈错误码和通用错误码；如果团队允许扩展公共错误码，后续可替换为 `RanchFacilityOutOfRange`、`AnimalNotIdle` 等更准确的错误。

## 兼容性说明

为了不影响已有代码，本版本保留了原来的鸡圈接口：

```cpp
ChickenCoop& GetChickenCoop();
const ChickenCoop& GetChickenCoop() const;
```

已有 UI 或测试如果继续使用 `GetChickenCoop().FeedChicken()`、`CollectEgg()`，仍然可以正常工作。

新增接口是向后兼容扩展，不删除旧接口，不要求其他模块立即改用新接口。

## 建议后续工作

后续如果团队公共类型允许扩展，可以继续完成：

- 在 `ItemId` 中增加 `Milk`、`Pork`、`Wool` 等真实动物产品。
- 在 `ItemCatalog` 中配置这些动物产品的名称和售价。
- 将 `GetPrimaryProduct` 从临时映射改成真实动物产物映射。
- 给 UI 增加统一养殖场面板，读取 `GetAllFacilities()` 和 `GetAnimals()`。
- 给存档系统接入设施和动物快照。
- 给订单系统读取 `IsAnimalProductUnlocked()` 和 `EstimateDailyOutput()`。
