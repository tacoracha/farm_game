# Ranch

负责人：zgm。

职责：鸡圈、购买鸡、喂鸡、生产倒计时、收鸡蛋、容量、批量喂养和批量收获。

主要类：`RanchSystem`。

主要文件：`include/farm/ranch/RanchSystem.h`、`src/ranch/RanchSystem.cpp`。

拥有数据：`RanchFacilityData`、`AnimalData`、下一动物 id。

对外接口：`BuyAnimal`、`FeedAnimal`、`HarvestAnimal`、`BatchFeed`、`BatchHarvest`、`FacilityViews`、`AnimalViews`。

依赖：`PlayerState` 扣鸡饲料、加鸡蛋、扣买鸡金币。

被依赖：`Game`、UI、存档、测试。

状态流转：`Idle -> Producing -> Ready -> Idle`。

存档字段：`RANCH`、`FACILITY`、`ANIMAL`。

错误码：`FacilityOutOfRange`、`FacilityFull`、`AnimalOutOfRange`、`AnimalNotIdle`、`AnimalNotReady`、`InsufficientItem`、`WarehouseFull`。

测试点：购买鸡、无饲料失败、喂食扣料、时间推进、鸡蛋成熟、仓库满不丢蛋。

典型流程：`BuyAnimal` -> `FeedAnimal` -> `Game::AdvanceTicks` -> `HarvestAnimal`。

扩展：牛/猪/羊可复用 `AnimalKind` 和 `RanchFacilityKind`，先增加饲料和产品物品，再扩展设施匹配规则。

