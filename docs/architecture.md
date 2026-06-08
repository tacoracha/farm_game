# Architecture

本次采用彻底重构但保留业务规则的方式。旧项目已经有种植、仓库、订单、鸡圈和饲料坊雏形，但根目录头文件、`include/farm`、ImGui UI 和构建产物混在一起，缺少存档和统一世界控制，继续扩展会让时间、库存和 UI 依赖越来越难讲清。

## 分层

UI Layer：`src/ui/Win32FarmApp.cpp`。只读取 View、调用公开业务接口、显示 `ErrorCode`。

Application/Core：`Game`、`TimeSystem`、`WeatherSystem`。`Game` 是跨模块调度者，也是唯一 tick 权威来源。

Business Systems：`PlantingSystem`、`RanchSystem`、`WorkshopSystem`、`OrderSystem`、`ShopSystem`。

Player State：`PlayerState` 统一金币、经验、仓库、物品锁和种子解锁。

Persistence：`SaveManager` 聚合所有模块状态，业务模块不直接读写文件。

## Tick 顺序

`Game::AdvanceTicks` 每 tick 按固定顺序执行：

1. `TimeSystem` 增加 tick。
2. `WeatherSystem` 更新天气剩余时间。
3. `PlantingSystem` 根据天气成长。
4. `RanchSystem` 更新动物生产。
5. `WorkshopSystem` 推进饲料坊队列。
6. `OrderSystem` 刷新冷却订单。

## 依赖规则

业务层不包含 Win32、ImGui、OpenGL 或 UI 文件。种植、养殖、工坊、订单都只能通过 `PlayerState` 扣物品、加物品、扣金币和加金币，避免直接修改容器。

## 存档聚合

`SaveManager` 保存 `Game` 快照到临时文件，写入成功后替换正式文件。读档先加载到临时 `Game`，全部校验成功后才替换当前游戏，避免坏档破坏内存状态。

