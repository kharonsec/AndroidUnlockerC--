use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::fs;
use std::path::PathBuf;

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct AuditEntry {
    pub timestamp: String,
    pub action: String,
    pub serial: String,
    pub prev_hash: String,
}

impl AuditEntry {
    pub fn new(action: &str, serial: &str, prev_hash: &str) -> Self {
        Self {
            timestamp: chrono_now(),
            action: action.to_string(),
            serial: serial.to_string(),
            prev_hash: prev_hash.to_string(),
        }
    }

    pub fn hash(&self) -> String {
        let mut h = Sha256::new();
        h.update(
            format!(
                "{}:{}:{}:{}",
                self.timestamp, self.action, self.serial, self.prev_hash
            )
            .as_bytes(),
        );
        format!("{:x}", h.finalize())
    }
}

pub fn log_action(action: &str, serial: &str) {
    let log_path = audit_path();
    let prev_hash = last_hash(&log_path);
    let entry = AuditEntry::new(action, serial, &prev_hash);
    let mut entries = read_log(&log_path);
    entries.push(entry);
    write_log(&log_path, &entries);
}

pub fn read_action_log() -> Vec<AuditEntry> {
    read_log(&audit_path())
}

fn audit_path() -> PathBuf {
    let home = std::env::var("HOME").unwrap_or_else(|_| ".".into());
    let dir = PathBuf::from(home).join(".androidtool");
    fs::create_dir_all(&dir).ok();
    dir.join("action_log.json")
}

fn last_hash(path: &PathBuf) -> String {
    let entries = read_log(path);
    entries
        .last()
        .map(|e| e.hash())
        .unwrap_or_else(|| "genesis".into())
}

fn read_log(path: &PathBuf) -> Vec<AuditEntry> {
    let content = fs::read_to_string(path).unwrap_or_default();
    if content.trim().is_empty() {
        return vec![];
    }
    serde_json::from_str(&content).unwrap_or_default()
}

fn write_log(path: &PathBuf, entries: &[AuditEntry]) {
    if let Ok(json) = serde_json::to_string_pretty(entries) {
        fs::write(path, json).ok();
    }
}

fn chrono_now() -> String {
    use std::time::{SystemTime, UNIX_EPOCH};
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs().to_string())
        .unwrap_or_default()
}
