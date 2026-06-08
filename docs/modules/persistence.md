# Persistence

负责人：cdy。

职责：手动存档、自动存档、读档、版本检查、坏档保护。

主要类：`SaveManager`。

主要文件：`include/farm/persistence/SaveManager.h`、`src/persistence/SaveManager.cpp`。

拥有数据：不长期持有数据，只读写 `Game` 快照。

对外接口：`Save`、`Load`、`Exists`。

依赖：`Game` 和各模块公开只读数据/加载接口。

被依赖：`Game`、UI、测试。

状态流转：保存写临时文件再替换；读取构造临时 `Game`，成功后替换当前游戏。

存档字段：详见 `docs/save_format.md`。

错误码：`SaveOpenFailed`、`SaveWriteFailed`、`SaveVersionMismatch`、`SaveCorrupted`。

测试点：保存、读取、往返一致、缺失文件、损坏文件、版本错误、读取失败不破坏当前内存。

典型流程：UI 点击保存 -> `Game::ManualSave("saves/save01.farm")` -> `SaveManager::Save`。

扩展：若后续改 JSON，只应改 `SaveManager`，不要让业务类直接依赖 JSON。

