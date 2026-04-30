# Android Bootloader & Recovery Tool - Complete Redesign

**Date:** 2026-04-30
**Author:** Eliott
**Status:** Approved

---

## Overview

Full redesign of the existing Rust/egui Android flashing tool from a single-panel layout into a tabbed, modular application covering the "Ultimate Feature List" across 3 phases. This spec covers the Phase 1 design in detail, with stubs for Phases 2-3.

Phase 1 delivers universal protocol unification (10+ device protocols), brick protection, backup/restore, batch operations, profiles, and cross-platform support.

---

## Tab Layout

| # | Tab | Phase 1 Content |
|---|-----|----------------|
| 1 | Dashboard | Device detection, mode indicator, quick actions (unlock/lock/reboot), device info, AMD quirk |
| 2 | Flash | Flash ROM/recovery/boot, sideload, root inject (Magisk), batch operations, flash profiles |
| 3 | Backup & Restore | Full NAND backup/restore, partition dumps, checksums, hash logging |
| 4 | Protocols | Protocol-specific operations, firehose/DA loader management, security bypasses |
| 5 | Files | MTP browser, ADB push/pull, file manager |
| 6 | Advanced | logcat viewer, dmesg viewer (+ Phase 2 stubs) |
| 7 | Settings | Preferences, driver management, Wi-Fi ADB, Docker config, about/license |

All tabs visible at all times. Non-Phase-1 features (AI, community, PQC security, etc.) show as labeled "Coming in Phase N" stubs.

---

## File/Module Structure

```
src/
├── main.rs                  # eframe entry point, app struct, main loop
├── app.rs                   # AndroidUnlockerApp core (state, action dispatch)
├── state.rs                 # AppState struct, defaults
├── config.rs                # FullConfig, DeviceConfig (unchanged)
├── ui/
│   ├── mod.rs               # Tab registry, shared UI helpers
│   ├── dashboard.rs         # Tab 1
│   ├── flash.rs             # Tab 2
│   ├── backup.rs            # Tab 3
│   ├── protocols.rs         # Tab 4
│   ├── files.rs             # Tab 5
│   ├── advanced.rs          # Tab 6
│   └── settings.rs          # Tab 7
├── protocols/
│   ├── mod.rs               # Protocol trait, ProtocolEngine, detection pipeline
│   ├── adb_fastboot.rs      # ADB, Fastboot, Fastbootd
│   ├── edl.rs               # Qualcomm EDL (9008/900E), Sahara, Firehose
│   ├── brom.rs              # MediaTek BROM, DA agents
│   ├── samsung.rs           # Samsung Download Mode
│   ├── mtk_flash.rs         # SP Flash Tool protocol
│   ├── xiaomi.rs            # MiFlash protocol
│   ├── huawei.rs            # Huawei eRecovery
│   └── mtp.rs               # MTP file transfer
├── safety/
│   ├── mod.rs
│   ├── backup.rs            # NAND backup/restore
│   ├── validation.rs        # Pre-flash compatibility checks
│   ├── watchdog.rs          # Reconnection watchdog
│   └── checksum.rs          # Checksumming and hash logging
├── executor.rs              # Async command executor (existing, minor additions)
├── platform/
│   └── drivers.rs           # Driver management (libusb, WinUSB)
└── network/
    ├── wifi_adb.rs          # TCP/IP ADB connectivity
    └── docker.rs            # Docker/container config gen
```

Each file targets 100-400 lines. No file exceeds ~500 lines.

---

## Protocol Engine

### Trait

```rust
pub trait ProtocolHandler: Send + Sync {
    fn name() -> &'static str;
    fn priority() -> u8;                          // Detection order (lower = checked first)
    fn vid_pids() -> &'static [(u16, u16)];       // Known USB VID/PID pairs
    async fn detect() -> Option<DeviceInfo>;       // Returns device info if detected
    async fn run_command(ctx: &ExecutionContext, cmd: ProtocolCommand) -> Result<ProcessOutput>;
}
```

