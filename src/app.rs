use crate::advanced::dmesg::DmesgState;
use crate::advanced::logcat::LogcatState;
use crate::config::FullConfig;
use crate::executor::{Executor, ProcessOutput};
use crate::state::AppState;
use crossbeam_channel::{unbounded, Receiver};
use eframe::egui::Color32;
use std::sync::Arc;
use tokio::sync::Mutex;

#[derive(PartialEq)]
pub enum Action {
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
    PushFile,
    PullFile,
    BackupPartitions,
    RestoreBackup,
    FullNandBackup,
    ForensicDump,
    StartLogcat,
    StopLogcat,
    CaptureDmesg,
    ToggleTab(usize),
}

pub struct AndroidUnlockerApp {
    pub state: Arc<Mutex<AppState>>,
    pub config: FullConfig,
    pub rx: Receiver<ProcessOutput>,
    pub rt: tokio::runtime::Runtime,
    pub executor: Executor,
    last_checked_time: std::time::Instant,
    pub logcat_state: Arc<Mutex<LogcatState>>,
    pub dmesg_state: Arc<Mutex<DmesgState>>,
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
            logcat_state: Arc::new(Mutex::new(LogcatState::default())),
            dmesg_state: Arc::new(Mutex::new(DmesgState::default())),
        };

        app.check_system_info();
        app
    }

    fn check_system_info(&self) {
        let state_clone = self.state.clone();
        self.rt.spawn(async move {
            let is_amd = std::fs::read_to_string("/proc/cpuinfo")
                .map(|s| s.contains("AuthenticAMD"))
                .unwrap_or(false);
            let has_quirk = std::fs::read_to_string("/sys/module/usbcore/parameters/quirks")
                .map(|s| s.contains("18d1:d00d:k"))
                .unwrap_or(false);
            let mut st = state_clone.lock().await;
            st.is_amd = is_amd;
            st.has_quirk = has_quirk;
        });
    }

    pub fn apply_quirk(&self) {
        let tx = self.executor.new_tx();
        self.rt.spawn(async move {
            let _ = tx.send(ProcessOutput::Stdout("Applying temporary USB quirk...".into()));
            let output = std::process::Command::new("sh")
                .arg("-c")
                .arg("echo '18d1:d00d:k' | sudo tee /sys/module/usbcore/parameters/quirks")
                .output();
            match output {
                Ok(out) if out.status.success() => {
                    let _ = tx.send(ProcessOutput::Stdout("Quirk applied successfully!".into()));
                }
                _ => {
                    let _ = tx.send(ProcessOutput::Error("Failed to apply quirk. Root permissions may be required.".into()));
                }
            }
        });
    }

    pub fn append_log(&self, text: &str, color: Color32) {
        if let Ok(mut state) = self.state.try_lock() {
            state.logs.push((text.to_string(), color));
        }
    }

    pub fn set_status(&self, text: &str) {
        if let Ok(mut state) = self.state.try_lock() {
            state.status_bar = text.to_string();
        }
    }

    pub fn run_cmd(&self, id: &str, command_str: &str, warning: Option<&str>) {
        if command_str.is_empty() {
            self.append_log(&format!("[ERROR] Command '{}' is empty!", id), Color32::RED);
            return;
        }
        if let Some(msg) = warning {
            self.append_log(&format!("[WARNING] {}", msg), Color32::YELLOW);
        }
        let parts: Vec<&str> = command_str.split_whitespace().collect();
        if parts.is_empty() { return; }
        self.start_process(id, parts[0].into(), parts[1..].iter().map(|s| s.to_string()).collect());
    }

    pub fn start_process(&self, id: &str, program: String, args: Vec<String>) {
        if let Ok(mut state) = self.state.try_lock() {
            if state.is_command_running {
                state.logs.push(("[WARNING] A command is already running.".into(), Color32::YELLOW));
                return;
            }
            state.is_command_running = true;
            state.current_command_id = id.to_string();
            let (cancel_tx, cancel_rx) = tokio::sync::mpsc::channel(1);
            state.cancel_tx = Some(cancel_tx);
            self.append_log(&format!("[*] Starting: {} {:?}", program, args), Color32::LIGHT_BLUE);
            self.executor.run_command(id.to_string(), program, args, cancel_rx);
        }
    }

    pub fn check_device_state(&mut self) {
        if self.last_checked_time.elapsed().as_secs() < 3 { return; }
        self.last_checked_time = std::time::Instant::now();
        {
            if let Ok(state) = self.state.try_lock() {
                if state.is_command_running { return; }
            }
        }
        let state_clone = self.state.clone();
        self.rt.spawn(async move {
            let mut detected_mode = "none".to_string();
            let mut detected_serial = "N/A".to_string();
            if let Ok((success, stdout, _)) = crate::executor::Executor::run_command_sync("adb", &["devices"]).await {
                if success {
                    for line in stdout.lines().skip(1) {
                        let parts: Vec<&str> = line.split_whitespace().collect();
                        if parts.len() == 2 {
                            let status = parts[1];
                            match status {
                                "device" => { detected_mode = "adb".into(); detected_serial = parts[0].into(); break; }
                                "recovery" => { detected_mode = "recovery".into(); detected_serial = parts[0].into(); break; }
                                "sideload" => { detected_mode = "sideload".into(); detected_serial = parts[0].into(); break; }
                                _ => {}
                            }
                        }
                    }
                }
            }
            if detected_mode == "none" {
                if let Ok((success, stdout, _)) = crate::executor::Executor::run_command_sync("fastboot", &["devices"]).await {
                    if success {
                        for line in stdout.lines() {
                            let parts: Vec<&str> = line.split_whitespace().collect();
                            if parts.len() >= 2 && parts[1] == "fastboot" {
                                detected_mode = "fastboot".into();
                                detected_serial = parts[0].into();
                                break;
                            }
                        }
                    }
                }
            }
            let mut st = state_clone.lock().await;
            st.device_mode = detected_mode;
            st.device_serial = detected_serial;
            if st.device_mode == "none" {
                st.device_manufacturer = "Unknown".into();
                st.device_model = "Unknown".into();
            }
        });
    }

    pub fn run_detect_details(&self) {
        let mode = { self.state.try_lock().unwrap().device_mode.clone() };
        if mode == "adb" || mode == "recovery" || mode == "sideload" {
            self.set_status("Getting ADB properties...");
            self.start_process("detect_adb_props", "adb".into(), vec!["shell".into(), "getprop".into()]);
        } else if mode == "fastboot" {
            self.set_status("Getting Fastboot variables...");
            self.start_process("detect_fastboot_vars", "fastboot".into(), vec!["getvar".into(), "all".into()]);
        }
    }
}
