use super::{Action, AppState};
use crate::config::{load_profile, FlashProfile};
use egui::{Color32, RichText};

pub fn render(
    ui: &mut egui::Ui,
    state: &AppState,
    action: &mut Option<Action>,
    disable_actions: bool,
) {
    let is_fastboot = state.device_mode == "fastboot";
    let is_adb = state.device_mode == "adb"
        || state.device_mode == "recovery"
        || state.device_mode == "sideload";
    let is_sideload = state.device_mode == "sideload";

    ui.horizontal(|ui| {
        egui::SidePanel::left("flash_profiles")
            .resizable(true)
            .default_width(180.0)
            .width_range(140.0..=300.0)
            .show_inside(ui, |ui| {
                render_profiles(ui, action);
            });

        egui::CentralPanel::default().show_inside(ui, |ui| {
            egui::ScrollArea::vertical().show(ui, |ui| {
                render_fastboot_ops(ui, is_fastboot, disable_actions, action);
                ui.add_space(10.0);
                render_adb_ops(ui, is_adb, is_sideload, disable_actions, action);
                ui.add_space(10.0);
                render_batch_panel(ui, state, is_fastboot || is_adb, disable_actions, action);
                ui.add_space(20.0);
                ui.label(
                    RichText::new("Coming in Phase 3")
                        .strong()
                        .color(Color32::GRAY),
                );
                ui.label("  Auto-download firmware/TWRP/Magisk");
            });
        });
    });
}

fn render_profiles(ui: &mut egui::Ui, _action: &mut Option<Action>) {
    ui.vertical_centered(|ui| {
        ui.label(RichText::new("Flash Profiles").strong());
    });
    ui.separator();

    let profiles_dir = dirs_profiles();
    if let Ok(entries) = std::fs::read_dir(&profiles_dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.extension().is_some_and(|e| e == "yaml") {
                let name = path
                    .file_stem()
                    .unwrap_or_default()
                    .to_string_lossy()
                    .to_string();
                if ui.button(&name).clicked() {
                    if let Ok(profile) = load_profile(&path.to_string_lossy()) {
                        ui.ctx().memory_mut(|mem| {
                            mem.data.insert_temp::<FlashProfile>(
                                egui::Id::new("loaded_profile"),
                                profile,
                            );
                        });
                    }
                }
            }
        }
    }

    ui.add_space(8.0);
    ui.button("Save Current Config").clicked();
    ui.button("Export Profile").clicked();
    if ui.button("Import Profile").clicked() {}
}

fn dirs_profiles() -> std::path::PathBuf {
    let home = std::env::var("HOME").unwrap_or_else(|_| ".".into());
    let dir = std::path::PathBuf::from(home)
        .join(".androidtool")
        .join("profiles");
    std::fs::create_dir_all(&dir).ok();
    dir
}

fn render_fastboot_ops(
    ui: &mut egui::Ui,
    is_fastboot: bool,
    disable: bool,
    action: &mut Option<Action>,
) {
    ui.group(|ui| {
        ui.vertical_centered(|ui| {
            ui.label(RichText::new("Fastboot Operations").strong());
        });
        ui.separator();
        ui.add_enabled_ui(is_fastboot && !disable, |ui| {
            if ui.button("Unlock Bootloader").clicked() {
                *action = Some(Action::UnlockBootloader);
            }
            if ui.button("Lock Bootloader").clicked() {
                *action = Some(Action::LockBootloader);
            }
            if ui.button("Flash Recovery Image").clicked() {
                *action = Some(Action::FlashRecovery);
            }
            ui.horizontal(|ui| {
                if ui.button("Flash Boot Image").clicked() {
                    *action = Some(Action::FlashBoot);
                }
                ui.checkbox(&mut false, "Patch with Magisk");
            });
        });
    });
}

fn render_adb_ops(
    ui: &mut egui::Ui,
    is_adb: bool,
    is_sideload: bool,
    disable: bool,
    action: &mut Option<Action>,
) {
    ui.group(|ui| {
        ui.vertical_centered(|ui| {
            ui.label(RichText::new("ADB Operations").strong());
        });
        ui.separator();
        ui.add_enabled_ui(is_adb && !disable, |ui| {
            if ui.button("Reboot System").clicked() {
                *action = Some(Action::RebootSystem);
            }
            if ui.button("Reboot Recovery").clicked() {
                *action = Some(Action::RebootRecovery);
            }
            if ui.button("Reboot Bootloader").clicked() {
                *action = Some(Action::RebootBootloader);
            }
        });
        ui.add_enabled_ui(is_sideload && !disable, |ui| {
            if ui.button("ADB Sideload ZIP").clicked() {
                *action = Some(Action::Sideload);
            }
        });
    });
}

fn render_batch_panel(
    ui: &mut egui::Ui,
    state: &AppState,
    has_device: bool,
    disable: bool,
    _action: &mut Option<Action>,
) {
    ui.collapsing("Batch Operations", |ui| {
        ui.add_enabled_ui(has_device && !disable, |ui| {
            ui.label("Connected devices:");
            ui.label(format!(
                "  1 device: {} ({})",
                state.device_serial, state.device_mode
            ));
            ui.label("Multi-device selection available in Phase 3.");
        });
    });
}
