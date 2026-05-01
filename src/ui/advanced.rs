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

    ui.add_space(10.0);
    ui.collapsing("Boot Image Analyzer", |ui| {
        if ui.button("Pick boot.img").clicked() {}
        ui.label("Select a boot image file, then click Analyze.");
        if ui.button("Analyze").clicked() {}
        if ui.button("Patch with Magisk").clicked() {}
    });

    ui.add_space(10.0);
    ui.collapsing("DTBO Info Viewer", |ui| {
        if ui.button("Pick .dtbo file").clicked() {}
        if ui.button("View Info").clicked() {}
    });

    ui.add_space(10.0);
    ui.collapsing("Partition Hex Viewer", |ui| {
        ui.horizontal(|ui| {
            ui.label("Path:");
            let mut path = String::from("/dev/block/by-name/boot");
            ui.add_sized([250.0, 20.0], egui::TextEdit::singleline(&mut path));
            if ui.button("Read").clicked() {}
        });
    });
}
