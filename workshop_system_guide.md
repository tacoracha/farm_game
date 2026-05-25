# 农场工厂系统运行逻辑文档

> 模块负责人：zjx  
> 文件：`include/farm/WorkshopSystem.h` / `src/WorkshopSystem.cpp` / `src/ui/panels/WorkshopPanel.cpp`

---

## 一、系统概述

工厂系统（WorkshopSystem）是农场游戏的核心加工模块。玩家使用金币建造工厂建筑，通过消耗仓库中的原材料，按配方排队生产加工品，完成后进入工厂货架，手动领取到仓库。

**核心数据流**：原材料（仓库） → 生产槽位（计时） → 完成进入货架 → 领取入库

---

## 二、四种工厂

| 工厂类型 | 枚举值 | 初始槽位 | 初始队列长度 | 初始货架容量 | 建造费用 |
|----------|--------|----------|-------------|-------------|----------|
| 饲料坊 | `FeedMill` | 1 | 3 | 10 | 100 gold |
| 乳品坊 | `Dairy` | 2 | 3 | 10 | 100 gold |
| 烘焙屋 | `Bakery` | 2 | 3 | 10 | 100 gold |
| 纺织间 | `Textile` | 2 | 3 | 10 | 100 gold |

饲料坊初始只有 1 个槽位（因为早期饲料需求较低），其他三种工厂初始有 2 个槽位。

---

## 三、等级解锁速查表

### 3.1 饲料坊（Feed Mill）

| 等级 | 解锁配方 | 单次产量 | 生产耗时 | 消耗材料 | 经验 |
|------|---------|---------|---------|---------|------|
| Lv.1 | Chicken Feed | x1 | 100 ticks | Wheat x2 | 10 |
| Lv.1 | Cow Feed | x1 | 150 ticks | Corn x2 + Carrot x1 | 15 |
| Lv.2 | — | x2 | 不变 | 不变 | — |
| Lv.3 | — | x3 | 不变 | 不变 | — |
| Lv.4 | — | x4 | 不变 | 不变 | — |
| Lv.5 | — | x5 | 不变 | 不变 | — |

> 饲料坊仅有 Lv.1 的两个配方。等级提升的唯一增益是 **单次产量倍率增加**（x1→x2→x3...）。

### 3.2 乳品坊（Dairy）

| 等级 | 解锁配方 | 单次产量 | 生产耗时 | 消耗材料 | 经验 |
|------|---------|---------|---------|---------|------|
| Lv.1 | —（无可用配方） | x1 | — | — | — |
| Lv.2 | **Cheese** | x2 | 200 ticks | Egg x3 | 20 |
| Lv.3 | **Butter** | x3 | 180 ticks | Egg x2 | 25 |
| Lv.4 | — | x4 | 不变 | 不变 | — |
| Lv.5 | — | x5 | 不变 | 不变 | — |

> 乳品坊 Lv.1 无法生产任何物品，需尽快升到 Lv.2 解锁 Cheese。

### 3.3 烘焙屋（Bakery）

| 等级 | 解锁配方 | 单次产量 | 生产耗时 | 消耗材料 | 经验 |
|------|---------|---------|---------|---------|------|
| Lv.1 | **Bread** | x1 | 120 ticks | Wheat x3 | 12 |
| Lv.2 | — | x2 | 不变 | 不变 | — |
| Lv.3 | — | x3 | 不变 | 不变 | — |
| Lv.4 | **Cake** | x4 | 250 ticks | Wheat x2 + Egg x1 | 30 |
| Lv.5 | — | x5 | 不变 | 不变 | — |

> 烘焙屋早期产出 Bread，Lv.4 解锁高价值 Cake。

### 3.4 纺织间（Textile）

