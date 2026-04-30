use crossbeam_channel::Sender;
use std::process::Stdio;
use tokio::io::{AsyncBufReadExt, BufReader};
use tokio::process::Command;

#[derive(Debug, Clone)]
pub enum ProcessOutput {
    Stdout(String),
    Stderr(String),
    Finished(bool, String), // success, command id
    Error(String),
}

pub struct Executor {
    tx: Sender<ProcessOutput>,
    handle: tokio::runtime::Handle,
}

impl Executor {
    pub fn new(tx: Sender<ProcessOutput>, handle: tokio::runtime::Handle) -> Self {
        Self { tx, handle }
    }

    pub fn new_tx(&self) -> Sender<ProcessOutput> {
        self.tx.clone()
    }

    pub fn run_command(
        &self,
        command_id: String,
        program: String,
        args: Vec<String>,
        mut cancel_rx: tokio::sync::mpsc::Receiver<()>,
    ) {
        let tx = self.tx.clone();

        self.handle.spawn(async move {
            let mut cmd = Command::new(&program);

            cmd.args(&args)
                .stdout(Stdio::piped())
                .stderr(Stdio::piped());

            let mut child = match cmd.spawn() {
                Ok(c) => c,
                Err(e) => {
                    let _ = tx.send(ProcessOutput::Error(format!(
                        "Failed to start '{}': {}",
                        program, e
                    )));
                    let _ = tx.send(ProcessOutput::Finished(false, command_id));
                    return;
                }
            };

            let stdout = child.stdout.take().unwrap();
            let stderr = child.stderr.take().unwrap();

            let tx_stdout = tx.clone();
            let mut reader_stdout = BufReader::new(stdout).lines();
            let out_task = tokio::spawn(async move {
                while let Ok(Some(line)) = reader_stdout.next_line().await {
                    let _ = tx_stdout.send(ProcessOutput::Stdout(line));
                }
            });

            let tx_stderr = tx.clone();
            let mut reader_stderr = BufReader::new(stderr).lines();
            let err_task = tokio::spawn(async move {
                while let Ok(Some(line)) = reader_stderr.next_line().await {
                    let _ = tx_stderr.send(ProcessOutput::Stderr(line));
                }
            });

            tokio::select! {
                status = child.wait() => {
                    let _ = out_task.await;
                    let _ = err_task.await;
                    match status {
                        Ok(exit_status) => {
                            let _ = tx.send(ProcessOutput::Finished(exit_status.success(), command_id));
                        }
                        Err(e) => {
                            let _ = tx.send(ProcessOutput::Error(format!("Wait error: {}", e)));
                            let _ = tx.send(ProcessOutput::Finished(false, command_id));
                        }
                    }
                }
                _ = cancel_rx.recv() => {
                    let _ = child.kill().await;
                    let _ = tx.send(ProcessOutput::Error(format!("Command '{}' cancelled", command_id)));
                    let _ = tx.send(ProcessOutput::Finished(false, command_id));
                }
            }
        });
    }

    pub async fn run_command_sync(
        program: &str,
        args: &[&str],
    ) -> Result<(bool, String, String), String> {
        let mut cmd = Command::new(program);
        cmd.args(args);

        match cmd.output().await {
            Ok(output) => {
                let stdout = String::from_utf8_lossy(&output.stdout).to_string();
                let stderr = String::from_utf8_lossy(&output.stderr).to_string();
                Ok((output.status.success(), stdout, stderr))
            }
            Err(e) => Err(format!("Failed to execute '{}': {}", program, e)),
        }
    }
}
