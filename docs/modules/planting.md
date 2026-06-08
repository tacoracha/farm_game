# Planting

负责人：zxc。

职责：地块、指定地块播种、浇水、施肥、成长、成熟、收割、扩建。

主要类：`PlantingSystem`。

主要文件：`include/farm/planting/PlantingSystem.h`、`src/planting/PlantingSystem.cpp`。

拥有数据：`PlotData` 列表，包括状态、水分、作物、种植 tick、成熟 tick、施肥状态和天气成长余量。

对外接口：`TryPlant`、`TryPlantAt`、`WaterPlot`、`ApplyFertilizer`、`Harvest`、`Expand`、`View`。

依赖：`PlayerState` 扣种子/肥料和入库作物；`WeatherSystem` 提供成长系数。

被依赖：`Game`、UI、存档、测试。

状态流转：`Idle -> Growing -> Mature -> Idle`。浇水提前 1 tick；施肥把剩余时间约减半。

存档字段：`PLOTS`、`PLOT state water crop planted_tick mature_tick fertilized growth_remainder`。

错误码：`PlotOutOfRange`、`PlotNotIdle`、`PlotNotGrowing`、`PlotAlreadyWatered`、`PlotAlreadyFertilized`、`PlotNotMature`、`WarehouseFull`。

测试点：播种、重复播种失败不扣种子、浇水、重复浇水、施肥、成熟、仓库满不丢作物。

典型流程：购买种子 -> `TryPlantAt` -> `WaterPlot` -> `Game::AdvanceTicks` -> `Harvest`。

扩展：新增作物只需在 `Types.cpp` 中添加物品配置和商店展示。

