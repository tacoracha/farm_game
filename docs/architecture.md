# 总体架构

本次采用彻底重构但保留业务规则的方式。旧项目已有种植、仓库、订单、鸡圈和饲料坊雏形，但根目录头文件、`include/farm`、旧 UI 和构建产物混在一起，缺少存档和统一世界控制，继续扩展会让时间、库存和 UI 依赖越来越难维护。

## 分层

UI Layer：`src/ui/Win32FarmApp.cpp` 和 `TextureManager`。只读取 View、调用公开业务接口、显示 `ErrorCode`，并负责贴图加载。

Application/Core：`Game`、`TimeSystem`、`WeatherSystem`。`Game` 是跨模块调度者，也是唯一 tick 权威来源。

Business Systems：`PlantingSystem`、`RanchSystem`、`WorkshopSystem`、`OrderSystem`、`ShopSystem`。

Player State：`PlayerState` 统一金币、经验、仓库、物品锁和种子解锁。

Persistence：`SaveManager` 聚合所有模块状态，业务模块不直接读写文件。

## 核心继承层级

养殖领域使用两层继承，并通过虚函数提供实体分类和产物就绪判断：

```text
FarmEntity（农场实体抽象基类）
  -> RanchEntity（养殖实体抽象基类）
      -> AnimalData（动物数据）
      -> RanchFacilityData（养殖设施数据）
```

`AnimalData` 和 `RanchFacilityData` 分别重写 `RanchType` 与 `HasReadyOutput`。
原有公开字段、业务接口和存档字段保持不变，因此 UI 不需要感知继承结构。

## Tick 顺序

`Game::AdvanceTicks` 每 tick 按固定顺序执行：

1. `TimeSystem` 增加 tick。
2. `WeatherSystem` 更新天气剩余时间。
3. `PlantingSystem` 根据天气成长。
4. `RanchSystem` 更新动物生产。
5. `WorkshopSystem` 推进饲料坊队列。
6. `OrderSystem` 刷新冷却订单。
7. `Game` 检查随机事件冷却，必要时发放金币、肥料或临时天气事件。

## 依赖规则

业务层不包含 Win32、ImGui、OpenGL 或 UI 文件。种植、养殖、工坊、订单都只能通过 `PlayerState` 扣物品、加物品、扣金币和加金币，避免直接修改容器。

## 存档聚合

`SaveManager` 保存 `Game` 快照到临时文件，写入成功后替换正式文件。读档先加载到临时 `Game`，全部校验成功后才替换当前游戏，避免坏档破坏内存状态。

## 随机事件与离线进度

随机事件由 `Game` 统一调度，每隔固定 tick 触发一次轻量事件。离线进度由 `SaveManager` 读取 `REALTIME` 后调用 `Game::ProcessOfflineSeconds`，按有限 tick 补算。

## 时间系统

当前 1 tick 表示 2 分钟游戏时间。Win32 UI 每 1000ms 推进一次，1x 为每秒 2 游戏分钟，2x 为每秒 4 游戏分钟，4x 为每秒 8 游戏分钟。`TimeSystem::Snapshot` 负责把 tick 转换为第几天、小时和分钟。

## DAG 解锁

`UnlockGraph` 使用节点和前置节点表达解锁关系。示例链路：

```text
小麦种子
  -> 玉米种子
      -> 胡萝卜种子
          -> 番茄种子

玉米种子 -> 扩建土地
胡萝卜种子 + 鸡圈 -> 牛棚 -> 牛 -> 羊圈 -> 羊
```

解锁节点由 `PlayerState` 保存。商店、土地购买和动物购买都通过 `PlayerState::IsUnlocked` 或 `IsSeedUnlocked` 检查。

## 贴图管线

`TextureManager` 只存在于 UI 层。它优先加载 PNG，失败后回退到 PPM，再失败则使用 GDI 色块。后续更换贴图只需要覆盖 `assets/textures/*.png` 或 `assets/icons/*.png`，不影响业务代码和测试。