### Detection Pipeline

1. Each protocol registers `vid_pids()` and `detect()` with the engine
2. On USB connection or timer tick (3s default), engine calls `detect()` in priority order
3. First successful detection sets the active protocol in `AppState`
4. UI renders available operations based on active protocol

### Implemented Protocols (Phase 1)

| Protocol | Module | VID/PID Examples | Key Operations |
|----------|--------|------------------|----------------|
| ADB | adb_fastboot | 18d1:4ee0, 04e8:685e | shell, push, pull, reboot, sideload |
| Fastboot | adb_fastboot | 18d1:4ee0 (fastboot mode) | flash, erase, boot, oem unlock, getvar |
| Fastbootd | adb_fastboot | Same as ADB | userspace fastboot operations |
| EDL 9008 | edl | 05c6:9008 | sahara handshake, firehose load, partition r/w |
| EDL 900E | edl | 05c6:900e | Same as 9008 |
| BROM | brom | 0e8d:0003, 0e8d:2000 | DA agent upload, partition flash |
| Samsung DL | samsung | 04e8:685d | odin protocol, tar flash |
| MTK SP Flash | mtk_flash | 0e8d:2001 | scatter file, partition flash |
| Xiaomi MiFlash | xiaomi | 2717:ff08 | miflash protocol |
| Huawei eRecovery | huawei | 12d1:107e | erecovery operations |
| MTP | mtp | Various | file browse, transfer |

### Dynamic Fallback

If an operation fails (e.g., Fastboot flash times out), the engine attempts the next available protocol for that device. Fallback chain configured per action type.

---

## Safety Pipeline

### Pre-Flash Validation

Every write operation passes through:
1. **Compatibility check** — Firmware matches device model/variant, anti-rollback version acceptable (loaded from local JSON/YAML compatibility DB)
2. **Auto-backup** (if enabled) — Reads current partition table, backs up: boot, recovery, dtbo, vbmeta, vendor_boot, misc
3. **Checksum log** — SHA-256 of all to-be-modified partitions stored in `flash_logs/<serial>/<timestamp>/`
4. **Post-verification** — Re-checksums after flash, logs success/failure

### Brick Protection

- **Watchdog Timer**: If device disconnects mid-flash, polls for reconnection every 5s for up to 5 minutes. Resumes automatically on reconnect. Logs failure after timeout.
- **Rollback Protection Bypass**: Opt-in checkbox settings (Xiaomi/Oppo). Workarounds: modify misc partition, flash vbmeta with `--disable-verity`, patched abl/xbl.
- **Checksum Verification**: All partitions verified before write. Mismatch aborts with warning.

### Forensic Mode

- **Read-Only extraction**: Dump any partition without modification
- **Hash Logging**: Full SHA-256 audit trail (pre-flash and post-flash) saved as timestamped JSON

### Data Storage

```
~/.androidtool/
├── backups/<serial>/<timestamp>/
│   ├── boot.img
│   ├── recovery.img
│   ├── vbmeta.img
│   └── manifest.yaml         # device info, partition map, pre-flash checksums
├── flash_logs/<serial>/<timestamp>.json  # actions, hashes, metadata
├── config.yaml                # user preferences
└── driver_cache/              # cached libusb/WinUSB drivers
```

---

## Tab Details

### Tab 1: Dashboard
- Mode indicator (colored, uppercase, large)
- Device info: manufacturer, model, serial, firmware version, slot (A/B if applicable)
- Quick action buttons: unlock, lock, reboot (system/recovery/bootloader)
- AMD/Xiaomi quirk panel
- Phase 2 stub: natural language command input

### Tab 2: Flash
- ROM flash: pick file or URL download, optional Magisk/Kitsune Mask root inject, optional DM-Verity disable
- Recovery flash: pick img, pick from known (TWRP/OrangeFox/PBRP)
- Boot flash: pick patched boot img
- Sideload: pick zip
- Batch sub-panel: table of connected devices, checkboxes, apply operations to all
- Profiles sub-panel: save/load/export/import YAML flash configs
- Phase 2 stub: auto-download latest firmware/TWRP/Magisk

