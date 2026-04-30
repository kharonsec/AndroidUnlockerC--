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