| 等级 | 解锁配方 | 单次产量 | 生产耗时 | 消耗材料 | 经验 |
|------|---------|---------|---------|---------|------|
| Lv.1 | —（无可用配方） | x1 | — | — | — |
| Lv.2 | **Yarn** | x2 | 160 ticks | Wheat x2 | 18 |
| Lv.3 | **Cloth** | x3 | 220 ticks | Yarn x3 | 22 |
| Lv.4 | — | x4 | 不变 | 不变 | — |
| Lv.5 | — | x5 | 不变 | 不变 | — |

> 纺织间 Lv.1 无可用配方，需升到 Lv.2。Cloth 依赖 Yarn 作为原材料（生产链：Wheat → Yarn → Cloth）。

---

## 四、配方数据库（8 种配方）

### 4.1 饲料坊配方

| 配方 | 输入材料 | 产出 | 生产时间 | 解锁等级 | 经验奖励 |
|------|---------|------|---------|---------|---------|
| **Chicken Feed** | 2×Wheat | 1×ChickenFeed | 100 ticks | Lv.1 | 10 exp |
| **Cow Feed** | 2×Corn + 1×Carrot | 1×CowFeed | 150 ticks | Lv.1 | 15 exp |

### 4.2 乳品坊配方

| 配方 | 输入材料 | 产出 | 生产时间 | 解锁等级 | 经验奖励 |
|------|---------|------|---------|---------|---------|
| **Cheese** | 3×Egg | 1×Cheese | 200 ticks | Lv.2 | 20 exp |
| **Butter** | 2×Egg | 1×Butter | 180 ticks | Lv.3 | 25 exp |

### 4.3 烘焙屋配方

| 配方 | 输入材料 | 产出 | 生产时间 | 解锁等级 | 经验奖励 |
|------|---------|------|---------|---------|---------|
| **Bread** | 3×Wheat | 1×Bread | 120 ticks | Lv.1 | 12 exp |
| **Cake** | 2×Wheat + 1×Egg | 1×Cake | 250 ticks | Lv.4 | 30 exp |

### 4.4 纺织间配方

| 配方 | 输入材料 | 产出 | 生产时间 | 解锁等级 | 经验奖励 |
|------|---------|------|---------|---------|---------|
| **Yarn** | 2×Wheat | 1×Yarn | 160 ticks | Lv.2 | 18 exp |
| **Cloth** | 3×Yarn | 1×Cloth | 220 ticks | Lv.3 | 22 exp |

> **配方解锁规则**：配方的 `unlock_level` ≤ 工厂当前等级时可用。  
> **产量计算**：实际产出数量 = 配方基础产出 × 工厂产出倍率（`output_multiplier = level`）。

---

## 五、生产流程详解

### 5.1 完整生产周期

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│  1. 发起生产  │ → │  2. 排队等待  │ → │  3. 槽位生产  │ → │  4. 完成入架  │
│ StartProduction│   │  生产队列    │   │  倒计时递减   │   │  Shelf 货架  │
└──────────────┘    └──────────────┘    └──────────────┘    └──────┬───────┘
                                                                    │
                                                              ┌─────▼───────┐
                                                              │ 5. 领取入库  │
                                                              │ ClaimProduct│
                                                              └─────────────┘
