# LUMI.BUDDY 8051

[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-8051%20%2F%20STC89C52-lightgrey.svg)](docs/hardware.zh.md)
[![Display](https://img.shields.io/badge/display-LCD12864%20ST7920-green.svg)](docs/hardware.zh.md)

**[中文课程报告 →](docs/final-report.zh.md)**

This is a pixel-art temperature clock HUD running on an 8051/STC89C52 course board with an LCD12864 (ST7920 graphic mode). I started from the standard character-mode clock demo used in class and extended it into a small graphical desktop buddy as a more expressive course-board interface.

![LUMI.BUDDY interface preview](assets/lumi_exp08_preview.png)

## Highlights

- **16-byte scanline buffer** — instead of allocating a 1024-byte framebuffer, I render one scanline at a time. Fits comfortably inside the 8051's 256-byte internal RAM.
- **Original pixel avatar** — 7 expression states (idle, sleep, hot, cold, heart, wave, error), all drawn at runtime with zero bitmap storage.
- **Dirty-flag partial refresh** — only the scan-line ranges that actually changed get rewritten to the LCD, which cuts down on visible flicker quite a bit.
- **Reserved DS18B20 driver path** — a single `#define` keeps the simulated demo separate from the real sensor code path, so switching back is trivial once the hardware works.
- **BCD validation** — DS1302 readings get sanity-checked before anything hits the screen; if the data looks corrupt, it falls back to a preset time automatically.

## Hardware

| Module | Pins |
| --- | --- |
| LCD12864 data | `DB0-DB7 = P0.0-P0.7` |
| LCD12864 control | `RS=P2.6` `RW=P2.5` `E=P2.7` `PSB=P3.2` |
| DS1302 RTC | `IO=P3.4` `RST=P3.5` `CLK=P3.6` |
| DS18B20 temp | Reserved — handout uses `DQ=P3.7` |

See [docs/hardware.zh.md](docs/hardware.zh.md) for port-conflict notes and the reserved temperature interface.

## Build & Flash

Pre-built firmware: [`firmware/lumi_exp08_reserved.hex`](firmware/lumi_exp08_reserved.hex)

To rebuild with Keil C51 (Windows, default path `C:\Keil_v5\C51\BIN`):

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-keil.ps1
```

Flash with STC-ISP or the download tool provided with your board. See [docs/build-and-flash.zh.md](docs/build-and-flash.zh.md) for the full walkthrough.

## Documents

| Document | Description |
| --- | --- |
| [Course Report](docs/final-report.zh.md) | Full report: experiments 1-7 summaries, experiment 8 design specification, flowchart, result photos |
| [Architecture Notes](docs/architecture.zh.md) | Rendering model, hardware boundary, and future RTOS/ROS expansion path |
| [Hardware Notes](docs/hardware.zh.md) | Wiring table, temperature reservation, button reservation |
| [Build & Flash](docs/build-and-flash.zh.md) | Keil rebuild steps and DS1302 time calibration |
| [Roadmap](ROADMAP.md) | Open-source evolution plan from course demo to modern MCU status buddy |

## Project Vision

`LUMI.BUDDY` is intentionally more than just a clock demo. I built it as a tiny 8051 prototype of a desktop status buddy:

| Now | Future |
| --- | --- |
| Time, simulated temperature, avatar state, mode text | Task cards, AI handoff status, data-lake index, ROS 2 metrics |
| 8051 + LCD12864 monochrome | ESP32-S3 / SiFli + color screen, FreeRTOS + LVGL, BLE/Wi-Fi |

Some of the inspiration comes from Anthropic's open-source [**Claude Desktop Buddy**](https://github.com/anthropics/claude-desktop-buddy) — specifically the idea of turning software state into a small physical display you can glance at on your desk. This project reworks that same interaction pattern for an 8051 course board with an original monochrome pixel character and a scanline renderer. No upstream code or assets from that project are included here.

## Repository Boundary

This repository does **not** include course handout PDFs, private notes, or personal evidence files. Those materials have separate ownership or privacy constraints, so they stay out of this repo.

## License

Apache License 2.0 — see [LICENSE](LICENSE) and [NOTICE](NOTICE).
