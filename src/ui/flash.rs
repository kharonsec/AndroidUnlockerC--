use super::{Action, AppState};
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

    egui::ScrollArea::vertical().show(ui, |ui| {
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Fastboot Operations").strong());
            });
            ui.separator();
            ui.add_enabled_ui(is_fastboot && !disable_actions, |ui| {
                if ui.button("Unlock Bootloader").clicked() {
                    *action = Some(Action::UnlockBootloader);
                }
                if ui.button("Lock Bootloader").clicked() {
                    *action = Some(Action::LockBootloader);
                }
                if ui.button("Flash Recovery Image").clicked() {
                    *action = Some(Action::FlashRecovery);
                }
                if ui.button("Flash Boot Image").clicked() {
                    *action = Some(Action::FlashBoot);
                }
            });
        });

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("ADB Operations").strong());
            });
            ui.separator();
            ui.add_enabled_ui(is_adb && !disable_actions, |ui| {
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
            ui.add_enabled_ui(is_sideload && !disable_actions, |ui| {
                if ui.button("ADB Sideload ZIP").clicked() {
                    *action = Some(Action::Sideload);
                }
            });
        });

        ui.add_space(20.0);
        ui.label(
            RichText::new("Coming in Phase 2")
                .strong()
                .color(Color32::GRAY),
        );
        ui.label("  Batch operations (flash multiple devices)");
        ui.label("  Flash profiles (save/load configs)");
        ui.label("  Auto-download firmware/TWRP/Magisk");
        ui.label("  Root inject (Magisk/Kitsune Mask)");
    });
}
