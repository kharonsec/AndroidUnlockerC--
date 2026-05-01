#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod app;
mod config;
mod executor;
mod flash;
mod network;
mod platform;
mod protocols;
mod safety;
mod state;
mod ui;

use app::{Action, AndroidUnlockerApp};
use eframe::egui::{self, Color32, RichText};
use executor::ProcessOutput;
use rfd::FileDialog;

impl eframe::App for AndroidUnlockerApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        ctx.set_visuals(egui::Visuals::light());
        self.check_device_state();

        while let Ok(event) = self.rx.try_recv() {
            let mut st = self.state.try_lock().unwrap();
            match event {
                ProcessOutput::Stdout(line) => {
                    st.logs.push((format!("[STDOUT] {}", line), Color32::from_rgb(100, 100, 100)));
                    if st.current_command_id == "detect_adb_props" {
                        if line.contains("ro.product.manufacturer") {
                            if let Some(val) = line.split("]: [").nth(1) {
                                st.device_manufacturer = val.trim_end_matches(']').to_string();
                            }
                        }
                        if line.contains("ro.product.model") {
                            if let Some(val) = line.split("]: [").nth(1) {
                                st.device_model = val.trim_end_matches(']').to_string();
                            }
                        }
                    }
                }
                ProcessOutput::Stderr(line) => {
                    if st.current_command_id == "detect_fastboot_vars" {
                        if line.starts_with("(bootloader) product:") {
                            if let Some(val) = line.split(':').nth(1) {
                                st.device_model = format!("{} (codename)", val.trim());
                            }
                        }
                    }
                    st.logs.push((format!("[STDERR] {}", line), Color32::from_rgb(200, 50, 50)));
                }
                ProcessOutput::Error(e) => {
                    st.logs.push((format!("[ERROR] {}", e), Color32::RED));
                    st.status_bar = "Error occurred.".into();
                }
                ProcessOutput::Finished(success, id) => {
                    st.is_command_running = false;
                    st.current_command_id.clear();
                    st.cancel_tx = None;
                    if success {
                        st.logs.push(("[SUCCESS] Command completed successfully.".into(), Color32::from_rgb(50, 150, 50)));
                        st.status_bar = format!("Command '{}' successful.", id);
                    } else {
                        st.logs.push(("[ERROR] Command failed.".into(), Color32::DARK_RED));
                        st.status_bar = format!("Command '{}' failed.", id);
                    }
                }
            }
        }

        let mut action = Option::<Action>::None;

        {
            let mut st = self.state.try_lock().unwrap();
            if st.is_amd && !st.has_quirk && st.device_manufacturer.to_lowercase().contains("xiaomi") && !st.quirk_prompt_dismissed {
                st.show_quirk_prompt = true;
            }

            let is_running = st.is_command_running;
            let mode = st.device_mode.clone();

            egui::TopBottomPanel::top("header").show(ctx, |ui| {
                ui.add_space(8.0);
                ui.horizontal(|ui| {
                    ui.vertical(|ui| {
                        ui.label(RichText::new("Android Bootloader & Recovery Tool").size(20.0).strong());
                        ui.label(RichText::new("Connected Device Details").color(Color32::GRAY));
                    });
                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                        let mode_color = match mode.as_str() {
                            "adb" => Color32::from_rgb(50, 150, 50),
                            "fastboot" => Color32::from_rgb(200, 100, 0),
                            "none" => Color32::GRAY,
                            _ => Color32::from_rgb(0, 100, 200),
                        };
                        ui.label(RichText::new(mode.to_uppercase()).strong().size(24.0).color(mode_color));
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
                            if ui.button("Cancel").clicked() {
                                if let Some(tx) = st.cancel_tx.take() {
                                    let _ = tx.try_send(());
                                }
                            }
                        });
                    }
                });
            });

            egui::TopBottomPanel::top("tabs").resizable(false).show(ctx, |ui| {
                ui.horizontal(|ui| {
                    for (i, tab) in ui::TABS.iter().enumerate() {
                        if ui.selectable_label(st.selected_tab == i, tab.name).clicked() {
                            st.selected_tab = i;
                        }
                    }
                });
            });

            egui::CentralPanel::default().show(ctx, |ui| {
                ui::render_tab_content(ui, st.selected_tab, &st, &mut action, is_running);
            });

            if st.show_quirk_prompt {
                egui::Window::new("AMD Xiaomi Fix Required")
                    .collapsible(false).resizable(false)
                    .anchor(egui::Align2::CENTER_CENTER, egui::vec2(0.0, 0.0))
                    .show(ctx, |ui| {
                        ui.label("Xiaomi devices often disconnect in fastboot on AMD. Apply USB quirk fix?");
                        ui.horizontal(|ui| {
                            if ui.button("Apply Fix").clicked() {
                                action = Some(Action::ApplyQuirk);
                                st.show_quirk_prompt = false;
                            }
                            if ui.button("Dismiss").clicked() {
                                st.show_quirk_prompt = false;
                                st.quirk_prompt_dismissed = true;
                            }
                        });
                    });
            }
        }

        if let Some(ref a) = action {
            if *a == Action::None { action = None; }
        }

        if let Some(a) = action {
            let config = {
                let st = self.state.try_lock().unwrap();
                self.config.get_config(&st.device_manufacturer, &st.device_model)
            };
            let mode = { self.state.try_lock().unwrap().device_mode.clone() };
            match a {
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
                    if let Some(path) = FileDialog::new().add_filter("image", &["img"]).pick_file() {
                        let partition = config.recovery_partition.unwrap_or_else(|| "recovery".into());
                        let args: Vec<String> = vec!["flash".into(), partition, path.to_string_lossy().into_owned()];
                        self.start_process("flash_recovery", "fastboot".into(), args);
                    }
                }
                Action::FlashBoot => {
                    if let Some(path) = FileDialog::new().add_filter("image", &["img"]).pick_file() {
                        let partition = config.boot_partition.unwrap_or_else(|| "boot".into());
                        let args: Vec<String> = vec!["flash".into(), partition, path.to_string_lossy().into_owned()];
                        self.start_process("flash_boot", "fastboot".into(), args);
                    }
                }
                Action::RebootSystem => {
                    let cmd = if mode == "fastboot" {
                        config.fastboot_reboot_command.unwrap_or_else(|| "fastboot reboot".into())
                    } else {
                        config.adb_reboot_system_command.unwrap_or_else(|| "adb reboot".into())
                    };
                    self.run_cmd("reboot_system", &cmd, None);
                }
                Action::RebootRecovery => {
                    let cmd = config.adb_reboot_recovery_command.unwrap_or_else(|| "adb reboot recovery".into());
                    self.run_cmd("reboot_recovery", &cmd, None);
                }
                Action::RebootBootloader => {
                    let cmd = config.adb_reboot_bootloader_command.unwrap_or_else(|| "adb reboot bootloader".into());
                    self.run_cmd("reboot_bootloader", &cmd, None);
                }
                Action::Sideload => {
                    if let Some(path) = FileDialog::new().add_filter("zip", &["zip"]).pick_file() {
                        let args: Vec<String> = vec!["sideload".into(), path.to_string_lossy().into_owned()];
                        self.start_process("sideload", "adb".into(), args);
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
            .with_inner_size([1000.0, 750.0])
            .with_title("Enhanced Android Bootloader & Recovery Tool (Rust)"),
        ..Default::default()
    };
    eframe::run_native("Android Unlocker Rust", options, Box::new(|cc| Box::new(AndroidUnlockerApp::new(cc))))
}
