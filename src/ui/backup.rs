use super::{Action, AppState};
use egui::{Color32, RichText};

pub fn render(
    ui: &mut egui::Ui,
    state: &AppState,
    action: &mut Option<Action>,
    disable_actions: bool,
) {
    let has_adb = state.device_mode == "adb"
        || state.device_mode == "recovery"
        || state.device_mode == "sideload";

    egui::ScrollArea::vertical().show(ui, |ui| {
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Backup").strong());
            });
            ui.separator();
            ui.add_enabled_ui(has_adb && !disable_actions, |ui| {
                if ui.button("Backup Critical Partitions").clicked() {
                    *action = Some(Action::BackupPartitions);
                }
                if ui.button("Full NAND Backup").clicked() {
                    *action = Some(Action::FullNandBackup);
                }
            });
        });

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Restore").strong());
            });
            ui.separator();
            ui.add_enabled_ui(has_adb && !disable_actions, |ui| {
                if ui.button("Restore from Backup").clicked() {
                    *action = Some(Action::RestoreBackup);
                }
            });
        });

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Forensic").strong());
            });
            ui.separator();
            ui.add_enabled_ui(has_adb && !disable_actions, |ui| {
                if ui.button("Forensic Dump (Read-Only)").clicked() {
                    *action = Some(Action::ForensicDump);
                }
            });
        });

        ui.add_space(20.0);
        ui.label(
            RichText::new("Phase 3 Preview")
                .strong()
                .color(Color32::GRAY),
        );
        ui.label("  Post-quantum encrypted cloud backup (Kyber)");
        ui.label("  Secure wipe (DoD 5220.22-M)");
    });
}
