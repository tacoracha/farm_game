# UI

负责人：集成模块。

职责：主菜单、页面切换、顶部状态栏、操作按钮、结果提示、贴图加载和 Win32 窗口生命周期。

主要类：`FarmWindow`、`TextureManager`，入口 `farm::ui::RunFarmApp`。

主要文件：`include/farm/ui/Win32FarmApp.h`、`include/farm/ui/TextureManager.h`、`src/ui/Win32FarmApp.cpp`、`src/ui/TextureManager.cpp`、`src/main.cpp`。

拥有数据：当前页面、选中地块、选中订单、临时提示文本、按钮命中区域和 UI 贴图缓存。

对外接口：`RunFarmApp`。

依赖：`Game` 的公开业务接口；贴图文件只在 UI 层读取。

被依赖：主程序。

状态流转：主菜单进入农场页面；顶部标签切换农田、鸡圈、饲料坊、订单、仓库、商店和存档。

存档字段：UI 临时状态不存档。

错误码：所有业务错误通过中文 `ErrorMessage(ErrorCode)` 显示在底部提示区域。

测试点：核心 UI 不做自动窗口测试；业务行为由核心测试覆盖，贴图缺失由 PPM/色块回退保证不崩溃。

典型流程：点击“种小麦” -> UI 调用 `PlantingSystem::TryPlantAt` -> 底部显示中文结果。

后续扩展：替换贴图时优先放入 `assets/textures/*.png` 和 `assets/icons/*.png`。如果后续改用 ImGui、SDL 或 SFML，仍必须保持业务层不包含 UI 头文件。

