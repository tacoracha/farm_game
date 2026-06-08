# Farm Game

Farm Game 是一个 C++17 农场经营课程项目。当前版本完成核心循环：购买种子、种植、浇水/施肥、推进时间、收割、生产鸡饲料、喂鸡、收鸡蛋、出售或交付订单。

截图位置：`docs/screenshots/`。当前 UI 使用 Win32/GDI 绘制农场风格色块和按钮，assets 中提供可替换的占位贴图。

## 已实现

- 主菜单、新游戏、继续游戏。
- 唯一全局时间，现实 1 秒 = 游戏 2 分钟，支持暂停、1x/2x/4x、基础天气。
- 金币、等级、经验、仓库容量、物品锁。
- 种植、土地购买、DAG 解锁、鸡圈、牛棚、羊圈、饲料坊、订单、商店、仓库。
- 手动存档、自动存档、读档、损坏/版本错误检测。
- 纯核心逻辑测试，不依赖图形窗口。
- MinGW-w64 CMake 构建和安装发布目录。

## 未实现

牛、猪、羊、高级工厂、订单链、紧急订单、复杂事件、离线收益和完整美术素材暂未完成，模块接口与文档中保留了扩展方向。

## 技术栈

- C++17
- CMake
- MinGW-w64 / MSYS2 MinGW64
- Windows Win32/GDI UI

## 构建

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix dist/FarmGame
``` 

运行：`dist/FarmGame/farm_game.exe`。存档默认生成在运行目录的 `saves/save01.farm`。

## 目录

- `include/farm/common`：公共类型、错误码、常量。
- `include/farm/core`：时间、天气、游戏聚合。
- `include/farm/inventory`：玩家资产和仓库。
- `include/farm/planting`：地块与作物。
- `include/farm/ranch`：鸡圈与动物扩展接口。
- `include/farm/workshop`：饲料坊。
- `include/farm/order`：订单板。
- `include/farm/shop`：商店。
- `include/farm/persistence`：存档。
- `include/farm/ui`：Win32 UI 入口。
- `docs/modules`：模块说明。

## 团队分工

cdy 负责 `core` 和 `persistence`；zxc 负责 `planting`；zgm 负责 `ranch`；zjx 负责 `workshop`；lly 负责 `order`；gml 负责 `inventory` 和 `shop`。

素材来源记录见 `assets/ATTRIBUTION.md`。更多架构、构建、存档和迁移说明见 `docs/`。
