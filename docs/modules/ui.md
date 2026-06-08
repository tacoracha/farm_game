# UI

负责人：集成模块。

职责：主菜单、页面切换、顶部状态栏、操作按钮、结果提示和 Win32 窗口生命周期。

主要类：`FarmWindow`，入口 `farm::ui::RunFarmApp`。

主要文件：`include/farm/ui/Win32FarmApp.h`、`src/ui/Win32FarmApp.cpp`、`src/main.cpp`。

拥有数据：当前页面、选中地块、选中订单、临时提示文本和按钮命中区域。

对外接口：`RunFarmApp`。

依赖：`Game` 的公开业务接口。

被依赖：主程序。

状态流转：主菜单 -> 农场页面；顶部 tabs 切换农场、鸡圈、饲料坊、订单、仓库、商店、存档。

存档字段：UI 临时状态不存档。

错误码：所有业务错误通过 `ToString(ErrorCode)` 显示在底部提示区域。

测试点：核心 UI 不做自动窗口测试；业务行为由核心测试覆盖。人工验收按钮流程。

典型流程：点击“Plant Wheat” -> UI 调用 `PlantingSystem::TryPlantAt` -> 底部显示结果。

扩展：可替换为 ImGui/SDL/SFML，但仍必须保持业务层不包含 UI 头文件。

