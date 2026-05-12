# gml - 仓库、商店接口规划

## 模块职责

gml 负责仓库、物品堆叠、物品保护锁、金币/工具等资产入口，以及商店购买。所有模块的物品和金币变化都应通过 gml 的接口或现有 `PlayerState` 封装完成。

## 对外提供接口

```cpp
enum class ItemCategory {
    Seed,
    Crop,
    AnimalProduct,
    FactoryProduct,
    Tool,
    Consumable,
    Feed,
};

struct ItemStack {
    ItemId item_id;
    int quantity;
};

struct ItemView {
    ItemId item_id;
    std::string_view name;
    ItemCategory category;
    int quantity;
    int sell_price;
    bool locked;
};

struct ShopItemView {
    ItemId item_id;
    int price_gold;
    int unlock_level;
    bool unlocked;
};

struct MarketShelfView {
    int slot_id;
    ItemId item_id;
    int quantity;
    int unit_price;
    bool sold_out;
    bool tool_or_rare_material;
};

class InventoryEconomyService {
public:
    int GetGold() const;
    Result<void> TrySpendGold(int amount);
    void AddGold(int amount);

    int GetItemCount(ItemId item_id) const;
    bool HasItem(ItemId item_id, int minimum_count) const;
    Result<void> TryAddItem(ItemId item_id, int quantity);
    Result<void> TryRemoveItem(ItemId item_id, int quantity);
    Result<void> TryRemoveItems(const std::vector<ItemStack>& items);

    void SetItemLocked(ItemId item_id, bool locked);
    bool IsItemLocked(ItemId item_id) const;

    int GetWarehouseCapacity() const;
    int GetWarehouseUsed() const;
    int GetWarehouseRemaining() const;
    bool IsWarehouseFull() const;
    std::vector<ItemView> GetItemsByCategory(ItemCategory category) const;

    Result<int> TrySellItem(ItemId item_id, int quantity);
    Result<void> UpgradeWarehouseCapacity();

    Result<void> BuyShopItem(ItemId item_id, int quantity);
    Result<void> BuySeed(ItemId seed_id, int quantity);
    Result<void> BuyFromShelf(int shelf_slot_id, int quantity);
    Result<void> ForceRefreshMarket(PlayerState& player);

    std::vector<ShopItemView> GetShopItems() const;
    std::vector<MarketShelfView> GetMarketShelves() const;
    int GetNextMarketRefreshTicks() const;
    float GetNextToolProbability() const;
    void OnTick(int tick_delta, int player_level);
};
```

## 需要其他人提供接口

- cdy：存档注册、等级解锁、商店刷新时间或活动折扣。
- zxc：种子、肥料、作物基础配置和解锁状态。
- zgm：饲料、动物产品、养殖工具配置和解锁状态。
- zjx：工厂工具、配方碎片、加工品配置和解锁状态。
- lly：订单奖励工具和订单交付前的材料保护锁提示。

## 给其他模块的联调约定

- 任何模块不得绕过仓库接口直接改库存。
- 关键材料被锁定时，出售、订单交付、工厂消耗都应失败或要求二次确认。
- 商店只负责购买入口，购买后的物品仍通过仓库容量校验入库。
- 集市货架由商店系统周期刷新，价格可按成本价乘以随机系数生成，但对外只暴露最终价格。
- 仓库扩容、出售和购买都应返回明确失败原因：金币不足、容量不足、物品锁定、数量不足。
- 当前代码已有 `Warehouse` 和 `PlayerState`，正式实现时可把 `InventoryEconomyService` 拆为现有类上的方法扩展。
