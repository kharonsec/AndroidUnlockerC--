use crate::executor::ProcessOutput;
use crossbeam_channel::Sender;
use tokio::sync::mpsc;

pub struct LogcatState {
    pub running: bool,
    pub filter: String,
    pub autoscroll: bool,
    cancel_tx: Option<mpsc::Sender<()>>,
}

impl Default for LogcatState {
    fn default() -> Self {
        Self {
            running: false,
            filter: String::new(),
            autoscroll: true,
            cancel_tx: None,
        }
    }
}

impl LogcatState {
    pub fn start(&mut self, handle: &tokio::runtime::Handle, tx: Sender<ProcessOutput>) {
        if self.running {
            return;
        }
        let (cancel_tx, mut cancel_rx) = mpsc::channel(1);
        self.cancel_tx = Some(cancel_tx);
        self.running = true;

        let log_tx = tx.clone();
        handle.spawn(async move {
            let child = tokio::process::Command::new("adb")
                .args(["logcat"])
                .stdout(std::process::Stdio::piped())
                .stderr(std::process::Stdio::piped())
                .spawn();

            match child {
                Ok(mut c) => {
                    let stdout = c.stdout.take().unwrap();
                    let stderr = c.stderr.take().unwrap();
                    let tx_out = log_tx.clone();
                    let tx_err = log_tx.clone();

                    let out_task = tokio::spawn(async move {
                        use tokio::io::{AsyncBufReadExt, BufReader};
                        let mut reader = BufReader::new(stdout).lines();
                        while let Ok(Some(line)) = reader.next_line().await {
                            let _ = tx_out.send(ProcessOutput::Stdout(line));
                        }
                    });
                    let err_task = tokio::spawn(async move {
                        use tokio::io::{AsyncBufReadExt, BufReader};
                        let mut reader = BufReader::new(stderr).lines();
                        while let Ok(Some(line)) = reader.next_line().await {
                            let _ = tx_err.send(ProcessOutput::Stderr(line));
                        }
                    });

                    tokio::select! {
                        _ = c.wait() => { let _ = out_task.await; let _ = err_task.await; }
                        _ = cancel_rx.recv() => { let _ = c.kill().await; }
                    }
                }
                Err(e) => {
                    let _ = log_tx.send(ProcessOutput::Error(format!("logcat: {}", e)));
                }
            }
        });
    }

    pub fn stop(&mut self) {
        self.cancel_tx = None;
        self.running = false;
    }
}
