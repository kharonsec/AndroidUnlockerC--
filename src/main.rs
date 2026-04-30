#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod config;
mod executor;

use config::FullConfig;
use executor::{Executor, ProcessOutput};

use crossbeam_channel::{unbounded, Receiver};
use eframe::egui;
use eframe::egui::{Color32, RichText, ScrollArea};
use rfd::FileDialog;
use std::sync::Arc;
use tokio::sync::Mutex;

struct AppState {
    device_mode: String,
    device_serial: String,
    device_manufacturer: String,
    device_model: String,

    is_command_running: bool,
    current_command_id: String,
    cancel_tx: Option<tokio::sync::mpsc::Sender<()>>,

    logs: Vec<(String, Color32)>,
    status_bar: String,

    // Quirk related
    is_amd: bool,
    has_quirk: bool,
    show_quirk_prompt: bool,
    quirk_prompt_dismissed: bool,
}

impl Default for AppState {
    fn default() -> Self {
        Self {
            device_mode: "none".to_string(),
            device_serial: "N/A".to_string(),
            device_manufacturer: "Unknown".to_string(),
            device_model: "Unknown".to_string(),
            is_command_running: false,
            current_command_id: String::new(),
            cancel_tx: None,
            logs: vec![],
            status_bar: "Ready.".to_string(),
            is_amd: false,
            has_quirk: false,
            show_quirk_prompt: false,
            quirk_prompt_dismissed: false,
        }
    }
}

pub struct AndroidUnlockerApp {
    state: Arc<Mutex<AppState>>,
    config: FullConfig,
    rx: Receiver<ProcessOutput>,
    rt: tokio::runtime::Runtime,
    executor: Executor,
    last_checked_time: std::time::Instant,
}

impl AndroidUnlockerApp {
    pub fn new(_cc: &eframe::CreationContext<'_>) -> Self {
        let (tx, rx) = unbounded();
        let rt = tokio::runtime::Runtime::new().expect("Unable to create Runtime");
        let executor = Executor::new(tx.clone(), rt.handle().clone());
        let config = FullConfig::load("devices.yaml");

        let app = Self {
            state: Arc::new(Mutex::new(AppState::default())),
            config,
            rx,
            rt,
            executor,
            last_checked_time: std::time::Instant::now(),
        };

        app.check_system_info();
        app
    }

    fn check_system_info(&self) {
        let state_clone = self.state.clone();
        self.rt.spawn(async move {
            let is_amd = if let Ok(cpuinfo) = std::fs::read_to_string("/proc/cpuinfo") {
                cpuinfo.contains("AuthenticAMD")
            } else {
                false
            };

            let has_quirk = if let Ok(quirks) =
                std::fs::read_to_string("/sys/module/usbcore/parameters/quirks")
            {
                quirks.contains("18d1:d00d:k")
            } else {
                false
            };

            let mut st = state_clone.lock().await;
            st.is_amd = is_amd;
            st.has_quirk = has_quirk;
        });
    }

    fn apply_quirk(&self) {
        let tx = self.executor.new_tx();
        self.rt.spawn(async move {
            let cmd = "echo '18d1:d00d:k' | sudo tee /sys/module/usbcore/parameters/quirks";
            let _ = tx.send(ProcessOutput::Stdout(
                "Applying temporary USB quirk...".into(),
            ));
            let output = std::process::Command::new("sh").arg("-c").arg(cmd).output();

            match output {
                Ok(out) if out.status.success() => {
                    let _ = tx.send(ProcessOutput::Stdout("Quirk applied successfully!".into()));
                }
                _ => {
                    let _ = tx.send(ProcessOutput::Error(
                        "Failed to apply quirk. Root permissions may be required.".into(),
                    ));
                }
            }
        });
    }

    fn append_log(&self, text: &str, color: Color32) {
        if let Ok(mut state) = self.state.try_lock() {
            state.logs.push((text.to_string(), color));
        }
    }

    fn set_status(&self, text: &str) {
        if let Ok(mut state) = self.state.try_lock() {
            state.status_bar = text.to_string();
        }
    }

    fn run_cmd(&self, id: &str, command_str: &str, warning: Option<&str>) {
        if command_str.is_empty() {
            self.append_log(
                &format!("[ERROR] Command '{}' is empty in config!", id),
                Color32::RED,
            );
            return;
        }

        if let Some(msg) = warning {
            self.append_log(&format!("[WARNING] {}", msg), Color32::YELLOW);
        }

        let parts: Vec<&str> = command_str.split_whitespace().collect();
        if parts.is_empty() {
            return;
        }

        let program = parts[0].to_string();
        let args: Vec<String> = parts[1..].iter().map(|&s| s.to_string()).collect();

        self.start_process(id, program, args);
    }

