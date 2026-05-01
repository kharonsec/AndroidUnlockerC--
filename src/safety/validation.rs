use crate::config::DeviceConfig;

pub struct ValidationResult {
    pub compatible: bool,
    pub warnings: Vec<String>,
}

pub fn validate_firmware(
    _config: &DeviceConfig,
    _firmware_model: &str,
    _firmware_version: &str,
) -> ValidationResult {
    ValidationResult {
        compatible: true,
        warnings: vec![
            "Always verify firmware is for your exact model before flashing.".into(),
            "Flashing incompatible firmware may permanently brick your device.".into(),
        ],
    }
}
