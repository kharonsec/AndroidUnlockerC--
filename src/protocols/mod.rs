pub mod adb_fastboot;
pub mod brom;
pub mod edl;
pub mod mtp;
pub mod samsung;

#[derive(Debug, Clone, PartialEq)]
pub enum ProtocolId {
    Adb,
    Fastboot,
    Fastbootd,
    Edl,
    Brom,
    SamsungDL,
    MtkSpFlash,
    XiaomiMiFlash,
    HuaweiERecovery,
    Mtp,
}

#[derive(Debug, Clone)]
pub struct DeviceInfo {
    pub serial: String,
    pub manufacturer: String,
    pub model: String,
    pub firmware_version: Option<String>,
    pub slot: Option<String>,
    pub is_unlocked: Option<bool>,
}

pub trait ProtocolHandler: Send + Sync {
    fn id() -> ProtocolId;
    fn name() -> &'static str;
    fn priority() -> u8;
    fn vid_pids() -> &'static [(u16, u16)];
    fn detect(stdout: &str) -> Option<DeviceInfo>;
    fn format_command(id: ProtocolId, args: &[String]) -> (String, Vec<String>);
    fn available_actions() -> Vec<&'static str>;
}

pub struct ProtocolEngine;

impl ProtocolEngine {
    fn parse_adb_devices(stdout: &str) -> Vec<(String, String)> {
        stdout
            .lines()
            .skip(1)
            .filter_map(|l| {
                let p: Vec<&str> = l.split_whitespace().collect();
                if p.len() == 2 {
                    Some((p[0].into(), p[1].into()))
                } else {
                    None
                }
            })
            .collect()
    }

    fn parse_fastboot_devices(stdout: &str) -> Vec<(String, String)> {
        stdout
            .lines()
            .filter_map(|l| {
                let p: Vec<&str> = l.split_whitespace().collect();
                if p.len() >= 2 {
                    Some((p[0].into(), p[1].into()))
                } else {
                    None
                }
            })
            .collect()
    }

    pub async fn detect_all() -> (String, String) {
        let mut mode = "none".to_string();
        let mut serial = "N/A".to_string();

        if let Ok((ok, stdout, _)) =
            crate::executor::Executor::run_command_sync("adb", &["devices"]).await
        {
            if ok {
                for (s, status) in Self::parse_adb_devices(&stdout) {
                    match status.as_str() {
                        "device" => {
                            mode = "adb".into();
                            serial = s;
                            break;
                        }
                        "recovery" => {
                            mode = "recovery".into();
                            serial = s;
                            break;
                        }
                        "sideload" => {
                            mode = "sideload".into();
                            serial = s;
                            break;
                        }
                        _ => {}
                    }
                }
            }
        }

        if mode == "none" {
            if let Ok((ok, stdout, _)) =
                crate::executor::Executor::run_command_sync("fastboot", &["devices"]).await
            {
                if ok {
                    for (s, status) in Self::parse_fastboot_devices(&stdout) {
                        if status == "fastboot" {
                            mode = "fastboot".into();
                            serial = s;
                            break;
                        }
                    }
                }
            }
        }

        (mode, serial)
    }
}
