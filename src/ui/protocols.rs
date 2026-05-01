use super::{Action, AppState};
use egui::{Color32, RichText};

pub fn render(ui: &mut egui::Ui, state: &AppState, _action: &mut Option<Action>, _disable: bool) {
    ui.vertical(|ui| {
        ui.label(RichText::new("Active Protocol").strong());
        ui.label(format!("Mode: {}", state.device_mode.to_uppercase()));

        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Protocol Modules").strong());
            });
            ui.separator();
            ui.label("EDL (Qualcomm 9008/900E)");
            ui.label("Sahara / Firehose");
            ui.label("BROM (MediaTek)");
            ui.label("Samsung Download Mode");
            ui.label("MTK SP Flash Tool");
            ui.label("Xiaomi MiFlash");
            ui.label("Huawei eRecovery");
            ui.label("Fastbootd");
            ui.label("MTP");
        });

        ui.add_space(20.0);
        ui.label(
            RichText::new("Phase 2 Preview")
                .strong()
                .color(Color32::GRAY),
        );
        ui.label("  Per-protocol advanced operations");
        ui.label("  FRP bypass tools");
        ui.label("  EDL auth bypass");

        ui.add_space(10.0);
        ui.label(
            RichText::new("Phase 3 Preview")
                .strong()
                .color(Color32::GRAY),
        );
        ui.label("  Community exploit database");
        ui.label("  Remote assistance");
    });
}
