# Roadmap

`LUMI.BUDDY 8051` starts as a course-ready LCD12864 experiment, but the design is intentionally shaped like a small open-source hardware/software project. This roadmap keeps the current 8051 version honest while leaving a clear path toward a modern desktop status buddy.

## v0.1 Course Demo - Current

- LCD12864 graph mode on an 8051/STC89C52-compatible board.
- DS1302 clock display with BCD validation and fallback initialization.
- Simulated temperature display through `TEMP_RESERVED_MODE`.
- Original monochrome pixel avatar with multiple expression states.
- 16-byte scanline rendering without a full 1024-byte framebuffer.
- Apache-2.0 public package with source, firmware, report, and hardware notes.

## v0.2 Hardware Recovery

- Recover real DS18B20 temperature reading after the `P3.7/DQ` low-level issue is fixed.
- Add a small diagnostic screen for DQ idle level, presence pulse, and raw scratchpad bytes.
- Confirm real board key mapping and add debounced input.
- Use keys for expression switching, manual refresh, and mode changes.

## v0.3 Modern MCU Port

- Port the UI concept to ESP32-S3, STM32, SiFli, or STC32G/AI8051-class boards.
- Replace monochrome LCD12864 with color LCD/OLED and LVGL.
- Split work into RTOS tasks: clock, sensor, UI render, communication, and watchdog.
- Add BLE/Wi-Fi for NTP time sync and desktop companion messages.

## v0.4 Personal Status Panel

- Add task-card and handoff-card display modes.
- Show local knowledge-base status, study checklist progress, or build/test results.
- Keep the avatar as a compact state indicator instead of a decorative-only mascot.
- Define a simple serial/BLE message protocol for upstream desktop software.

## v0.5 Robotics / Systems HUD

- Display ROS 2 latency, QoS, missed frames, or actuator state summaries.
- Add warning states for jitter, stale telemetry, and sensor failure.
- Use the device as a tiny always-on "system heartbeat" panel for lab experiments.

## Non-Goals For v0.1

- No private course handout PDFs are included.
- No copyrighted character image is embedded.
- The temperature value is not claimed as a real measurement while `TEMP_RESERVED_MODE` is enabled.
- The board key feature is documented as reserved until the physical key mapping is verified.
