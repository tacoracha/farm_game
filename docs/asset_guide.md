# 素材与贴图指南

当前 UI 使用 Win32/GDI 绘制，并通过 `TextureManager` 加载贴图。发布时 CMake 会复制整个 `assets/` 目录。

## 加载规则

UI 优先加载 `assets/textures/` 下的 PNG。当前主要 key 与目录如下：

- `terrain/`：草地、干土、湿土、木板、道路、水面等地形。
- `crops/<crop>/0_seed.png` 到 `4_mature.png`：作物 5 阶段成长图。
- `seeds/`：商店中展示的小麦、玉米、胡萝卜、番茄种子。
- `items/`：仓库、订单、状态栏中展示的农作物、饲料、鸡蛋、牛奶、羊毛、肥料、金币。
- `animals/`：牧场中展示鸡、牛、羊、猪，其中鸡有空闲、已喂食、可收获状态。
- `buildings/`：牧场、饲料坊、商店、仓库等建筑贴图。
- `weather/`：顶部状态栏天气图标。
- `ui/`：按钮、顶部栏、订单卡、商店卡、仓库格子等 UI 贴图。

如果 PNG 不存在，会自动回退到仓库内置 PPM：

- `assets/textures/grass.ppm`
- `assets/textures/soil.ppm`
- `assets/textures/wood.ppm`
- `assets/icons/egg.ppm`

如果 PNG 和 PPM 都缺失，UI 会退回到 GDI 色块，不会崩溃。`TextureManager` 只在 UI 层使用，业务逻辑、测试和存档不依赖贴图。

## 当前贴图包

用户新增的 `pic/Farm Game Rich Texture Pack` 已复制到 `assets/textures/`，作为项目当前默认贴图。该包包含作物阶段、未来工厂、动物、天气和 UI 占位图，适合后续继续覆盖美术资源。

后续换图时优先保持现有目录和文件名。若新增作物、动物或建筑，需要在 `Win32FarmApp.cpp` 中补充一次 `textures_.Load(...)` 和对应的 key 映射 helper。

## 替换方式

1. 从 CC0 或允许再分发的素材站下载贴图。
2. 裁切或导出为上面目录中的同名 PNG。
3. 放入 `assets/textures/`。
4. 重新执行 `cmake --install build --prefix dist/FarmGame`。

业务模块不依赖图片文件，因此替换贴图不会影响测试和存档格式。

## 推荐来源

- OpenGameArt `Simple Farm Tiles`，作者 qubodup，CC0。
- Kenney `Isometric Miniature Farm`，作者 Kenney，CC0。

添加外部素材时必须更新 `assets/ATTRIBUTION.md`，记录文件名、来源、作者、许可证、原始页面和是否修改。
