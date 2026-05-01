use crate::executor::Executor;
use std::fs;
use std::path::PathBuf;

pub fn backup_dir() -> PathBuf {
    let home = dirs_next().unwrap_or_else(|| PathBuf::from("."));
    home.join(".androidtool").join("backups")
}

pub fn ensure_backup_dir() -> PathBuf {
    let dir = backup_dir();
    fs::create_dir_all(&dir).ok();
    dir
}

pub async fn backup_partitions(serial: &str, partitions: &[&str]) -> Result<PathBuf, String> {
    let dir = ensure_backup_dir().join(serial).join(chrono_timestamp());
    fs::create_dir_all(&dir).map_err(|e| format!("mkdir: {}", e))?;

    for part in partitions {
        let (ok, _, stderr) = Executor::run_command_sync(
            "adb",
            &[
                "shell",
                "dd",
                &format!("if=/dev/block/by-name/{}", part),
                &format!("of=/sdcard/{}.img", part),
            ],
        )
        .await
        .map_err(|e| e)?;
        if !ok {
            return Err(format!("dd failed for {}: {}", part, stderr));
        }
        let (ok, _, stderr) = Executor::run_command_sync(
            "adb",
            &[
                "pull",
                &format!("/sdcard/{}.img", part),
                &dir.join(format!("{}.img", part)).to_string_lossy(),
            ],
        )
        .await
        .map_err(|e| e)?;
        if !ok {
            return Err(format!("pull failed for {}: {}", part, stderr));
        }
        Executor::run_command_sync("adb", &["shell", "rm", &format!("/sdcard/{}.img", part)])
            .await
            .ok();
    }
    Ok(dir)
}

fn chrono_timestamp() -> String {
    use std::time::{SystemTime, UNIX_EPOCH};
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs().to_string())
        .unwrap_or_default()
}

fn dirs_next() -> Option<PathBuf> {
    std::env::var("HOME").ok().map(PathBuf::from)
}
