use std::path::Path;

pub fn patch_boot_image(boot_path: &Path) -> Result<std::path::PathBuf, String> {
    let magiskboot = which_magiskboot()?;
    let output = std::process::Command::new(&magiskboot)
        .args(["patch", &boot_path.to_string_lossy()])
        .output()
        .map_err(|e| format!("magiskboot: {}", e))?;

    if !output.status.success() {
        return Err(String::from_utf8_lossy(&output.stderr).to_string());
    }

    let parent = boot_path.parent().unwrap_or(Path::new("."));
    Ok(parent.join("new-boot.img"))
}

fn which_magiskboot() -> Result<String, String> {
    for candidate in &["magiskboot", "magiskinit"] {
        if std::process::Command::new("which")
            .arg(candidate)
            .output()
            .map(|o| o.status.success())
            .unwrap_or(false)
        {
            return Ok(candidate.to_string());
        }
    }
    for path in &["/usr/local/bin/magiskboot", "/data/local/tmp/magiskboot"] {
        if std::path::Path::new(path).exists() {
            return Ok(path.to_string());
        }
    }
    Err("magiskboot not found. Install Magisk tools.".into())
}
