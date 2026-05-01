use super::{Action, AppState};
use egui::{Color32, RichText};

pub fn render(ui: &mut egui::Ui, state: &AppState, _action: &mut Option<Action>, _disable: bool) {
    let has_adb = state.device_mode == "adb" || state.device_mode == "recovery";

    egui::ScrollArea::vertical().show(ui, |ui| {
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Debugging").strong());
            });
            ui.separator();
            ui.add_enabled_ui(has_adb, |ui| {
                if ui.button("Start logcat").clicked() {}
                if ui.button("Capture dmesg").clicked() {}
            });
        });

        ui.add_space(20.0);
        ui.label(
            RichText::new("Phase 2 Preview")
                .strong()
                .color(Color32::GRAY),
        );
        ui.label("  Live logcat with filtering");
        ui.label("  dmesg viewer");
        ui.label("  USB traffic sniffer");
        ui.label("  Partition hex editor");
        ui.label("  Boot image patcher (Magisk/Kitsune)");
        ui.label("  DTBO overlays editor");
        ui.label("  SELinux policy explorer");
    });
}