```

### 5.2 各阶段详解

**阶段 1：发起生产 (`StartProduction`)**
- 检查工厂存在、配方匹配、生产次数 > 0
- 检查队列是否已满（队列长度 ≥ `max_queue_length`）
- 将生产任务加入队列末尾（FIFO 按加入顺序排队）
- 生产次数 `times` 支持 1~10 次批量排产
- **此阶段不扣材料**

**阶段 2：排队等待**
- 队列中的任务按优先级排序（priority 越小越优先）
- 普通任务 priority = 队列入队序号（FIFO）
- 插队任务 priority = -1（最高优先级，排到队首）
- 槽位空闲时，从队列中弹出优先级最高的任务开始生产

**阶段 3：槽位生产**
- 每个 Tick，运行中槽位的 `remaining_ticks` 减 1
- 归零时标记 `completed = true`，`batch_completed++`
- 将产出自动放入货架 Shelf
- 工厂获得该配方的 `exp_reward` 经验值
- 如果是批量生产且还有剩余批次，自动开始下一批次
- **下一批次不需要重新消耗材料**（材料在首次开始生产时已扣除）

**阶段 4：货架暂存**
- 相同 ItemId 的产出会堆叠在同一货架格
- 货架满（shelf.size() ≥ shelf_capacity）时暂停后续产出结算
- 避免成品因无空间而丢失

**阶段 5：领取入库 (`ClaimProduct` / `ClaimAllProducts`)**
- 从货架取出产品，调用 `PlayerState::TryAddToWarehouse`
- 仓库满时领取失败（产品保留在货架）
- `ClaimAllProducts` 返回成功领取的数量

---

## 六、升级与熟练度系统

### 6.1 升级方式

**方式一：主动升级 (`UpgradeFactory`)**
- 消耗金币：`工厂当前等级 × 50` gold
- 提升一级
- 同时获得所有升级奖励（队列扩容、货架扩容、速度提升）

**方式二：熟练度自动升级 (`CheckLevelUp`)**
- 每完成一次生产，工厂获得该配方的 `exp_reward` 经验值
- 升级所需经验 = `当前等级 × 100`
- 经验达标后自动升级（扣除所需经验，溢出经验保留）
- 同时获得所有升级奖励

### 6.2 每级升级奖励

| 属性 | 每级增量 | 上限 |
|------|---------|------|
| 等级 `level` | +1 | 无上限 |
| 单次产量 `output_multiplier` | = level（Lv.1=x1, Lv.2=x2, ...） | 无上限 |
| 队列长度 `max_queue_length` | +1 | 10 |
| 货架容量 `shelf_capacity` | +5 | 50 |

### 6.3 产量加成计算

```
实际产出数量 = 配方基础产出数量 × output_multiplier
```

- Lv.1 生产 Chicken Feed：产出 1×1 = 1 个
- Lv.3 生产 Chicken Feed：产出 1×3 = 3 个
- Lv.5 生产 Cake：产出 1×5 = 5 个

**产量加成只影响最终产出数量，不影响：**
- 生产耗时（始终使用配方基础 ticks）
- 材料消耗（始终使用配方基础输入数量）
- 队列容量

---

## 七、安全机制

### 7.1 材料锁 (Material Protection)

工厂在生产过程中，会将正在使用的材料标记为"已锁定"（`material_lock_counts`），防止：
- 其他工厂同时消耗同一材料导致缺料
- 玩家误将正在生产的材料卖出

查询接口：`IsMaterialProtected(player, item_id)` — 返回该材料是否被任一工厂锁定。

### 7.2 高价值材料二次确认

`IsHighValueMaterial(item_id)` 返回 true 时，前端应弹出确认框。

当前标记为高价值的物品：
- **Cake**（蛋糕）— 消耗 Wheat + Egg，生产时间长
- **Cloth**（布料）— 消耗 Yarn，生产链长

### 7.3 生产队列保护

- 生产中不可拆除建筑（外部模块通过 `GetProductionSlots` 检查是否有 running 槽位）
- 货架满时暂停后续完成结算，避免成品丢失
- 取消未开始的队列任务时退还材料（`CancelQueuedProduction` 目前移除队列项，材料退还由外部调度）

---

## 八、批量生产逻辑

### 8.1 发起批量生产

```cpp
StartProduction(player, factory_id, RecipeId::ChickenFeed, 5, current_tick);
```

- `times = 5` 表示生产 5 批次的 Chicken Feed
- 该任务进入队列时记录 `times` 数量
- 只占用一个队列位置（不是 5 个）

### 8.2 批量执行过程

```
槽位开始 → 扣材料 → 倒计时 → 完成1批 → 入货架 → 获得经验
                                            ↓
                              batch_completed < batch_total ?
                                            ↓ 是
                        自动继续 → 倒计时 → 完成2批 → ...
                                            ↓ 否
                              槽位空闲，从队列取下一任务
