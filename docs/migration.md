# Migration

## 保留

- `docs/team_interfaces/` 全部保留，作为团队边界和需求依据。
- `docs/ranch/` 保留，作为 zgm 后续扩展参考。

## 迁移/改写

- 旧 `PlayerState`、`Warehouse` 规则迁移到 `include/farm/inventory/PlayerState.h`。
- 旧 `PlantingSystem` 的指定地块、浇水、施肥、仓库满保留成熟作物规则迁移到 `planting`。
- 旧 `ChickenCoop` 和 `RanchSystem` 合并为统一 `RanchSystem`，核心版本实现鸡圈。
- 旧 `FeedMill` 改写为 `WorkshopSystem`，从单槽升级为队列加货架。
- 旧 `OrderSystem` 改写为带冷却和锁定的固定槽订单。
- 旧 ImGui UI 改写为 Win32/GDI UI，降低第三方依赖和 MinGW 发布复杂度。

## 删除

根目录散落头文件、旧 ImGui UI、旧 demo、旧单文件工坊说明已被新结构替代。删除原因是避免重复接口、错误包含路径和构建目标混乱。

## 旧接口映射

| 旧接口 | 新接口 |
| --- | --- |
| `Game::AdvanceTick` | `Game::AdvanceTicks` / `AdvanceBySpeed` |
| `PlayerState::TryAddToWarehouse` | `PlayerState::TryAddItem` |
| `PlayerState::TryRemoveFromWarehouse` | `PlayerState::TryRemoveItem` |
| `ShopSystem::BuySeed` | `ShopSystem::BuyItem` |
| `FeedMill::StartProduction` | `WorkshopSystem::StartProduction` |
| `ChickenCoop::FeedChicken` | `RanchSystem::FeedAnimal` |
| `OrderSystem::CompleteOrder` | `OrderSystem::CompleteOrder(player, slot, tick)` |

