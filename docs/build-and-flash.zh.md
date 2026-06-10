# 构建与烧录

## 已构建固件

仓库里直接附了编译好的 HEX 文件：

```text
firmware/lumi_exp08_reserved.hex
```

这个版本里启用了以下配置：

```c
#define TEMP_RESERVED_MODE      1
#define TEMP_RAW_DIAG           0
```

也就是说温度区域走的是模拟显示，不会去访问那条有问题的 DS18B20 总线。

## 使用 Keil C51 重新构建

前提条件：

- Windows 系统
- 已安装 Keil C51
- 默认安装路径：`C:\Keil_v5\C51\BIN`

在 PowerShell 里执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-keil.ps1
```

编译输出会在这里：

```text
build/lumi_exp08_reserved.hex
```

## 烧录

用课程板配套的下载工具，或者随便找一个 STC-ISP 类工具，把 `firmware/lumi_exp08_reserved.hex`（或重新编译后 `build/lumi_exp08_reserved.hex`）烧进去就行。

如果需要重新校准 DS1302 的初始时间，可以临时改一下 `src/main.c` 里的宏：

```c
#define RTC_FORCE_SET_ON_BOOT   1
```

然后把 `INIT_YEAR`、`INIT_MONTH`、`INIT_DAY`、`INIT_HOUR`、`INIT_MINUTE`、`INIT_SECOND` 改成你想要的时间值，烧录一次之后再改回 `0` 就好。
