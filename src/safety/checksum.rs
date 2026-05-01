use sha2::{Digest, Sha256};
use std::path::Path;

pub fn checksum_file(path: &Path) -> Result<String, String> {
    let data = std::fs::read(path).map_err(|e| format!("Read error: {}", e))?;
    let mut h = Sha256::new();
    h.update(&data);
    Ok(format!("{:x}", h.finalize()))
}

pub fn checksum_partition(partition: &str) -> Result<String, String> {
    let path = format!("/dev/block/by-name/{}", partition);
    checksum_file(Path::new(&path))
}
