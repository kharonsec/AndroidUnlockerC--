use super::{Action, AppState};
use egui::{Color32, RichText};

pub fn render(ui: &mut egui::Ui, _state: &AppState, _action: &mut Option<Action>, _disable: bool) {
    egui::ScrollArea::vertical().show(ui, |ui| {
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Preferences").strong());
            });
            ui.separator();
            ui.label("Auto-detect interval: 3s");
            ui.label("Backup location: ~/.androidtool/backups");
        });

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Compliance").strong());
            });
            ui.separator();
            let mut geo_locked = false;
            ui.checkbox(
                &mut geo_locked,
                "Disable exploit features (compliance mode)",
            );
        });

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Wi-Fi ADB").strong());
            });
            ui.separator();
            ui.horizontal(|ui| {
                ui.label("IP:");
                let mut ip = String::from("192.168.1.");
                ui.text_edit_singleline(&mut ip);
                ui.label("Port:");
                let mut port = String::from("5555");
                ui.add_sized([60.0, 20.0], egui::TextEdit::singleline(&mut port));
            });
            ui.horizontal(|ui| {
                if ui.button("Connect").clicked() {}
                if ui.button("Disconnect").clicked() {}
            });
            ui.label("Status: Not connected");
        });

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Docker").strong());
            });
            ui.separator();
            if ui.button("Generate Dockerfile").clicked() {
                let content = crate::network::wifi_adb::generate_dockerfile();
                std::fs::write("Dockerfile", content).ok();
            }
            if ui.button("Generate docker-compose.yml").clicked() {
                let content = crate::network::docker::generate_compose();
                std::fs::write("docker-compose.yml", content).ok();
            }
        });

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Audit").strong());
            });
            ui.separator();
            ui.collapsing("Action Log Entries", |ui| {
                let entries = crate::safety::audit::read_action_log();
                for e in &entries {
                    ui.label(format!("[{}] {} on {}", e.timestamp, e.action, e.serial));
                }
                if entries.is_empty() {
                    ui.label("No actions logged yet.");
                }
            });
            ui.collapsing("Consent History", |ui| {
                ui.label("Consent records logged with each action.");
            });
        });

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("About").strong());
            });
            ui.separator();
            ui.label("Android Bootloader & Recovery Tool");
            ui.label("Version: 0.4.0");
            ui.label("Author: Eliott");
            ui.label("License: GPLv3");
        });
    });
}
