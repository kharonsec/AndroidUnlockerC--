# Android Bootloader & Recovery Tool (Rust / egui)

GUI tool for Android device operations wrapping ADB and Fastboot.

## Building

```bash
cargo build --release
```

## Usage

Place `devices.yaml` next to the binary. Run `./target/release/android_unlocker`.

Requires: `adb` and `fastboot` in PATH.
