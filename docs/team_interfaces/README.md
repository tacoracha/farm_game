# Team Interface README Index

本目录按群公告分工整理接口规划。每个 README 只定义模块边界和联调接口，不要求立即实现。

## 分工链接

| 负责人 | 模块 | README |
| --- | --- | --- |
| cdy | 时间、天气、存档、总体调控 | [cdy_time_weather_save_control.md](cdy_time_weather_save_control.md) |
| zgm | 养殖 | [zgm_ranch.md](zgm_ranch.md) |
| zxc | 种植 | [zxc_planting.md](zxc_planting.md) |
| zjx | 工厂 | [zjx_workshop.md](zjx_workshop.md) |
| lly | 订单 | [lly_order.md](lly_order.md) |
| gml | 仓库、商店 | [gml_warehouse_shop.md](gml_warehouse_shop.md) |

## 全局约定

- 所有金币、物品、仓库变化统一通过 `PlayerState` 或 gml 提供的资产接口完成。
- 所有生产、冷却、刷新时间统一以 cdy 的时间接口为准。
- 各模块只暴露查询视图和操作接口，不直接修改其他模块内部数据。
- 接口返回值建议统一使用 `Result<T>` 或 `Result<void>`，失败原因使用 `ErrorCode`。
- `ItemId`、模块类型枚举、奖励结构等公共类型应放在公共头文件，避免各模块重复定义。
- 逻辑层 `farm/` 禁止包含 ImGui/UI 代码；UI 层只能调用逻辑层公开接口。
- 当前仓库已有 `Types.h`、`Constants.h`、`ItemCatalog.h`、`PlayerState.h`、`Game.h`，新增接口优先贴合这些文件，而不是重新定义一套资源系统。

## 建议公共类型

这些类型会被多个负责人共用，建议最终放到公共头文件中，并和现有 `ItemId` / `Result<T>` 合并：

```cpp
enum class ItemCategory {
    Seed,
    Crop,
    AnimalProduct,
    Processed,
    Feed,
    Tool,
    Consumable,
    UpgradeMaterial,
};

struct ItemStack {
    ItemId item_id;
    int quantity;
};

struct TransactionResult {
    bool success;
    int gold_delta;
    int exp_delta;
    std::vector<ItemStack> item_delta;
    ErrorCode error;
};
```

## 当前代码对接提醒

- 当前 `PlantingSystem::TryPlant` 自动选择第一个空闲田地；如果要指定地块，请新增 `TryPlantAt`，不要直接破坏旧接口。
- 当前 `WorkshopSystem` 只有 `FeedMill`，扩展工厂时建议保留 `StartProduction -> Tick -> Collect` 的同类接口形状。
- 当前 `RanchSystem` 只有 `ChickenCoop`，扩展牛棚/猪圈/羊圈时建议统一 `facility_id` / `animal_id` 查询和操作。
- 当前 `OrderSystem` 是固定槽位、单物品订单；扩展多物品、放弃、冷却、订单链时建议保留现有 `CompleteOrder(PlayerState&, slot_index)` 兼容入口。