### Tab 3: Backup & Restore
- "Create Backup" button → backs up critical partitions
- "Full NAND Backup" button → full device image via dd or TWRP backup mode
- Restore panel: pick backup directory, select partitions to restore
- Checksum viewer: table of partition → SHA-256, timestamps
- Forensic dump: partition selector + "Dump (Read-Only)" + "Generate Hash Log"
- Phase 2 stub: encrypted cloud backup (PQC)

### Tab 4: Protocols
- Dynamic content based on active protocol
- Per-protocol panels:
  - EDL: firehose loader upload, auth bypass tools, partition r/w
  - BROM: DA agent upload, scatter file flash
  - Samsung: odin tar flash, pit operations
  - All protocols: custom command input field
- Security bypasses panel: FRP bypass, EDL auth bypass, Samsung KG bypass (each with explicit consent checkbox)
- Phase 3 stub: exploit DB auto-update

### Tab 5: Files
- MTP browser (when MTP protocol active): directory tree, file list, upload/download buttons
- ADB Push: pick local file, enter device destination, "Push" button
- ADB Pull: enter device source, pick local destination, "Pull" button
- Transfer history with status

### Tab 6: Advanced
- Live logcat viewer: streaming output, filter text field, pause/resume
- dmesg viewer: snapshot capture from `adb shell dmesg`
- Phase 2 stubs: USB sniffer (Wireshark-like capture), partition hex editor, boot image patcher (Magisk/Kitsune auto-patch), DTBO editor, SELinux policy explorer

### Tab 7: Settings
- Preferences: auto-detect interval (1-30s), backup location picker, default backup behavior
- Driver management: detect OS, list required drivers (libusb/WinUSB/Zadig), install button (with admin/sudo prompt)
- Network: Wi-Fi ADB connect (IP:port input, connect/disconnect buttons)
- Docker: generate Dockerfile/docker-compose.yml button, display headless mode instructions
- About: version, author (Eliott), GPLv3 license, dependency credits

---

## Phase 2 Stubs (Smart & Dev)

Features showing "Coming in Phase 2":
- Tab 1: Natural language command input
- Tab 2: Auto-download latest firmware/TWRP/Magisk from official sources
- Tab 4: Community device profile contributions (GitHub integration)
- Tab 6: USB traffic sniffer, partition hex editor, boot image patcher (Magisk/Kitsune auto-patch), DTBO overlays editor, SELinux policy explorer
- Tab 7: Docker full container mode for CI/CD

---

## Phase 3 Stubs (Ecosystem)

Features showing "Coming in Phase 3":
- Tab 1: AI-powered device database, community profiles
- Tab 2: Drag-and-drop workflow editor, Python/JS scripting API
- Tab 3: Post-quantum encrypted cloud backup (Kyber/NTRU), secure wipe (DoD/Gutmann)
- Tab 4: Crowdsourced exploit DB, IRC/Discord bot, remote assistance
- Tab 7: Zero-knowledge cloud sync (self-hosted EU), HashiCorp Vault integration, GDPR compliance panel
- Legal: Geo-locked exploit features, tamper-evident logs, community bug bounty

---

## Key Design Decisions

1. **Trait-based protocol abstraction** — Scales to 10+ protocols, keeps each protocol in its own file
2. **Safety pipeline (validation → backup → checksum → flash → verify)** — Prevents bricks
3. **Watchdog with reconnection polling** — Survives mid-flash disconnects
4. **All tabs visible from start** — Full skeleton with stubs communicates the roadmap
5. **Files capped at ~500 lines** — Enforces modularity, keeps diffs manageable
6. **~/.androidtool/ directory for all data** — Clean separation from project, platform-native home dir

---

## Implementation Plan

Phase 1 implemented and delivered before Phase 2 begins. Phase 3 delivered after Phase 2. Each phase is its own spec → plan → implementation cycle.

Current Phase: 1