    fn start_process(&self, id: &str, program: String, args: Vec<String>) {
        if let Ok(mut state) = self.state.try_lock() {
            if state.is_command_running {
                state.logs.push((
                    "[WARNING] A command is already running.".to_string(),
                    Color32::YELLOW,
                ));
                return;
            }

            state.is_command_running = true;
            state.current_command_id = id.to_string();

            let (cancel_tx, cancel_rx) = tokio::sync::mpsc::channel(1);
            state.cancel_tx = Some(cancel_tx);

            self.append_log(
                &format!("[*] Starting command: {} {:?}", program, args),
                Color32::LIGHT_BLUE,
            );
            self.executor
                .run_command(id.to_string(), program, args, cancel_rx);
        }
    }

    fn check_device_state(&mut self) {
        if self.last_checked_time.elapsed().as_secs() < 3 {
            return;
        }
        self.last_checked_time = std::time::Instant::now();

        if let Ok(state) = self.state.try_lock() {
            if state.is_command_running {
                return;
            }
        }

        let state_clone = self.state.clone();

        self.rt.spawn(async move {
            let mut detected_mode = "none".to_string();
            let mut detected_serial = "N/A".to_string();

            // Check ADB
            if let Ok((success, stdout, _)) = Executor::run_command_sync("adb", &["devices"]).await
            {
                if success {
                    let lines: Vec<&str> = stdout.lines().collect();
                    for line in lines.iter().skip(1) {
                        let parts: Vec<&str> = line.split_whitespace().collect();
                        if parts.len() == 2 {
                            let serial = parts[0];
                            let status = parts[1];
                            if status == "device" {
                                detected_mode = "adb".to_string();
                                detected_serial = serial.to_string();
                                break;
                            } else if status == "recovery" {
                                detected_mode = "recovery".to_string();
                                detected_serial = serial.to_string();
                                break;
                            } else if status == "sideload" {
                                detected_mode = "sideload".to_string();
                                detected_serial = serial.to_string();
                                break;
                            }
                        }
                    }
                }
            }

            // Check Fastboot if ADB not found
            if detected_mode == "none" {
                if let Ok((success, stdout, _)) =
                    Executor::run_command_sync("fastboot", &["devices"]).await
                {
                    if success {
                        let lines: Vec<&str> = stdout.lines().collect();
                        for line in lines {
                            let parts: Vec<&str> = line.split_whitespace().collect();
                            if parts.len() >= 2 && parts[1] == "fastboot" {
                                detected_mode = "fastboot".to_string();
                                detected_serial = parts[0].to_string();
                                break;
                            }
                        }
                    }
                }
            }

            let mut st = state_clone.lock().await;
            st.device_mode = detected_mode.clone();
            st.device_serial = detected_serial;

            if st.device_mode == "none" {
                st.device_manufacturer = "Unknown".to_string();
                st.device_model = "Unknown".to_string();
            }
        });
    }

    fn run_detect_details(&self) {
        let mode = {
            let st = self.state.try_lock().unwrap();
            st.device_mode.clone()
        };

        if mode == "adb" || mode == "recovery" || mode == "sideload" {
            self.set_status("Getting ADB properties...");
            self.start_process(
                "detect_adb_props",
                "adb".to_string(),
                vec!["shell".to_string(), "getprop".to_string()],
            );
        } else if mode == "fastboot" {
            self.set_status("Getting Fastboot variables...");
            self.start_process(
                "detect_fastboot_vars",
                "fastboot".to_string(),
                vec!["getvar".to_string(), "all".to_string()],
            );
        }
    }

