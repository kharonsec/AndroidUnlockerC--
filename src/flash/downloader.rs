use reqwest::Client;
use std::path::PathBuf;

pub async fn download_file(url: &str, dest: PathBuf, on_progress: impl Fn(u64, u64)) -> Result<PathBuf, String> {
    let client = Client::new();
    let response = client.get(url).send().await.map_err(|e| format!("Request: {}", e))?;
    let total = response.content_length().unwrap_or(0);

    let mut downloaded = 0u64;
    let mut buf = Vec::new();
    let mut stream = response.bytes_stream();

    use futures_util::StreamExt;
    while let Some(chunk) = stream.next().await {
        let chunk = chunk.map_err(|e| format!("Chunk: {}", e))?;
        buf.extend_from_slice(&chunk);
        downloaded += chunk.len() as u64;
        on_progress(downloaded, total);
    }

    std::fs::write(&dest, &buf).map_err(|e| format!("Write: {}", e))?;
    Ok(dest)
}

pub fn twrp_download_url(device_codename: &str) -> String {
    format!("https://dl.twrp.me/{}/twrp-{}-latest.img", device_codename, device_codename)
}
