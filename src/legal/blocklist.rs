use serde::Deserialize;

#[derive(Debug, Deserialize, Clone)]
pub struct BlocklistEntry {
    pub pattern: String,
    pub warning: String,
}

#[derive(Debug, Deserialize)]
struct BlocklistFile {
    pub malware: Vec<BlocklistEntry>,
}

pub fn load_blocklist(path: &str) -> Vec<BlocklistEntry> {
    let content = std::fs::read_to_string(path).unwrap_or_default();
    serde_yaml::from_str::<BlocklistFile>(&content)
        .map(|f| f.malware)
        .unwrap_or_default()
}

pub fn check_filename(filename: &str, blocklist: &[BlocklistEntry]) -> Option<String> {
    let lower = filename.to_lowercase();
    for entry in blocklist {
        let pattern = entry.pattern.trim_matches('*').to_lowercase();
        if pattern.is_empty() {
            continue;
        }
        if lower.contains(&pattern) {
            return Some(entry.warning.clone());
        }
    }
    None
}
