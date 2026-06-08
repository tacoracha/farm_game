# Ranch

负责人：zgm。

职责：鸡圈、牛棚、羊圈、购买动物、喂养、生产倒计时、收获动物产品、容量、批量喂养和批量收获。

主要类：`RanchSystem`。

主要文件：`include/farm/ranch/RanchSystem.h`、`src/ranch/RanchSystem.cpp`。

拥有数据：`RanchFacilityData`、`AnimalData`、下一动物 id。

对外接口：`BuyAnimal`、`FeedAnimal`、`HarvestAnimal`、`BatchFeed`、`BatchHarvest`、`FacilityViews`、`AnimalViews`。

依赖：`PlayerState` 扣鸡饲料、加鸡蛋、扣买鸡金币。

被依赖：`Game`、UI、存档、测试。

状态流转：`Idle -> Producing -> Ready -> Idle`。鸡产鸡蛋，牛产牛奶，羊产羊毛。

存档字段：`RANCH`、`FACILITY`、`ANIMAL`。

错误码：`FacilityOutOfRange`、`FacilityFull`、`AnimalOutOfRange`、`AnimalNotIdle`、`AnimalNotReady`、`InsufficientItem`、`WarehouseFull`。

测试点：购买鸡、无饲料失败、喂食扣料、时间推进、鸡蛋成熟、仓库满不丢蛋。

典型流程：`BuyAnimal` -> `FeedAnimal` -> `Game::AdvanceTicks` -> `HarvestAnimal`。

扩展：当前已实现牛和羊的基础流程。猪仍保留在枚举中，后续可按牛/羊模式添加专属产品和饲料。
