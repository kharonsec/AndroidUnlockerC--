pub mod advanced;
pub mod backup;
pub mod dashboard;
pub mod files;
pub mod flash;
pub mod protocols;
pub mod settings;

use super::state::AppState;
use super::Action;

pub struct Tab {
    pub name: &'static str,
    pub id: &'static str,
}

pub const TABS: &[Tab] = &[
    Tab {
        name: "Dashboard",
        id: "dashboard",
    },
    Tab {
        name: "Flash",
        id: "flash",
    },
    Tab {
        name: "Backup & Restore",
        id: "backup",
    },
    Tab {
        name: "Protocols",
        id: "protocols",
    },
    Tab {
        name: "Files",
        id: "files",
    },
    Tab {
        name: "Advanced",
        id: "advanced",
    },
    Tab {
        name: "Settings",
        id: "settings",
    },
];

pub fn render_tab_content(
    ui: &mut egui::Ui,
    tab_index: usize,
    state: &AppState,
    action: &mut Option<Action>,
    disable_actions: bool,
) {
    match tab_index {
        0 => dashboard::render(ui, state, action, disable_actions),
        1 => flash::render(ui, state, action, disable_actions),
        2 => backup::render(ui, state, action, disable_actions),
        3 => protocols::render(ui, state, action, disable_actions),
        4 => files::render(ui, state, action, disable_actions),
        5 => advanced::render(ui, state, action, disable_actions),
        6 => settings::render(ui, state, action, disable_actions),
        _ => {}
    }
}
