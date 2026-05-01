use super::{Action, AppState};
use egui::RichText;

pub fn render(ui: &mut egui::Ui, _state: &AppState, _action: &mut Option<Action>, _disable: bool) {
    egui::ScrollArea::vertical().show(ui, |ui| {
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Preferences").strong());
            });
            ui.separator();
            ui.label("Auto-detect interval: 3s (configurable in Phase 2)");
            ui.label("Backup location: ~/.androidtool/backups");
        });

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Network").strong());
            });
            ui.separator();
            ui.label("Wi-Fi ADB: Phase 2");
            ui.label("Docker mode: Phase 2");
        });

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("About").strong());
            });
            ui.separator();
            ui.label("Android Bootloader & Recovery Tool");
            ui.label("Version: 0.2.0");
            ui.label("Author: Eliott");
            ui.label("License: GPLv3");
        });
    });
}
