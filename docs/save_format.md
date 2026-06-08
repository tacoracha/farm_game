# 存档格式

存档文件：`saves/save01.farm`。

当前版本：`FARM_SAVE 2`。

格式是文本行协议，便于调试和课程答辩解释。示例：

```text
FARM_SAVE 2
TIME 12 1 12
WEATHER 0 8
EVENT 0
PLAYER 80 1 0 40
ITEM 0 4 1 1
...
UNLOCKS 3
UNLOCK 0
PLOTS 6
PLOT 1 0 3 0 3 0 0
RANCH 2 1
FACILITY 1 0 1 3 1
ANIMAL 1 0 0 1 20
WORKSHOP 0 0 1
JOB 0 2
ORDERS 5 4 4
ORDER 1 0 3 2 17 6 0 0
REALTIME 1780930000
END
```

## 字段说明

- `TIME`：当前 tick、速度、上次自动存档 tick。
- `WEATHER`：天气枚举值、剩余 tick。
- `EVENT`：上一次随机事件触发 tick。
- `PLAYER`：金币、等级、经验、仓库容量。
- `ITEM`：物品 id、数量、是否锁定、种子是否解锁。
- `UNLOCKS/UNLOCK`：DAG 解锁节点集合，包括种子、土地、设施和动物。
- `PLOT`：地块状态、水分、作物、种植 tick、成熟 tick、是否施肥、成长余量。
- `RANCH/FACILITY/ANIMAL`：养殖设施和动物状态。
- `WORKSHOP/JOB`：饲料坊鸡饲料货架、牛饲料货架和生产队列。
- `ORDERS/ORDER`：订单槽、奖励、冷却和锁定状态。
- `REALTIME`：保存时的 Unix 秒数，用于离线进度结算。

## 安全流程

保存时写入 `save01.farm.tmp`，关闭并检查流状态后替换正式文件。读取时先构造临时 `Game`，只有完整读到 `END` 且版本匹配时才替换当前游戏。

## 离线进度

读档时根据 `REALTIME` 计算离线秒数。当前规则为 60 秒折算 1 tick，最多补算 48 tick，避免长时间离线导致状态爆炸。

## 错误处理

- 文件不存在：`SaveOpenFailed`
- 魔数错误、字段缺失：`SaveCorrupted`
- 版本不兼容：`SaveVersionMismatch`
- 写入或替换失败：`SaveWriteFailed`
