# Save Format

存档文件：`saves/save01.farm`。

当前版本：`FARM_SAVE 1`。

格式是文本行协议，便于大一学生调试和答辩时解释。示例：

```text
FARM_SAVE 1
TIME 12 1 12
WEATHER 0 8
PLAYER 80 1 0 40
ITEM 0 4 1 1
...
PLOTS 6
PLOT 1 0 3 0 3 0 0
RANCH 2 1
FACILITY 1 0 1 3 1
ANIMAL 1 0 0 1 20
WORKSHOP 0 1
JOB 0 2
ORDERS 5 4 4
ORDER 1 0 3 2 17 6 0 0
END
```

## 安全流程

保存时写入 `save01.farm.tmp`，关闭并检查流状态后替换正式文件。读取时先构造临时 `Game`，只有完整读到 `END` 且版本匹配时才替换当前游戏。

## 错误处理

- 文件不存在：`SaveOpenFailed`
- 魔数错误、字段缺失：`SaveCorrupted`
- 版本不兼容：`SaveVersionMismatch`
- 写入或替换失败：`SaveWriteFailed`

