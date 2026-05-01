# Android Tool - Phase 2 Design (Practical User Tools)

**Date:** 2026-04-30
**Author:** Eliott
**Status:** Approved

---

## Overview

Phase 2 upgrades Phase 1 stubs to working features across Tabs 2, 5, 6, and 7. Focus is practical user tools: profiles UI, batch operations, root inject, auto-download, MTP browser, logcat viewer, Wi-Fi ADB UI, Docker mode.

Natural language commands and modding tools (USB sniffer, hex editor, DTBO/SELinux) are deferred to Phase 3.

---

## Feature Breakdown

### Tab 2: Flash — Enhancements

**Profiles UI:**
- Left sidebar listing saved profiles as clickable rows
- "Save Current Config" button captures device + last-used files into a named profile
- "Export Profile" / "Import Profile" uses file dialog, reuses `config.rs` FlashProfile types
- Profiles stored as `.yaml` in `~/.androidtool/profiles/`

**Batch Operations:**
- Sub-panel below profiles showing connected devices table (serial, mode, manufacturer, model)
- Checkboxes for multi-select
- "Apply to Selected" button runs the current flash operation on all checked devices
- Progress shown per-device in table with green check / red X

**Root Inject:**
- "Patch boot.img with Magisk" checkbox next to "Flash Boot Image"
- When checked, before flashing: calls external `magiskboot` tool or warns if not found
- Patches the selected boot.img in a temp dir, flashes the patched version

**Auto-Download Recovery:**
- "Download TWRP/OrangeFox" button opens a small dialog
- User selects device, tool fetches latest download URL from known mirrors
- Progress bar during download, saves to local file, auto-selects in file picker

### Tab 5: Files — MTP Browser + Push/Pull

**MTP Browser:**
- When MTP protocol active: split pane (device tree left, local file list right)
- Device tree shows folders/files from MTP device
- Upload: pick local file(s), send to device
- Download: select device file(s), pick local destination

**ADB Push/Pull UI:**
- Push: file picker for local file + text field for device path + "Push" button
- Pull: text field for device path + folder picker for local dest + "Pull" button
- Both show transfer status in log

### Tab 6: Advanced — logcat + dmesg

**Live logcat:**
- "Start logcat" / "Stop logcat" toggle button
- Streaming output in ScrollArea with monospace font
- Filter text field: only shows lines containing filter text
- Autoscroll checkbox

**dmesg Viewer:**
- "Capture dmesg" button runs `adb shell dmesg`
- Output displayed in ScrollArea

### Tab 7: Settings — Wi-Fi ADB + Docker

**Wi-Fi ADB:**
- IP address text field + Port number (default 5555)
- "Connect" button calls `adb connect`
- Shows connection status: "Connected" / "Disconnected" / "Error"
- "Disconnect" button

**Docker:**
- "Generate Dockerfile" button writes Dockerfile to project dir
- "Generate docker-compose.yml" button writes compose file
- Both use existing `network/docker.rs` generate functions

---

## File Changes

| Action | File | Purpose |
|--------|------|---------|
| Modify | `src/ui/flash.rs` | Profiles panel, batch table, root checkbox, download button |
| Modify | `src/ui/files.rs` | MTP browser + full push/pull UI |
| Modify | `src/ui/advanced.rs` | logcat stream + dmesg viewer |
| Modify | `src/ui/settings.rs` | Wi-Fi ADB UI + Docker buttons |
| Create | `src/flash/downloader.rs` | HTTP download with progress |
| Create | `src/flash/magisk.rs` | Boot image patching |
| Create | `src/advanced/logcat.rs` | logcat stream management |
| Create | `src/advanced/dmesg.rs` | dmesg capture helper |
| Create | `src/files/mtp_browser.rs` | MTP device tree + transfer |

No changes to: `app.rs`, `state.rs`, `executor.rs`, `config.rs`, `protocols/`, `safety/`.

---

## Data Flow

All features use existing infrastructure:
- **ProcessOutput channel** for logcat streaming and dmesg
- **Action enum** for button click dispatch (existing variants reused)
- **tokio runtime** for async downloads and parallel batch execution

No new dependencies beyond `reqwest` (or `ureq`) for HTTP downloads.

---

Current Phase: 2
