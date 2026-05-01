use crate::executor::Executor;

pub async fn connect(ip: &str, port: u16) -> Result<String, String> {
    let addr = format!("{}:{}", ip, port);
    let (ok, stdout, stderr) = Executor::run_command_sync("adb", &["connect", &addr])
        .await
        .map_err(|e| format!("Error: {}", e))?;
    if ok {
        Ok(stdout)
    } else {
        Err(stderr)
    }
}

pub async fn disconnect(ip: &str, port: u16) -> Result<String, String> {
    let addr = format!("{}:{}", ip, port);
    let (ok, stdout, stderr) = Executor::run_command_sync("adb", &["disconnect", &addr])
        .await
        .map_err(|e| format!("Error: {}", e))?;
    if ok {
        Ok(stdout)
    } else {
        Err(stderr)
    }
}

pub fn generate_dockerfile() -> &'static str {
    r#"FROM ubuntu:22.04
RUN apt update && apt install -y android-sdk-platform-tools
COPY ./androidtool /usr/local/bin/androidtool
ENTRYPOINT ["/usr/local/bin/androidtool"]"#
}