    fn render_info_area(
        &self,
        ui: &mut egui::Ui,
        st: &AppState,
        action: &mut Action,
        disable_actions: bool,
    ) {
        ui.add_space(10.0);
        ui.group(|ui| {
            ui.vertical_centered(|ui| {
                ui.label(RichText::new("Device Info").strong());
            });
            ui.separator();
            ui.label(RichText::new(format!("Manufacturer: {}", st.device_manufacturer)).strong());
            ui.label(RichText::new(format!("Model: {}", st.device_model)).strong());
            ui.label(RichText::new(format!("Serial: {}", st.device_serial)).strong());
        });

        ui.add_space(20.0);

        if st.is_amd {
            ui.group(|ui| {
                ui.label(
                    RichText::new("System (AMD Detected)").color(Color32::from_rgb(200, 50, 50)),
                );
                if st.has_quirk {
                    ui.label(
                        RichText::new("✔ USB Quirk Applied").color(Color32::from_rgb(50, 150, 50)),
                    );
                } else {
                    ui.label(RichText::new("⚠ USB Quirk Missing").strong());
                    if ui.button("Apply Fix (Temporary)").clicked() {
                        *action = Action::ApplyQuirk;
                    }
                }
            });
        }

        ui.add_space(20.0);
        ui.vertical_centered(|ui| {
            ui.add_enabled_ui(!disable_actions, |ui| {
                if ui.button("🔍 Detect Device Details").clicked() {
                    *action = Action::DetectDetails;
                }
            });
        });
    }

    fn render_console_area(&self, ui: &mut egui::Ui, st: &AppState) {
        ui.label(RichText::new("Action Console").strong());
        ui.add_space(4.0);

        ScrollArea::vertical().stick_to_bottom(true).show(ui, |ui| {
            ui.set_min_width(ui.available_width());
            for (log, color) in &st.logs {
                ui.label(RichText::new(log).color(*color).monospace());
            }
        });
    }

    fn render_buttons_area(
        &self,
        ui: &mut egui::Ui,
        is_fastboot: bool,
        is_adb: bool,
        is_recovery: bool,
        is_sideload: bool,
        disable_actions: bool,
        action: &mut Action,
    ) {
        ui.horizontal_wrapped(|ui| {
            ui.spacing_mut().item_spacing = egui::vec2(8.0, 8.0);

            ui.add_enabled_ui(is_fastboot && !disable_actions, |ui| {
                if ui.button("🔓 Unlock Bootloader").clicked() {
                    *action = Action::UnlockBootloader;
                }
                if ui.button("🔒 Lock Bootloader").clicked() {
                    *action = Action::LockBootloader;
                }
                if ui.button("📥 Flash Recovery").clicked() {
                    *action = Action::FlashRecovery;
                }
                if ui.button("⚡ Flash Boot").clicked() {
                    *action = Action::FlashBoot;
                }
            });

            ui.add_enabled_ui(
                (is_adb || is_fastboot || is_recovery || is_sideload) && !disable_actions,
                |ui| {
                    if ui.button("🔄 Reboot System").clicked() {
                        *action = Action::RebootSystem;
                    }
                },
            );

            ui.add_enabled_ui(
                (is_adb || is_recovery || is_sideload) && !disable_actions,
                |ui| {
                    if ui.button("🛠 Reboot Recovery").clicked() {
                        *action = Action::RebootRecovery;
                    }
                    if ui.button("👾 Reboot Bootloader").clicked() {
                        *action = Action::RebootBootloader;
                    }
                },
            );

            ui.add_enabled_ui(is_sideload && !disable_actions, |ui| {
                if ui.button("📦 ADB Sideload").clicked() {
                    *action = Action::Sideload;
                }
            });
        });
    }
}

#[derive(PartialEq)]
enum Action {
    None,
    ApplyQuirk,
    DetectDetails,
    UnlockBootloader,
    LockBootloader,
    FlashRecovery,
    FlashBoot,
    RebootSystem,
    RebootRecovery,
    RebootBootloader,
    Sideload,
}

