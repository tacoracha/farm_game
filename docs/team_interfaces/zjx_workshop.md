# zjx - 工厂接口规划

## 模块职责

zjx 负责饲料坊、乳品坊、烘焙屋、纺织间等加工建筑。工厂消耗仓库原材料，按配方和队列生产加工品，完成后进入仓库或建筑货架。

## 对外提供接口

```cpp
enum class FactoryKind {
    FeedMill,
    Dairy,
    Bakery,
    Textile,
};

enum class RecipeId {
    ChickenFeed,
    CowFeed,
    Cheese,
    Butter,
    Bread,
    Cake,
    Yarn,
    Cloth,
};

struct RecipeView {
    RecipeId recipe_id;
    FactoryKind factory_kind;
    std::string_view name;
    std::vector<ItemStack> inputs;
    std::vector<ItemStack> outputs;
    int production_ticks;
    int unlock_level;
    int exp_reward;
};

struct ProductionSlotView {
    int slot_id;
    RecipeId recipe_id;
    int remaining_ticks;
    int batch_remaining;
    bool running;
};

struct FactoryView {
    int factory_id;
    FactoryKind kind;
    int level;
    int queue_size;
    int shelf_used;
    int shelf_capacity;
    int max_queue_length;
    int proficiency_exp;
};

class WorkshopSystem {
public:
    void Init();
    void Tick(int current_tick);
    void OnTick(int tick_delta);
    Result<void> ProcessOfflineTicks(PlayerState& player, int offline_ticks);

    std::vector<FactoryView> GetFactories() const;
    std::vector<RecipeView> GetAvailableRecipes(int factory_id) const;
    std::vector<ProductionSlotView> GetProductionSlots(int factory_id) const;
    int GetShelfItemCount(int factory_id) const;
    float GetSpeedMultiplier(int factory_id) const;

    Result<void> BuildFactory(PlayerState& player, FactoryKind kind);
    Result<void> UpgradeFactory(PlayerState& player, int factory_id);
    Result<void> StartProduction(PlayerState& player, int factory_id, RecipeId recipe_id,
                                 int times, int current_tick);
    Result<void> CancelQueuedProduction(PlayerState& player, int factory_id, int queue_id);
    Result<void> ClaimProduct(PlayerState& player, int factory_id, int shelf_slot);
    Result<int> ClaimAllProducts(PlayerState& player, int factory_id);

    bool HasAvailableFeedRecipe(ItemId feed_id) const;
    bool IsFactoryProductUnlocked(ItemId product_id) const;
};
```

## 需要其他人提供接口

- cdy：`CurrentTick()`、`Tick` 调度、离线生产结算、存档注册。
- gml：扣除原材料、领取成品入库、升级消耗金币和工具、材料保护锁。
- zxc：农作物产能和解锁情况，供饲料坊/烘焙屋缺料提示。
- zgm：动物产品产能和解锁情况，供乳品坊/烘焙屋/纺织间缺料提示。
- lly：订单读取 `IsFactoryProductUnlocked`，并按工厂产能生成加工品订单。

## 给其他模块的联调约定

- 工厂只负责生产和货架，不直接完成订单。
- 开始生产时扣材料，领取时加成品；取消未开始队列时可退材料。
- 高级配方解锁由总体调控或等级系统通知，工厂只读解锁状态。
- 生产完成先进入工厂货架；货架满时暂停后续完成结算，避免成品丢失。
- 批量生产次数建议支持 `1/5/10`，但接口保留 `times` 方便调参。
- 离线收益由 cdy 计算离线 tick 后调用 `ProcessOfflineTicks`。
