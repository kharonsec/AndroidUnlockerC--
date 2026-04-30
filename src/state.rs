use crate::protocols::{DeviceInfo, ProtocolId};

pub struct AppState {
    pub device_mode: String,
    pub device_serial: String,
    pub device_manufacturer: String,
    pub device_model: String,
    pub active_protocol: Option<ProtocolId>,
    pub device_info: Option<DeviceInfo>,

    pub is_command_running: bool,
    pub current_command_id: String,
    pub cancel_tx: Option<tokio::sync::mpsc::Sender<()>>,

    pub logs: Vec<(String, egui::Color32)>,
    pub status_bar: String,

    pub is_amd: bool,
    pub has_quirk: bool,
    pub show_quirk_prompt: bool,
    pub quirk_prompt_dismissed: bool,

    pub selected_tab: usize,
}

impl Default for AppState {
    fn default() -> Self {
        Self {
            device_mode: "none".into(),
            device_serial: "N/A".into(),
            device_manufacturer: "Unknown".into(),
            device_model: "Unknown".into(),
            active_protocol: None,
            device_info: None,
            is_command_running: false,
            current_command_id: String::new(),
            cancel_tx: None,
            logs: vec![],
            status_bar: "Ready.".into(),
            is_amd: false,
            has_quirk: false,
            show_quirk_prompt: false,
            quirk_prompt_dismissed: false,
            selected_tab: 0,
        }
    }
}
