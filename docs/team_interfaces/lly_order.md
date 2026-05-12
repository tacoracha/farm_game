# lly - 订单接口规划

## 模块职责

lly 负责常驻订单板、订单生成、订单状态、交付、放弃、冷却和奖励发放。订单系统串联种植、养殖、工厂、仓库和经济系统。

## 对外提供接口

```cpp
enum class OrderState {
    Available,
    WaitingMaterials,
    ReadyToDeliver,
    CoolingDown,
    Locked,
};

enum class OrderRewardType {
    Gold,
    Tool,
    Mixed,
};

enum class OrderType {
    Basic,
    Mixed,
    HighValue,
    Urgent,
    ChainStep,
};

struct OrderRequirement {
    ItemId item_id;
    int quantity;
};

struct OrderReward {
    int gold;
    std::vector<ItemStack> tools;
};

struct OrderView {
    int order_id;
    OrderType type;
    OrderState state;
    std::vector<OrderRequirement> requirements;
    OrderReward reward;
    int cooldown_end_tick;
    bool locked;
    int chain_step;
    int total_chain_steps;
};

class OrderSystem {
public:
    void Init(int max_concurrent_orders = 10);
    void Tick(int current_tick);
    void OnTick(int tick_delta, int player_level, const PlayerState& player);

    const std::vector<OrderView>& GetOrdersForDisplay() const;
    OrderView GetOrder(int order_id) const;
    std::vector<int> GetDeliverableOrderIds(const PlayerState& player) const;

    Result<void> CompleteOrder(PlayerState& player, int order_id);
    Result<void> AbandonOrder(int order_id, int current_tick);
    Result<void> LockOrder(int order_id, bool locked);
    Result<void> AccelerateCooldown(PlayerState& player, int order_id, int minutes);

    void RefreshOrder(int slot_index);
    Result<int> GenerateUrgentOrder(int player_level, const PlayerState& player);
    Result<int> StartChain(int player_level, const PlayerState& player);
    Result<int> AdvanceChain(int current_order_id);

    int GetCompletedCountToday() const;
    int GetTotalExpEarnedToday() const;
    bool CanGenerateItem(ItemId item_id) const;
};
```

## 需要其他人提供接口

- cdy：当前时间、订单冷却 Tick、等级解锁、存档注册。
- gml：查询库存、扣除交付物品、发放金币和工具、材料保护锁校验。
- zxc：农作物解锁和产能估算。
- zgm：动物产品解锁、品质产物、养殖产能估算。
- zjx：加工品解锁、配方产能、工厂货架/队列状态。

## 给其他模块的联调约定

- 订单永久有效，不自动顶掉；完成或放弃后进入冷却。
- 订单生成只抽取已解锁且当前产能可达的物品。
- 订单交付只通过 gml/`PlayerState` 扣物品和发奖励，不直接改仓库内部结构。
- 放弃订单建议进入较长冷却；完成订单进入较短冷却。
- 订单链由订单系统维护步骤，最终奖励仍通过 gml/cdy 发放金币、工具和经验。
- 限时紧急订单可由 cdy 随机事件触发，订单系统负责生成和状态展示。