impl eframe::App for AndroidUnlockerApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        // Set professional white theme
        ctx.set_visuals(egui::Visuals::light());

        self.check_device_state();

        // Process incoming events
        while let Ok(event) = self.rx.try_recv() {
            let mut state = self.state.try_lock().unwrap();
            match event {
                ProcessOutput::Stdout(line) => {
                    state.logs.push((
                        format!("[STDOUT] {}", line),
                        Color32::from_rgb(100, 100, 100),
                    ));

                    if state.current_command_id == "detect_adb_props" {
                        if line.contains("ro.product.manufacturer") {
                            if let Some(val) = line.split("]: [").nth(1) {
                                state.device_manufacturer = val.trim_end_matches(']').to_string();
                            }
                        }
                        if line.contains("ro.product.model") {
                            if let Some(val) = line.split("]: [").nth(1) {
                                state.device_model = val.trim_end_matches(']').to_string();
                            }
                        }
                    }
                }
                ProcessOutput::Stderr(line) => {
                    if state.current_command_id == "detect_fastboot_vars" {
                        if line.starts_with("(bootloader) product:") {
                            if let Some(val) = line.split(':').nth(1) {
                                state.device_model = val.trim().to_string() + " (codename)";
                            }
                        }
                    }
                    state
                        .logs
                        .push((format!("[STDERR] {}", line), Color32::from_rgb(200, 50, 50)));
                }
                ProcessOutput::Error(e) => {
                    state.logs.push((format!("[ERROR] {}", e), Color32::RED));
                    state.status_bar = "Error occurred.".to_string();
                }
                ProcessOutput::Finished(success, id) => {
                    state.is_command_running = false;
                    state.current_command_id.clear();
                    state.cancel_tx = None;

                    if success {
                        state.logs.push((
                            format!("[SUCCESS] Command completed successfully."),
                            Color32::from_rgb(50, 150, 50),
                        ));
                        state.status_bar = format!("Command '{}' successful.", id);
                    } else {
                        state
                            .logs
                            .push(("[ERROR] Command failed.".to_string(), Color32::DARK_RED));
                        state.status_bar = format!("Command '{}' failed.", id);
                    }
                }
            }
        }

        let mut action = Action::None;

        // Lock state for UI rendering
        {
            let mut st = match self.state.try_lock() {
                Ok(s) => s,
                Err(_) => return,
            };

            // Logic to prompt for quirk
            if st.is_amd
                && !st.has_quirk
                && st.device_manufacturer.to_lowercase().contains("xiaomi")
                && !st.quirk_prompt_dismissed
            {
                st.show_quirk_prompt = true;
            }

            let is_running = st.is_command_running;
            let mode = st.device_mode.clone();
            let is_adb = mode == "adb";
            let is_fastboot = mode == "fastboot";
            let is_sideload = mode == "sideload";
            let is_recovery = mode == "recovery";
            let disable_actions = is_running;

            egui::TopBottomPanel::top("header").show(ctx, |ui| {
                ui.add_space(8.0);
                ui.horizontal(|ui| {
                    ui.vertical(|ui| {
                        ui.label(
                            RichText::new("Android Bootloader & Recovery Tool")
                                .size(20.0)
                                .strong(),
                        );
                        ui.label(RichText::new("Connected Device Details").color(Color32::GRAY));
                    });

                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                        let mode_color = match mode.as_str() {
                            "adb" => Color32::from_rgb(50, 150, 50),
                            "fastboot" => Color32::from_rgb(200, 100, 0),
                            "none" => Color32::GRAY,
                            _ => Color32::from_rgb(0, 100, 200),
                        };
                        ui.label(
                            RichText::new(mode.to_uppercase())
                                .strong()
                                .size(24.0)
                                .color(mode_color),
                        );
                        ui.label("MODE:");
                    });
                });
                ui.add_space(8.0);
            });

            egui::TopBottomPanel::bottom("status_bar").show(ctx, |ui| {
                ui.horizontal(|ui| {
                    ui.label(RichText::new(&st.status_bar).small());
                    if is_running {
                        ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                            if ui.button("Cancel Operation").clicked() {
                                if let Some(tx) = st.cancel_tx.take() {
                                    let _ = tx.try_send(());
                                }
                            }
                        });
                    }
                });
            });

            egui::TopBottomPanel::bottom("buttons_panel")
                .resizable(false)
                .show(ctx, |ui| {
                    ui.add_space(4.0);
                    self.render_buttons_area(
                        ui,
                        is_fastboot,
                        is_adb,
                        is_recovery,
                        is_sideload,
                        disable_actions,
                        &mut action,
                    );
                    ui.add_space(4.0);
                });

            let screen_width = ctx.screen_rect().width();
            let is_narrow = screen_width < 750.0;

            if !is_narrow {
                egui::SidePanel::left("info_panel")
                    .resizable(true)
                    .default_width(260.0)
                    .width_range(200.0..=400.0)
                    .show(ctx, |ui| {
                        ScrollArea::vertical().show(ui, |ui| {
                            self.render_info_area(ui, &st, &mut action, disable_actions);
                        });
                    });
            }

            egui::CentralPanel::default().show(ctx, |ui| {
                ui.vertical(|ui| {
                    if is_narrow {
                        ui.collapsing("📱 Device & System Info", |ui| {
                            self.render_info_area(ui, &st, &mut action, disable_actions);
                        });
                        ui.separator();
                    }

                    self.render_console_area(ui, &st);
                });

                if st.show_quirk_prompt {
                    egui::Window::new("AMD Xiaomi Fix Required")
                        .collapsible(false)
                        .resizable(false)
                        .anchor(egui::Align2::CENTER_CENTER, egui::vec2(0.0, 0.0))
                        .show(ctx, |ui| {
                            ui.label(
                                "It looks like you are using an AMD system and a Xiaomi device.",
                            );
                            ui.label(
                                "Xiaomi devices often disconnect in fastboot mode on AMD hardware.",
                            );
                            ui.label("Would you like to apply the USB quirk fix?");
                            ui.add_space(10.0);
                            ui.horizontal(|ui| {
                                if ui.button("Apply Fix").clicked() {
                                    action = Action::ApplyQuirk;
                                    st.show_quirk_prompt = false;
                                }
                                if ui.button("Dismiss").clicked() {
                                    st.show_quirk_prompt = false;
                                    st.quirk_prompt_dismissed = true;
                                }
                            });
                        });
                }
            });
        }

        // Execute action outside lock
        if action != Action::None {
            let config = {
                let st = self.state.try_lock().unwrap();
                self.config
                    .get_config(&st.device_manufacturer, &st.device_model)
            };
            let mode = {
                let st = self.state.try_lock().unwrap();
                st.device_mode.clone()
            };

            match action {
                Action::ApplyQuirk => self.apply_quirk(),
                Action::DetectDetails => self.run_detect_details(),
                Action::UnlockBootloader => {
                    if let Some(cmd) = config.unlock_command {
                        self.run_cmd("unlock", &cmd, config.unlock_warning.as_deref());
                    }
                }
                Action::LockBootloader => {
                    if let Some(cmd) = config.lock_command {
                        self.run_cmd("lock", &cmd, config.lock_warning.as_deref());
                    }
                }
                Action::FlashRecovery => {
                    if let Some(path) = FileDialog::new().add_filter("image", &["img"]).pick_file()
                    {
                        let partition = config
                            .recovery_partition
                            .unwrap_or_else(|| "recovery".into());
                        self.start_process(
                            "flash_recovery",
                            "fastboot".into(),
                            vec![
                                "flash".into(),
                                partition,
                                path.to_string_lossy().into_owned(),
                            ],
                        );
                    }
                }
                Action::FlashBoot => {
                    if let Some(path) = FileDialog::new().add_filter("image", &["img"]).pick_file()
                    {
                        let partition = config.boot_partition.unwrap_or_else(|| "boot".into());
                        self.start_process(
                            "flash_boot",
                            "fastboot".into(),
                            vec![
                                "flash".into(),
                                partition,
                                path.to_string_lossy().into_owned(),
                            ],
                        );
                    }
                }
                Action::RebootSystem => {
                    let cmd = if mode == "fastboot" {
                        config
                            .fastboot_reboot_command
                            .unwrap_or_else(|| "fastboot reboot".into())
                    } else {
                        config
                            .adb_reboot_system_command
                            .unwrap_or_else(|| "adb reboot".into())
                    };
                    self.run_cmd("reboot_system", &cmd, None);
                }
                Action::RebootRecovery => {
                    let cmd = config
                        .adb_reboot_recovery_command
                        .unwrap_or_else(|| "adb reboot recovery".into());
                    self.run_cmd("reboot_recovery", &cmd, None);
                }
                Action::RebootBootloader => {
                    let cmd = config
                        .adb_reboot_bootloader_command
                        .unwrap_or_else(|| "adb reboot bootloader".into());
                    self.run_cmd("reboot_bootloader", &cmd, None);
                }
                Action::Sideload => {
                    if let Some(path) = FileDialog::new().add_filter("zip", &["zip"]).pick_file() {
                        self.start_process(
                            "sideload",
                            "adb".into(),
                            vec!["sideload".into(), path.to_string_lossy().into_owned()],
                        );
                    }
                }
                _ => {}
            }
        }
    }
}

fn main() -> eframe::Result<()> {
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([900.0, 700.0])
            .with_title("Enhanced Android Bootloader & Recovery Tool (Rust)"),
        ..Default::default()
    };
    eframe::run_native(
        "Android Unlocker Rust",
        options,
        Box::new(|cc| Box::new(AndroidUnlockerApp::new(cc))),
    )
}
