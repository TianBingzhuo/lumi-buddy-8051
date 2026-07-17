# Roadmap

This roadmap separates verified behavior from optional follow-up work. It is not a promise that every item will be implemented.

## Current Verified Build

- LCD12864 graphic mode on an 8051/STC89C52-compatible board.
- DS1302 clock display with BCD validation and fallback initialization.
- Simulated temperature under `TEMP_RESERVED_MODE=1`.
- Seven original monochrome pixel-character states.
- 16-byte scanline rendering and dirty-region refresh.
- Keil C51 build with a checked-in HEX artifact.

## Next Hardware Validation

- Diagnose the DS18B20 `P3.7/DQ` line and record idle level, presence pulse, and scratchpad bytes.
- Re-enable real temperature only after repeatable sensor reads are confirmed.
- Verify the physical key mapping and add debounced input.
- Add a short hardware test checklist covering LCD, RTC, temperature bus, and keys.

## Optional Follow-Up

- Refactor display states into smaller modules while preserving the low-memory scanline design.
- Define a compact serial message format so a host computer can update status text.
- Port the interface concept to a modern MCU and color display only if the new version has a concrete use case and testable hardware target.

## Acceptance Rule

A feature moves from "planned" to "implemented" only after its source, build result, hardware state, and limitations are documented. Simulated or reserved interfaces remain labeled as such.
