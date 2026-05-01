use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Debug, Deserialize, Clone)]
pub struct DeviceConfig {
    pub unlock_command: Option<String>,
    pub lock_command: Option<String>,
    pub adb_reboot_bootloader_command: Option<String>,
    pub adb_reboot_recovery_command: Option<String>,
    pub adb_reboot_system_command: Option<String>,
    pub fastboot_reboot_command: Option<String>,
    pub recovery_partition: Option<String>,
    pub boot_partition: Option<String>,
    pub unlock_warning: Option<String>,
    pub lock_warning: Option<String>,
    #[allow(dead_code)]
    pub notes: Option<String>,
}

impl Default for DeviceConfig {
    fn default() -> Self {
        Self {
            unlock_command: Some("fastboot oem unlock".into()),
            lock_command: Some("fastboot oem lock".into()),
            adb_reboot_bootloader_command: Some("adb reboot bootloader".into()),
            adb_reboot_recovery_command: Some("adb reboot recovery".into()),
            adb_reboot_system_command: Some("adb reboot".into()),
            fastboot_reboot_command: Some("fastboot reboot".into()),
            recovery_partition: Some("recovery".into()),
            boot_partition: Some("boot".into()),
            unlock_warning: Some("Unlocking the bootloader will erase all data (factory reset) and may void your warranty! Proceed with caution.".into()),
            lock_warning: Some("Locking the bootloader will erase all data. Ensure you have backups before proceeding.".into()),
            notes: Some("Built-in default configuration.".into()),
        }
    }
}

#[derive(Debug, Deserialize)]
pub struct FullConfig {
    pub default: Option<DeviceConfig>,
    #[serde(flatten)]
    pub devices: HashMap<String, DeviceConfig>,
}

impl FullConfig {
    pub fn load(path: &str) -> Self {
        let default_config = Self {
            default: Some(DeviceConfig::default()),
            devices: HashMap::new(),
        };

        let file_content = std::fs::read_to_string(path).unwrap_or_default();
        if file_content.is_empty() {
            return default_config;
        }

        match serde_yaml::from_str::<FullConfig>(&file_content) {
            Ok(mut config) => {
                if config.default.is_none() {
                    config.default = Some(DeviceConfig::default());
                }
                config
            }
            Err(e) => {
                eprintln!("Failed to parse config: {}", e);
                default_config
            }
        }
    }

    pub fn get_config(&self, manufacturer: &str, model: &str) -> DeviceConfig {
        let exact_key = format!("{} {}", manufacturer.to_lowercase(), model.to_lowercase());
        if let Some(config) = self.devices.get(&exact_key) {
            return config.clone();
        }

        let mfg_key = manufacturer.to_lowercase();
        if let Some(config) = self.devices.get(&mfg_key) {
            return config.clone();
        }

        self.default.clone().unwrap_or_default()
    }
}

#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct FlashProfile {
    pub name: String,
    pub device: Option<String>,
    pub operations: Vec<ProfileOp>,
}

#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct ProfileOp {
    #[serde(rename = "type")]
    pub op_type: String,
    pub target: String,
    pub file: Option<String>,
}

pub fn save_profile(profile: &FlashProfile, path: &str) -> Result<(), String> {
    let yaml = serde_yaml::to_string(profile).map_err(|e| format!("Serialize: {}", e))?;
    std::fs::write(path, yaml).map_err(|e| format!("Write: {}", e))
}

pub fn load_profile(path: &str) -> Result<FlashProfile, String> {
    let content = std::fs::read_to_string(path).map_err(|e| format!("Read: {}", e))?;
    serde_yaml::from_str(&content).map_err(|e| format!("Parse: {}", e))
}
