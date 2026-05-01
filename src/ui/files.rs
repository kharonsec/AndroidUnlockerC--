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
                ui.label(RichText::new("File Transfer").strong());
            });
            ui.separator();
            ui.add_enabled_ui(has_adb && !disable_actions, |ui| {
                if ui.button("ADB Push File").clicked() {
                    *action = Some(Action::PushFile);
                }
                if ui.button("ADB Pull File").clicked() {
                    *action = Some(Action::PullFile);
                }
            });
        });

        ui.add_space(20.0);
        ui.label(
            RichText::new("Phase 2 Preview")
                .strong()
                .color(Color32::GRAY),
        );
        ui.label("  MTP browser (directory tree, upload/download)");
        ui.label("  Transfer history with status");
    });
}
