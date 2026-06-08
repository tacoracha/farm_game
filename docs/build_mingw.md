# Build With MinGW-w64

推荐环境：MSYS2 MinGW64 或已加入 `PATH` 的独立 MinGW-w64。

## MSYS2 安装

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make
```

在 “MSYS2 MinGW x64” 终端中进入项目目录。

## 配置、编译、测试

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

## 安装发布目录

```bash
cmake --install build --prefix dist/FarmGame
```

发布目录包含：

- `farm_game.exe`
- `assets/`
- MinGW 运行库 DLL，如检测到 `libgcc_s_*.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll`
- `README.txt`

## 常见问题

如果提示找不到编译器，确认 MinGW 的 `bin` 目录在 `PATH`。如果双击 exe 提示缺 DLL，重新执行安装命令，或确认 CMake 使用的编译器目录中存在对应 DLL。

