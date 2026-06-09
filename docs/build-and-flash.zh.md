# 构建与烧录

## 已构建固件

提交版 HEX：

```text
firmware/lumi_exp08_reserved.hex
```

该版本启用：

```c
#define TEMP_RESERVED_MODE      1
#define TEMP_RAW_DIAG           0
```

即温度区域使用模拟显示，不访问异常的 DS18B20 总线。

## 使用 Keil C51 重新构建

要求：

- Windows
- Keil C51
- 默认安装路径：`C:\Keil_v5\C51\BIN`

执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-keil.ps1
```

输出：

```text
build/lumi_exp08_reserved.hex
```

## 烧录

使用课程板配套下载工具或 STC-ISP 类工具烧录 `firmware/lumi_exp08_reserved.hex` 或重新构建后的 `build/lumi_exp08_reserved.hex`。

如果需要重新校准 DS1302 初始时间，可在 `src/main.c` 中临时修改：

```c
#define RTC_FORCE_SET_ON_BOOT   1
```

并修改 `INIT_YEAR`、`INIT_MONTH`、`INIT_DAY`、`INIT_HOUR`、`INIT_MINUTE`、`INIT_SECOND`，烧录一次后再改回 `0`。
