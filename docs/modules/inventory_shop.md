# Inventory And Shop

负责人：gml。

职责：金币、经验、仓库容量、物品堆叠、物品锁、出售、商店购买。

主要类：`PlayerState`、`ShopSystem`。

主要文件：`include/farm/inventory/PlayerState.h`、`src/inventory/PlayerState.cpp`、`include/farm/shop/ShopSystem.h`、`src/shop/ShopSystem.cpp`。

拥有数据：金币、等级、经验、仓库容量、物品 map、锁定物品、已解锁种子。

对外接口：`TryAddItem`、`TryRemoveItem`、`TrySpendGold`、`TrySellItem`、`BuyItem`、`InventoryView`。

依赖：公共物品配置。

被依赖：种植、养殖、工坊、订单、UI、存档。

状态流转：购买先检查金币和容量，再扣金币，最后入库；入库失败会退金币。出售先检查锁和价格，再扣物品加金币。

存档字段：`PLAYER`、每个 `ITEM` 的数量、锁、解锁状态。

错误码：`InsufficientGold`、`InsufficientItem`、`WarehouseFull`、`ProtectedItem`、`CannotSell`、`InvalidQuantity`。

测试点：添加、扣除、容量满、金币不足、购买失败不扣钱、物品锁。

典型流程：`ShopSystem::BuyItem(player, WheatSeed, 1)` -> 检查 -> `TrySpendGold` -> `TryAddItem`。

扩展：市场货架、折扣和工具材料可以继续放在 `ShopSystem`，但入库和金币仍走 `PlayerState`。

