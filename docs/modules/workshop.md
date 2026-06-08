# Workshop

负责人：zjx。

职责：饲料坊配方、消耗作物、生产队列、完成货架、领取鸡饲料。

主要类：`WorkshopSystem`。

主要文件：`include/farm/workshop/WorkshopSystem.h`、`src/workshop/WorkshopSystem.cpp`。

拥有数据：生产队列 `ProductionJob`、鸡饲料货架数量。

对外接口：`StartProduction`、`Tick`、`ClaimProduct`、`ClaimAllProducts`、`View`。

依赖：`PlayerState` 扣小麦和入库鸡饲料。

被依赖：`Game`、UI、存档、测试。

状态流转：`StartProduction -> Tick -> shelf ready -> ClaimProduct`。货架满时完成品保留在队列头，不丢失。

存档字段：`WORKSHOP`、`JOB`。

错误码：`RecipeUnavailable`、`ProductionQueueFull`、`InsufficientItem`、`ProtectedItem`、`ProductNotReady`、`WarehouseFull`。

测试点：材料不足、开始生产扣料、生产完成、领取产品、货架满不丢失。

典型流程：收割小麦 -> `StartProduction(player, ChickenFeed, 1)` -> tick -> `ClaimProduct`。

扩展：乳品坊、烘焙屋、纺织间可增加 `RecipeId` 和 `FactoryKind`，核心队列逻辑不需要改成复杂继承。

