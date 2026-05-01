pub struct ConsentRequest {
    pub title: String,
    pub description: String,
    pub accepted: bool,
    pub show: bool,
}

impl ConsentRequest {
    pub fn new(title: &str, description: &str) -> Self {
        Self {
            title: title.into(),
            description: description.into(),
            accepted: false,
            show: true,
        }
    }

    pub fn show(&mut self, ctx: &egui::Context) -> bool {
        let mut result = false;
        if self.show {
            egui::Window::new(&self.title)
                .collapsible(false)
                .resizable(false)
                .anchor(egui::Align2::CENTER_CENTER, egui::vec2(0.0, 0.0))
                .show(ctx, |ui| {
                    ui.label(egui::RichText::new(&self.description).strong());
                    ui.add_space(10.0);
                    ui.checkbox(&mut self.accepted, "I understand and accept the risks");
                    ui.add_space(10.0);
                    ui.horizontal(|ui| {
                        if ui
                            .add_enabled(self.accepted, egui::Button::new("Proceed"))
                            .clicked()
                        {
                            result = true;
                            self.show = false;
                        }
                        if ui.button("Cancel").clicked() {
                            self.show = false;
                        }
                    });
                });
        }
        result
    }
}
