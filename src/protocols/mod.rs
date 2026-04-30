pub mod adb_fastboot;
pub mod brom;
pub mod edl;
pub mod mtp;
pub mod samsung;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ProtocolId {
    Adb,
    Fastboot,
    Edl,
    Brom,
    Samsung,
    Mtp,
}

#[derive(Debug, Clone)]
pub struct DeviceInfo {
    pub manufacturer: String,
    pub model: String,
    pub android_version: String,
    pub chipset: String,
}
