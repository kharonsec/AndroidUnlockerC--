use crate::executor::ProcessOutput;
use crossbeam_channel::Sender;

#[derive(Default)]
pub struct DmesgState {
    pub capturing: bool,
}

impl DmesgState {
    pub fn capture(&mut self, handle: &tokio::runtime::Handle, tx: Sender<ProcessOutput>) {
        if self.capturing {
            return;
        }
        self.capturing = true;

        let dmesg_tx = tx.clone();
        handle.spawn(async move {
            let child = tokio::process::Command::new("adb")
                .args(["shell", "dmesg"])
                .stdout(std::process::Stdio::piped())
                .stderr(std::process::Stdio::piped())
                .spawn();

            match child {
                Ok(mut c) => {
                    let stdout = c.stdout.take().unwrap();
                    let tx_out = dmesg_tx.clone();
                    let out_task = tokio::spawn(async move {
                        use tokio::io::{AsyncBufReadExt, BufReader};
                        let mut reader = BufReader::new(stdout).lines();
                        while let Ok(Some(line)) = reader.next_line().await {
                            let _ = tx_out.send(ProcessOutput::Stdout(line));
                        }
                    });

                    let _ = c.wait().await;
                    let _ = out_task.await;
                }
                Err(e) => {
                    let _ = dmesg_tx.send(ProcessOutput::Error(format!("dmesg: {}", e)));
                }
            }
        });
    }
}
