# Order

负责人：lly。

职责：固定订单槽、单物品订单、奖励、可交付判断、完成、放弃、冷却、刷新和锁定。

主要类：`OrderSystem`。

主要文件：`include/farm/order/OrderSystem.h`、`src/order/OrderSystem.cpp`。

拥有数据：订单槽、下一订单 id、生成序列。

对外接口：`Orders`、`CanDeliver`、`CompleteOrder`、`AbandonOrder`、`SetLocked`、`Tick`。

依赖：`PlayerState` 扣交付物、加金币和经验。

被依赖：`Game`、UI、存档、测试。

状态流转：`Available -> CoolingDown -> Available`；锁定时为 `Locked`。

存档字段：`ORDERS`、`ORDER`。

错误码：`OrderSlotOutOfRange`、`OrderCoolingDown`、`OrderLocked`、`InsufficientItem`、`ProtectedItem`。

测试点：生成订单、库存不足、完成订单、奖励、放弃冷却、保存恢复。

典型流程：仓库有鸡蛋 -> `CompleteOrder(player, slot, tick)` -> 扣鸡蛋 -> 加金币经验 -> 订单冷却。

扩展：多物品订单可把 `OrderData::item/quantity` 替换为 `vector<ItemStack>`，扣除仍调用 `PlayerState::TryRemoveItems`。