```

- **关键**：批量生产的多批次是连续在同一个槽位执行的，批次之间不重新排队
- 每批次独立触发经验奖励和等级检查
- 每批次产出独立放入货架

---

## 九、高级操作接口

### 9.1 插队生产 (`InsertProductionAtFront`)
将紧急任务插入队列最前端（priority = -1），跳过所有排队任务。

### 9.2 替换队列任务 (`ReplaceQueuedProduction`)
修改已排队但尚未开始的任务的配方和次数，不改变其在队列中的位置。

### 9.3 一键快速生产 (`QuickStartProduction`)
自动选择工厂第一个可用配方，以 1 次生产量启动。

### 9.4 自动领取 (`SetAutoCollect`)
开启后，成品自动从货架转移到仓库（依赖 Tick 调度，需要外部在处理时传入 PlayerState）。

---

## 十、离线收益

```cpp
ProcessOfflineTicks(player, offline_ticks)
```

离线时由其他模块（cdy）计算离线 tick 数，工厂模拟这些 tick 内的生产进度：
- 所有运行中槽位的倒计时继续推进
- 完成的产出正常进入货架
- 注意：离线期间不触发自动领取（货架可能满）

---

## 十一、联动接口（给其他模块使用）

| 接口 | 使用方 | 用途 |
|------|--------|------|
| `GetFactories()` | UI / 订单系统 | 获取所有工厂状态 |
| `GetAvailableRecipes(factory_id)` | UI | 显示可生产配方列表 |
| `GetProductionSlots(factory_id)` | UI | 显示槽位生产进度 |
| `GetShelfItemCount(factory_id)` | UI | 货架红点提示 |
| `GetOutputMultiplier(factory_id)` | UI / 所有模块 | 获取当前工厂产量倍率 |
| `GetSpeedMultiplier(factory_id)` | (废弃) | 始终返回 1.0f |
| `HasAvailableFeedRecipe(feed_id)` | 养殖系统 | 检查是否有配方能产出指定饲料 |
| `IsFactoryProductUnlocked(product_id)` | 订单系统(lly) | 检查加工品是否已解锁，决定是否生成相关订单 |
| `GetFactoryLevel(factory_id)` | UI | 获取工厂等级 |
| `GetProficiencyExp(factory_id)` | UI | 获取熟练度进度 |
| `IsHighValueMaterial(item_id)` | UI | 是否弹出二次确认 |
| `IsMaterialProtected(player, item_id)` | UI | 材料是否被锁定 |

---

## 十二、核心数据结构速查

### FactoryView（工厂快照，只读）
```cpp
factory_id, kind, level, queue_size, shelf_used,
shelf_capacity, max_queue_length, proficiency_exp
```

### RecipeView（配方快照，只读）
```cpp
recipe_id, factory_kind, name, inputs (ItemStack[]),
outputs (ItemStack[]), production_ticks, unlock_level, exp_reward
```

### ProductionSlotView（槽位快照，只读）
```cpp
slot_id, recipe_id, remaining_ticks, batch_remaining, running
```

### FactoryInternal（内部状态，不对外暴露）
```cpp
factory_id, kind, level, proficiency_exp, max_queue_length,
shelf_capacity, output_multiplier, auto_collect,
slots (ProductionSlot[]), queue (ProductionQueueItem[]),
shelf (ShelfItem[]), material_lock_counts (map<ItemId, int>)
```

---

## 十三、错误码

生产过程中可能返回的错误：

| 错误码 | 触发场景 |
|--------|---------|
| `InsufficientGold` | 建造/升级时金币不足 |
| `InsufficientItem` | 开始生产时材料不足 |
| `WarehouseFull` | 领取时仓库满 / 队列已满 |
| `InvalidItem` | 配方与工厂类型不匹配 |
| `InvalidQuantity` | 生产次数 ≤ 0 |
| `OrderSlotOutOfRange` | 队列索引或货架索引越界 |
| `InternalError` | 工厂不存在或内部异常 |
