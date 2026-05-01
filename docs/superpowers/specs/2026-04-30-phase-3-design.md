# Phase 3 - Ecosystem & Completeness Design

**Date:** 2026-04-30
**Author:** Eliott
**Status:** Approved

---

## Overview

Phase 3 fills the remaining stubs from Tabs 3, 4, 6, and 7 plus global legal safeguards. Focus: local security, device profiling, firmware analysis, and ethical compliance — all desktop-only, no backend services.

No new crate dependencies beyond `aes-gcm` for local backup encryption. All features use existing channel-based async infrastructure.

---

## Feature Breakdown

### Tab 3: Backup & Restore — Encryption + Secure Wipe

**Local encrypted backup**
- "Encrypt backup with AES-256" checkbox in Backup group
- User provides passphrase via text field (2 fields: enter + confirm)
- Partition images encrypted via AES-256-GCM before saving
- Restore prompts for passphrase, decrypts then flashes
- Encrypted files stored in `~/.androidtool/backups/<serial>/<ts>/` with `.enc` extension
- Manifest (device info, checksums) stored encrypted alongside

**Secure wipe**
- "Quick Wipe" button: `fastboot erase userdata` + `fastboot erase cache`
- "Secure Wipe (DoD 5220.22-M)" button: 3-pass random overwrite via `adb shell dd if=/dev/urandom of=/dev/block/by-name/userdata bs=1M`
- Shows BIG consent dialog: "THIS WILL PERMANENTLY DESTROY ALL DATA. CANNOT BE UNDONE."

### Tab 4: Protocols — Exploit & Device Profiles

**Local exploit DB**
- Ships `exploits.yaml` in project root
- Format:

```yaml
- device_pattern: "xiaomi mi"
  type: "EDL Auth Bypass"
  risk: "high"
  command: "fastboot oem edl"
- device_pattern: "samsung"
  type: "KG State Check"
  risk: "medium"
  command: "fastboot oem lks"
```

- Rendered as table: Device Pattern | Type | Risk (color-coded) | Action button
- Clicking the action button reveals instructions and command
- Consent checkbox required per exploit activation

**Custom command pane**
- Text input field "Enter custom adb/fastboot command"
- "Run" button executes via executor
- Output streams to console log

### Tab 6: Advanced — Modding Tools

**Boot image analyzer**
- "Pick boot.img" button opens file dialog
- "Analyze" reads boot image header:
  - Magic number (ANDROID!)
  - Kernel size, kernel cmdline
  - Ramdisk size
  - Second stage / DTB presence
- Displayed as key-value table (read-only info)
- "Patch with Magisk" button calls `crate::flash::magisk::patch_boot_image()`
- Output file path shown in label

**DTBO info viewer**
- "Pick .dtbo file" button opens file dialog
- "View Info" parses header: magic (0xd7b7ab1e), total_size, header_size, dt_entry_size, dt_entry_offset
- Displayed as key-value table
- Stub: Phase 3 shows header info only, full DTB parsing in future

**Partition hex viewer**
- Text input: block device path (default: `/dev/block/by-name/boot`)
- "Read" button runs `adb shell dd if=<path> bs=4096 count=1 2>/dev/null | xxd`
- Output in monospace ScrollArea: `<offset>  <hex columns>  <ASCII>`
- Read-only, no write capability

### Tab 7: Settings — Tamper Evidence + Consent

**Tamper-evident action log**
- Every destructive action logged to `~/.androidtool/action_log.json`
- Each entry: `{ "timestamp": "...", "action": "unlock", "serial": "...", "prev_hash": "sha256..." }`
- SHA-256 hash chain (each entry hashes previous entry's content)
- "View Action Log" button opens collapsible table
- Lightweight, local only

**Consent history**
- "View Consent History" shows log of accepted consent dialogs
- Same hash-chain format, stored in `~/.androidtool/consent_log.json`
- Each entry: action, timestamp, device serial, hash

**Geo-lock toggle**
- "Disable exploit features (compliance mode)" checkbox
- When checked: hides exploit DB table, disables custom command pane
- Preference stored in `~/.androidtool/config.yaml`
- Default: OFF (exploits visible)

**Warranty database**
- Ships `warranty_warnings.yaml` in project root
- Format:

```yaml
samsung:
  warning: "Unlocking will trip Knox. Samsung Pay, Secure Folder, and warranty may be permanently lost."
xiaomi:
  warning: "Xiaomi requires Mi Unlock tool approval. Unauthorized unlock may void warranty."
```

- On device detection: if manufacturer matches, shows dismissable warning panel
- Rendered in Dashboard tab AND as part of consent dialogs

### Global: Consent Dialogs

**Standard consent dialog** (reuses Window::new popup pattern from main.rs):
- Title: "Confirm: [Action Name]"
- Body: Action description (bold) + warranty warning if applicable
- Checkbox: "I understand and accept the risks"
- "Proceed" button (disabled until checkbox checked) + "Cancel" button
- Used for: unlock, lock, wipe, exploit, flash

**Malware blocklist**
- Ships `blocklist.yaml`:

```yaml
malware:
  - pattern: "*spynote*"
    warning: "SpyNote RAT detected. This is malware."
  - pattern: "*ahmyth*"
    warning: "AhMyth RAT detected. This is malware."
```

- Checked before sideload (matches filename against patterns)
- Shows warning dialog if matched, user can override with consent

---

## File Changes

| Action | File | Purpose |
|--------|------|---------|
| Modify | `Cargo.toml` | Add `aes-gcm = "0.10"` |
| Modify | `src/ui/backup.rs` | Encryption checkbox + secure wipe buttons |
| Modify | `src/ui/protocols.rs` | Exploit DB table + custom command pane |
| Modify | `src/ui/advanced.rs` | Boot analyzer, DTBO viewer, hex viewer |
| Modify | `src/ui/settings.rs` | Action log, consent history, geo-lock, warranty DB |
| Modify | `src/ui/dashboard.rs` | Warranty warning panel on device detection |
| Create | `src/safety/encryption.rs` | AES-256-GCM encrypt/decrypt helpers |
| Create | `src/safety/audit.rs` | Tamper-evident hash chain logging |
| Create | `src/legal/mod.rs` | Consent dialog, geo-lock state, blocklist loader |
| Create | `src/legal/consent.rs` | Consent dialog rendering |
| Create | `src/legal/blocklist.rs` | Malware pattern checker |
| Create | `src/analyzer/mod.rs` | Modding tool submodule declarations |
| Create | `src/analyzer/boot.rs` | Boot image header parser |
| Create | `src/analyzer/dtbo.rs` | DTBO header parser |
| Create | `exploits.yaml` | Device exploit profiles |
| Create | `warranty_warnings.yaml` | Manufacturer warranty warnings |
| Create | `blocklist.yaml` | Malware filename patterns |

No changes to: `app.rs`, `state.rs`, `executor.rs`, `config.rs`, `main.rs` (unless new Action variants needed).

---

## Data Flow

All features follow existing patterns:
- **ProcessOutput channel** for async command output
- **tokio runtime** for adb/fastboot commands
- **egui Window::new** for consent dialogs
- **FileDialog** for file pickers

---

Current Phase: 3
