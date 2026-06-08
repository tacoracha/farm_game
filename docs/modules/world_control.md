# World Control

负责人：cdy。

职责：维护唯一全局时间、暂停/速度、天气切换、跨模块 tick 顺序、玩家经验等级入口和存档调度。

主要类：`Game`、`TimeSystem`、`WeatherSystem`。

主要文件：`include/farm/core/Game.h`、`src/core/Game.cpp`。

拥有数据：当前 tick、速度、天气、天气剩余 tick、自动存档 tick。

对外接口：`AdvanceTicks`、`AdvanceBySpeed`、`ManualSave`、`Load`、`AutoSaveIfNeeded`、`Time().Snapshot()`、`Weather().Snapshot()`。

依赖：种植、养殖、工坊、订单、玩家资产、存档。

被依赖：UI、测试、`SaveManager`。

状态流转：`Paused` 不推进；`Normal/Fast/VeryFast` 每次 UI 定时器推进 1/2/4 tick。天气剩余时间归零后按确定性规则切换。

存档字段：`TIME`、`WEATHER`、自动存档 tick。

错误码：`GamePaused`、存档相关错误。

测试点：暂停、倍速、天气切换、tick 顺序、读档后继续推进。

典型流程：UI 点击 `+Tick` -> `Game::AdvanceTicks(1)` -> 依序调用天气、种植、养殖、工坊、订单。

扩展：随机事件和离线收益应放在 `Game` 或新增 `WorldController`，仍由唯一 tick 分发。

