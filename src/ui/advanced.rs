use super::{Action, AppState};
use egui::{Color32, RichText};

pub fn render(ui: &mut egui::Ui, state: &AppState, action: &mut Option<Action>, _disable: bool) {
    let has_adb = state.device_mode == "adb" || state.device_mode == "recovery";

    ui.add_space(10.0);
    ui.group(|ui| {
        ui.vertical_centered(|ui| {
            ui.label(RichText::new("Live logcat").strong());
        });
        ui.separator();
        ui.add_enabled_ui(has_adb, |ui| {
            ui.horizontal(|ui| {
                if ui.button("Start logcat").clicked() {
                    *action = Some(Action::StartLogcat);
                }
                if ui.button("Stop logcat").clicked() {
                    *action = Some(Action::StopLogcat);
                }
            });
        });
    });

    ui.add_space(10.0);
    ui.group(|ui| {
        ui.vertical_centered(|ui| {
            ui.label(RichText::new("dmesg Viewer").strong());
        });
        ui.separator();
        ui.add_enabled_ui(has_adb, |ui| {
            if ui.button("Capture dmesg").clicked() {
                *action = Some(Action::CaptureDmesg);
            }
        });
    });

    ui.add_space(20.0);
    ui.label(
        RichText::new("Phase 3 Preview")
            .strong()
            .color(Color32::GRAY),
    );
    ui.label("  USB traffic sniffer");
    ui.label("  Partition hex editor");
    ui.label("  Boot image patcher (Magisk/Kitsune auto-patch)");
    ui.label("  DTBO overlays editor");
    ui.label("  SELinux policy explorer");
}
