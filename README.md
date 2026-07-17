# LUMI.BUDDY 8051

[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-8051%20%2F%20STC89C52-lightgrey.svg)](docs/hardware.zh.md)
[![Display](https://img.shields.io/badge/display-LCD12864%20ST7920-green.svg)](docs/hardware.zh.md)

An LCD12864 temperature-clock interface for an 8051/STC89C52 course board. The project extends the standard character-display exercises into a small graphical status display while staying within the memory limits of a classic 8051.

![LUMI.BUDDY interface preview](assets/lumi_exp08_preview.png)

## What Is Implemented

- **16-byte scanline renderer:** draws one LCD row at a time instead of allocating a 1024-byte framebuffer in the 8051's limited internal RAM.
- **Seven pixel-character states:** idle, sleep, hot, cold, heart, wave, and error are generated at runtime.
- **Partial refresh:** dirty flags limit writes to regions whose content changed and reduce visible flicker.
- **DS1302 clock path:** BCD values are checked before display; invalid values fall back to a known initialization.
- **DS18B20 interface reservation:** the current verified build uses simulated temperature with `TEMP_RESERVED_MODE=1`. The real sensor path remains in the source but is not claimed as hardware-validated.

## Hardware

| Module | Connection |
|---|---|
| LCD12864 data | `DB0-DB7 = P0.0-P0.7` |
| LCD12864 control | `RS=P2.6`, `RW=P2.5`, `E=P2.7`, `PSB=P3.2` |
| DS1302 RTC | `IO=P3.4`, `RST=P3.5`, `CLK=P3.6` |
| DS18B20 temperature | Reserved at `DQ=P3.7` |

See [docs/hardware.zh.md](docs/hardware.zh.md) for wiring, pin conflicts, and the status of reserved interfaces.

## Build And Flash

The repository includes a verified course-board build:

- source: [`src/main.c`](src/main.c)
- firmware: [`firmware/lumi_exp08_reserved.hex`](firmware/lumi_exp08_reserved.hex)

Rebuild with Keil C51 on Windows:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-keil.ps1
```

The script expects Keil C51 at `C:\Keil_v5\C51\BIN` unless configured otherwise. Flash the HEX file with STC-ISP or the tool supplied for the board. Full steps are in [docs/build-and-flash.zh.md](docs/build-and-flash.zh.md).

## Repository Map

| Path | Contents |
|---|---|
| `src/main.c` | Experiment 8 firmware and LCD renderer. |
| `firmware/` | Verified reserved-temperature HEX build. |
| `assets/` | Photos from experiments 1-8 and the interface preview. |
| `docs/final-report.zh.md` | Course report and experiment 8 design notes. |
| `docs/architecture.zh.md` | Rendering model, state organization, and hardware boundaries. |
| `docs/hardware.zh.md` | Wiring and interface validation status. |
| `docs/build-and-flash.zh.md` | Keil build, flashing, and RTC setup. |
| `ROADMAP.md` | Hardware validation and optional follow-up work. |

## Design Notes

The project was inspired by the general idea of a small desktop display that turns software or sensor state into a glanceable physical interface. The implementation, pixel character, renderer, and assets in this repository were created for this course project; no upstream Claude Desktop Buddy code or artwork is included.

## License

Apache License 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).

---

# 中文说明

`LUMI.BUDDY 8051` 是一个运行在 8051/STC89C52 课程板和 LCD12864 上的温度时钟界面。它从课程中的字符显示实验继续扩展，在经典 8051 的内存限制下实现图形状态界面。

当前已验证内容包括 16 字节扫描线渲染、七种像素角色状态、局部刷新、DS1302 时钟和 BCD 校验。温度功能当前使用 `TEMP_RESERVED_MODE=1` 的模拟值；DS18B20 实测链路仍需在硬件修复后重新验证，因此仓库不把模拟温度描述为传感器实测结果。

构建、接线和完整课程报告分别见 [构建说明](docs/build-and-flash.zh.md)、[硬件说明](docs/hardware.zh.md)和[课程报告](docs/final-report.zh.md)。
