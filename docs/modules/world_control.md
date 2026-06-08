# World Control

负责人：cdy。

职责：维护唯一全局时间、暂停/速度、天气切换、跨模块 tick 顺序、玩家经验等级入口、随机事件、离线进度和存档调度。

主要类：`Game`、`TimeSystem`、`WeatherSystem`。

主要文件：`include/farm/core/Game.h`、`src/core/Game.cpp`。

拥有数据：当前 tick、速度、天气、天气剩余 tick、自动存档 tick、随机事件 tick、最近事件消息。

对外接口：`AdvanceTicks`、`AdvanceBySpeed`、`ManualSave`、`Load`、`AutoSaveIfNeeded`、`ProcessOfflineSeconds`、`Time().Snapshot()`、`Weather().Snapshot()`。

依赖：种植、养殖、工坊、订单、玩家资产、存档。

被依赖：UI、测试、`SaveManager`。

状态流转：`Paused` 不推进；`Normal/Fast/VeryFast` 每秒推进 1/2/4 tick。每 tick 是 2 分钟游戏时间。天气剩余时间归零后按确定性规则切换。随机事件按固定间隔触发。

存档字段：`TIME`、`WEATHER`、`EVENT`、`REALTIME`。

错误码：`GamePaused`、存档相关错误。

测试点：暂停、倍速、天气切换、tick 顺序、随机事件、离线进度、读档后继续推进。

典型流程：UI 点击 `+1刻` -> `Game::AdvanceTicks(1)` -> 依序调用天气、种植、养殖、工坊、订单和随机事件。

扩展：复杂随机事件和更完整离线收益仍应放在 `Game` 或新增 `WorldController`，由唯一 tick 分发。
