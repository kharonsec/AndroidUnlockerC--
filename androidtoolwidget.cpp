#include "androidtoolwidget.h"

#include <QTextStream>
#include <QColor>
#include <QDebug>
#include <QStringList> // For command arguments
#include <QFileDialog> // For file selection (later)
#include <QMessageBox> // For warnings/confirmations (later)


AndroidToolWidget::AndroidToolWidget(QWidget *parent)
    : QWidget(parent),
      mainLayout(new QVBoxLayout(this)),
      infoLayout(new QHBoxLayout()),
      buttonLayoutMain(new QHBoxLayout()), // Initialize new layouts
      buttonLayoutReboot(new QHBoxLayout()),
      buttonLayoutFile(new QHBoxLayout()),
      manufacturerLabel(new QLabel("Manufacturer: Unknown")),
      modelLabel(new QLabel("Model: Unknown")),
      serialLabel(new QLabel("Serial: N/A")), // Initialize serial label
      modeLabel(new QLabel("Mode: Disconnected")),
      outputArea(new QTextEdit()),
      // Initialize all buttons
      detectButton(new QPushButton("Detect Device Details")),
      unlockButton(new QPushButton("Unlock Bootloader")),
      lockButton(new QPushButton("Lock Bootloader")),
      recoveryButton(new QPushButton("Flash Recovery")),
      magiskButton(new QPushButton("Flash Patched Boot")),
      rebootSystemButton(new QPushButton("Reboot System")),
      rebootRecoveryButton(new QPushButton("Reboot Recovery")),
      rebootBootloaderButton(new QPushButton("Reboot Bootloader")),
      sideloadButton(new QPushButton("ADB Sideload ZIP")),
      adbPushButton(new QPushButton("ADB Push File")),
      adbPullButton(new QPushButton("ADB Pull File")),
      mainProcess(new QProcess(this)), // Initialize main process
      stateCheckProcess(new QProcess(this)), // Initialize state check process
      stateTimer(new QTimer(this)), // Initialize timer
      deviceMode("none"), // Initialize state variables
      deviceSerialAdb("N/A"),
      deviceSerialFastboot("N/A")

{
    // --- UI Setup ---
    outputArea->setReadOnly(true);
    outputArea->setPlaceholderText("Command output and logs will appear here...");
    // Basic styling for dark theme readability (can be improved with QPalette)
    // outputArea->setStyleSheet("QTextEdit { color: #ABB2BF; }");


    // Info layout
    QFont boldFont;
    boldFont.setBold(true);
    modeLabel->setFont(boldFont);

    infoLayout->addWidget(manufacturerLabel);
    infoLayout->addWidget(modelLabel);
    infoLayout->addWidget(serialLabel); // Add serial label to layout
    infoLayout->addStretch(1); // Push labels to the left
    infoLayout->addWidget(modeLabel);

    // Button layouts (adding all buttons)
    buttonLayoutMain->addWidget(detectButton);
    buttonLayoutMain->addWidget(unlockButton);
    buttonLayoutMain->addWidget(lockButton);
    buttonLayoutMain->addWidget(recoveryButton);
    buttonLayoutMain->addWidget(magiskButton);

    buttonLayoutReboot->addWidget(rebootSystemButton);
    buttonLayoutReboot->addWidget(rebootRecoveryButton);
    buttonLayoutReboot->addWidget(rebootBootloaderButton);

    buttonLayoutFile->addWidget(sideloadButton);
    buttonLayoutFile->addWidget(adbPushButton);
    buttonLayoutFile->addWidget(adbPullButton);


    // Main layout
    mainLayout->addLayout(infoLayout);
    mainLayout->addWidget(outputArea);
    mainLayout->addLayout(buttonLayoutMain); // Add new layouts
    mainLayout->addLayout(buttonLayoutReboot);
    mainLayout->addLayout(buttonLayoutFile);
    mainLayout->addStretch(1); // Push everything towards the top


    // Set the size of the window (optional)
    setGeometry(100, 100, 900, 700); // Slightly larger window

    // --- Connections ---
    // Connect button signals to slots
    connect(detectButton, &QPushButton::clicked, this, &AndroidToolWidget::on_detectButton_clicked);
    connect(unlockButton, &QPushButton::clicked, this, &AndroidToolWidget::on_unlockButton_clicked);
    connect(lockButton, &QPushButton::clicked, this, &AndroidToolWidget::on_lockButton_clicked);
    connect(recoveryButton, &QPushButton::clicked, this, &AndroidToolWidget::on_recoveryButton_clicked);
    connect(magiskButton, &QPushButton::clicked, this, &AndroidToolWidget::on_magiskButton_clicked);
    connect(rebootSystemButton, &QPushButton::clicked, this, &AndroidToolWidget::on_rebootSystemButton_clicked);
    connect(rebootRecoveryButton, &QPushButton::clicked, this, &AndroidToolWidget::on_rebootRecoveryButton_clicked);
    connect(rebootBootloaderButton, &QPushButton::clicked, this, &AndroidToolWidget::on_rebootBootloaderButton_clicked); // Connect implemented slot
    connect(sideloadButton, &QPushButton::clicked, this, &AndroidToolWidget::on_sideloadButton_clicked);
    connect(adbPushButton, &QPushButton::clicked, this, &AndroidToolWidget::on_adbPushButton_clicked);
    connect(adbPullButton, &QPushButton::clicked, this, &AndroidToolWidget::on_adbPullButton_clicked);


    // Connect Main QProcess signals to slots
    connect(mainProcess, &QProcess::readyReadStandardOutput, this, &AndroidToolWidget::processReadyReadStandardOutput);
    connect(mainProcess, &QProcess::readyReadStandardError, this, &AndroidToolWidget::processReadyReadStandardError);
    // Use static_cast for the overloaded finished signal
    connect(mainProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &AndroidToolWidget::processFinished);
    connect(mainProcess, &QProcess::errorOccurred, this, &AndroidToolWidget::processErrorOccurred);

    // Connect State Check QProcess signals to slots
    // We connect finished, but may not need output signals if just parsing final output
    connect(stateCheckProcess, &QProcess::readyReadStandardOutput, this, &AndroidToolWidget::stateCheckProcessReadyReadStandardOutput); // Optional
    connect(stateCheckProcess, &QProcess::readyReadStandardError, this, &AndroidToolWidget::stateCheckProcessReadyReadStandardError); // Optional
    connect(stateCheckProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &AndroidToolWidget::stateCheckProcessFinished); // Connect implemented slot
    connect(stateCheckProcess, &QProcess::errorOccurred, this, &AndroidToolWidget::stateCheckProcessErrorOccurred); // Optional, for debugging state checks


    // Connect QTimer signal to slot
    connect(stateTimer, &QTimer::timeout, this, &AndroidToolWidget::checkDeviceState);

    // --- Initial State ---
    // Start the timer to check device state periodically
    stateTimer->start(3000); // Check every 3 seconds

    // Perform an initial state check immediately
    checkDeviceState();

    // Initial button states are set by the checkDeviceState -> updateButtonStates call
}

AndroidToolWidget::~AndroidToolWidget()
{
    // Qt's object tree handles deletion of children.
    // currentProcess, stateCheckProcess, stateTimer, layouts, labels, buttons, outputArea
    // are children of 'this' or other objects in the hierarchy, and will be deleted automatically.
}

// --- Helper Functions Implementation ---

void AndroidToolWidget::appendOutput(const QString &text, const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);

    QTextCursor cursor = outputArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text + "\n", format);

    // Auto-scroll to the bottom
    outputArea->verticalScrollBar()->setValue(outputArea->verticalScrollBar()->maximum());
}

void AndroidToolWidget::updateDeviceMode(const QString& mode)
{
    if (deviceMode != mode) { // Only update if mode changed
        deviceMode = mode;
        modeLabel->setText("Mode: " + deviceMode.capitalize()); // Capitalize first letter
        appendOutput(QString("[*] Device mode updated to: %1").arg(deviceMode.capitalize()), Qt::gray);
        updateButtonStates(); // Update button states when mode changes
    }
}

void AndroidToolWidget::updateDeviceSerial(const QString& serial, bool isFastboot)
{
    QString* currentSerial = isFastboot ? &deviceSerialFastboot : &deviceSerialAdb;
    if (*currentSerial != serial) {
        *currentSerial = serial;
         // Update the displayed serial based on the current active mode
        if ((isFastboot && deviceMode == "fastboot") || (!isFastboot && (deviceMode == "adb" || deviceMode == "recovery" || deviceMode == "sideload"))) {
             serialLabel->setText("Serial: " + serial);
             appendOutput(QString("[*] Device serial (%1) updated to: %2").arg(isFastboot ? "Fastboot" : "ADB").arg(serial), Qt::gray);
        }
    }
}


void AndroidToolWidget::updateButtonStates()
{
    bool mainProcessRunning = mainProcess->state() != QProcess::NotRunning;
    //bool stateCheckProcessRunning = stateCheckProcess->state() != QProcess::NotRunning;
    // Disable all action buttons if a main process is running
    bool disableActions = mainProcessRunning; // || stateCheckProcessRunning; // Decide if state check also disables actions

    bool is_adb = deviceMode == "adb";
    bool is_fastboot = deviceMode == "fastboot";
    bool is_sideload = deviceMode == "sideload";
    bool is_recovery = deviceMode == "recovery";
    bool is_connected = deviceMode != "none"; // Any connection


    // Main Control Buttons (mostly Fastboot)
    unlockButton->setEnabled(is_fastboot && !disableActions);
    lockButton->setEnabled(is_fastboot && !disableActions);
    recoveryButton->setEnabled(is_fastboot && !disableActions);
    magiskButton->setEnabled(is_fastboot && !disableActions);

    // Reboot Buttons
    // Reboot System works from ADB, Fastboot, and Recovery
    rebootSystemButton->setEnabled((is_adb || is_fastboot || is_recovery) && !disableActions);
    // Reboot Recovery/Bootloader work from ADB and Recovery
    rebootRecoveryButton->setEnabled((is_adb || is_recovery) && !disableActions);
    rebootBootloaderButton->setEnabled((is_adb || is_recovery) && !disableActions);

    // Sideload / File Operations
    // Sideload specifically needs sideload mode active in recovery
    sideloadButton->setEnabled(is_sideload && !disableActions);
    // Push/Pull need normal ADB mode or Recovery mode
    adbPushButton->setEnabled((is_adb || is_recovery) && !disableActions);
    adbPullButton->setEnabled((is_adb || is_recovery) && !disableActions);

    // Detect button is always enabled unless a main process is running
    detectButton->setEnabled(!mainProcessRunning);
    // If stateCheckProcess is running, detect button might ideally be disabled too,
    // or it could just trigger a manual state check (which is what it does now).
    // Let's disable if *either* process is running for simplicity.
    detectButton->setEnabled(mainProcess->state() == QProcess::NotRunning && stateCheckProcess->state() == QProcess::NotRunning);

}

// Helper to start a process asynchronously (can be extended with timeout)
// bool AndroidToolWidget::startProcess(QProcess* process, const QString& program, const QStringList& arguments)
// {
//     if (process->state() != QProcess::NotRunning) {
//          // Process is already running
//          return false;
//     }
//     process->start(program, arguments);
//     return true;
// }


// --- Main QProcess Slots Implementation ---

void AndroidToolWidget::processReadyReadStandardOutput()
{
    QTextStream stream(mainProcess->readAllStandardOutput());
    while (!stream.atEnd()) {
        appendOutput("[STDOUT] " + stream.readLine(), Qt::darkCyan);
    }
}

void AndroidToolWidget::processReadyReadStandardError()
{
    QTextStream stream(mainProcess->readAllStandardError());
     while (!stream.atEnd()) {
        appendOutput("[STDERR] " + stream.readLine(), Qt::red);
    }
}

void AndroidToolWidget::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString status = (exitStatus == QProcess::NormalExit) ? "Normal Exit" : "Crash";
    appendOutput(QString("[*] Command finished with exit code %1 (%2)").arg(exitCode).arg(status), Qt::blue);

    // Any specific post-command logic goes here
    // Example: If an unlock/lock command finished, you might want to reboot or re-check mode.
    // Or if a flash finished, indicate success/failure based on exit code and stderr.

    updateButtonStates(); // Re-enable/update buttons now that main process is finished
    // After a command finishes, the device state might change (e.g., rebooting)
    // The timer will eventually pick this up via checkDeviceState, but a slight delay might be needed.
    // Or manually trigger a state check with a small delay:
    QTimer::singleShot(500, this, &AndroidToolWidget::checkDeviceState); // Check state shortly after command finishes
}

void AndroidToolWidget::processErrorOccurred(QProcess::ProcessError error)
{
    QString errorString;
    switch (error) {
        case QProcess::FailedToStart: errorString = "Failed to Start"; break;
        case QProcess::Crashed: errorString = "Crashed"; break;
        case QProcess::Timedout: errorString = "Timed Out"; break;
        case QProcess::ReadError: errorString = "Read Error"; break;
        case QProcess::WriteError: errorString = "Write Error"; break;
        case QProcess::UnknownError:
        default: errorString = "Unknown Error"; break;
    }
    appendOutput(QString("[ERROR] Main Process Error: %1").arg(errorString), Qt::darkRed);
    appendOutput("[ERROR Details] " + mainProcess->errorString(), Qt::darkRed);

    updateButtonStates(); // Re-enable buttons after error
    QTimer::singleShot(500, this, &AndroidToolWidget::checkDeviceState); // Check state after error
}


// --- State Check QProcess Slots Implementation ---
// These might just be used for debugging or if you decide to log state check output
void AndroidToolWidget::stateCheckProcessReadyReadStandardOutput() { /* Optional: append to log */ }
void AndroidToolWidget::stateCheckProcessReadyReadStandardError() { /* Optional: append errors to log */ }

void AndroidToolWidget::stateCheckProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Check if a main command is *still* running (unlikely if timer was stopped, but safe)
    if (mainProcess->state() != QProcess::NotRunning) {
        qDebug() << "State check finished, but main process is running. Skipping parsing.";
        return; // Skip parsing if a main command started during the check
    }

    QString command = stateCheckProcess->program() + " " + stateCheckProcess->arguments().join(" ");
    qDebug() << "State check command finished:" << command << "Exit Code:" << exitCode; // Debug log

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = stateCheckProcess->readAllStandardOutput();
        if (command.contains("adb devices")) {
             parseAdbDevicesOutput(output);
             // After adb devices check, run fastboot devices check
             runFastbootStateCheck();
        } else if (command.contains("fastboot devices")) {
             parseFastbootDevicesOutput(output);
             // After both checks, update buttons
             updateButtonStates();
        }
    } else {
        // Command failed or crashed - implies no device in this mode or adb/fastboot not found
        if (command.contains("adb devices")) {
            // If adb check fails, explicitly set adb mode to none before fastboot check
            updateDeviceMode("none");
            updateDeviceSerial("N/A", false);
            runFastbootStateCheck(); // Still try fastboot check
        } else if (command.contains("fastboot devices")) {
            // If fastboot check fails AND adb check also failed, set mode to none
            // If adb check succeeded, the mode will still be adb/recovery/sideload
            if (deviceMode == "none") { // Only set to none if adb check found nothing
                 updateDeviceMode("none");
                 updateDeviceSerial("N/A", true);
            }
            updateButtonStates(); // Update buttons
        }
         // Don't append error output to main log unless it's a critical issue with the tool itself
         qDebug() << "State check command failed:" << command << "Stderr:" << stateCheckProcess->readAllStandardError();
    }
}

void AndroidToolWidget::stateCheckProcessErrorOccurred(QProcess::ProcessError error)
{
    // This indicates a problem starting the state check process itself (e.g. adb/fastboot not found)
    // Handle this error gracefully, perhaps by setting mode to "Error" or showing a warning once.
     QString errorString;
    switch (error) {
        case QProcess::FailedToStart: errorString = "Failed to Start (Executable not found)"; break;
        case QProcess::Crashed: errorString = "Crashed"; break;
        case QProcess::Timedout: errorString = "Timed Out"; break;
        case QProcess::ReadError: errorString = "Read Error"; break;
        case QProcess::WriteError: errorString = "Write Error"; break;
        case QProcess::UnknownError:
        default: errorString = "Unknown Error"; break;
    }
    qDebug() << "State Check Process Error: " << errorString << stateCheckProcess->errorString();
    // Prevent spamming errors for missing adb/fastboot
    if (error == QProcess::FailedToStart) {
         if (deviceMode != "error_missing_tools") { // Avoid spamming
              updateDeviceMode("error_missing_tools");
              appendOutput("[CRITICAL] ADB or Fastboot command not found. Ensure platform-tools is in your PATH.", Qt::darkRed);
              QMessageBox::critical(this, "Tool Error", "ADB or Fastboot executable not found.\nPlease ensure the Android SDK Platform-Tools are installed and added to your system's PATH environment variable.");
              // Stop the timer if tools are missing? Or keep checking in case user adds them?
              // Let's keep checking.
         }
    } else {
        // Other state check errors might indicate transient issues or different modes
         updateDeviceMode("check_failed"); // Indicate state check failed
    }
     updateButtonStates(); // Update button states
}

// --- State Check Logic Helpers Implementation ---

void AndroidToolWidget::checkDeviceState()
{
    // Only start a state check if no process (main or state check) is currently running
    if (mainProcess->state() == QProcess::NotRunning && stateCheckProcess->state() == QProcess::NotRunning) {
        runAdbStateCheck(); // Start the ADB check
        // Fastboot check will be initiated after the ADB check finishes in stateCheckProcessFinished
    } else {
        qDebug() << "checkDeviceState skipped: another process is running.";
    }
}

void AndroidToolWidget::runAdbStateCheck()
{
     qDebug() << "Starting ADB state check...";
    // Start the adb devices command using the stateCheckProcess
    stateCheckProcess->start("adb", QStringList() << "devices");
}

void AndroidToolWidget::runFastbootStateCheck()
{
    qDebug() << "Starting Fastboot state check...";
    // Start the fastboot devices command using the stateCheckProcess
    stateCheckProcess->start("fastboot", QStringList() << "devices");
}

void AndroidToolWidget::parseAdbDevicesOutput(const QString& output)
{
    QString newMode = "none";
    QString serial = "N/A";
    // Example parsing for 'adb devices' output:
    // List of devices attached
    // R52D107M77L   device
    // R58D107M77L   recovery
    // R59D107M77L   sideload
    // R60D107M77L   unauthorized

    QStringList lines = output.split('\n', QString::SkipEmptyParts);
    bool deviceFound = false;
    bool recoveryFound = false;
    bool sideloadFound = false;

    if (lines.size() > 1 && lines.first().contains("List of devices attached")) {
        for (int i = 1; i < lines.size(); ++i) {
            QString line = lines.at(i).trimmed();
            if (line.isEmpty() || line.startsWith('*')) continue; // Skip empty lines or "daemon started" etc.

            QStringList parts = line.split('\t', QString::SkipEmptyParts);
            if (parts.size() == 2) {
                serial = parts.at(0).trimmed();
                QString status = parts.at(1).trimmed();

                if (status == "device") {
                    deviceFound = true;
                    break; // Found a normal ADB device, highest priority
                } else if (status == "sideload") {
                    sideloadFound = true; // Found sideload, but keep looking for 'device'
                } else if (status == "recovery") {
                    recoveryFound = true; // Found recovery, but keep looking for 'device' or 'sideload'
                }
                 // Add handling for "unauthorized" state if needed (e.g., show a message)
            }
        }
    }

    // Determine the mode based on priority
    if (deviceFound) {
        newMode = "adb";
    } else if (sideloadFound) {
        newMode = "sideload";
    } else if (recoveryFound) {
        newMode = "recovery";
    } else {
        newMode = "none"; // No adb-like device found
        serial = "N/A";
    }

    updateDeviceMode(newMode); // Update mode first
    updateDeviceSerial(serial, false); // Update ADB serial
}

void AndroidToolWidget::parseFastbootDevicesOutput(const QString& output)
{
    QString newMode = "none"; // Assume none unless fastboot found
    QString serial = "N/A";
    // Example parsing for 'fastboot devices' output:
    // R52D107M77L   fastboot
    // Or sometimes just:
    // R52D107M77L

    QStringList lines = output.split('\n', QString::SkipEmptyParts);
     bool fastbootFound = false;

    if (!lines.isEmpty()) {
        for (const QString& line : lines) {
            QString trimmedLine = line.trimmed();
             if (trimmedLine.isEmpty()) continue;

            QStringList parts = trimmedLine.split('\t', QString::SkipEmptyParts);
            if (parts.size() >= 1) {
                 serial = parts.at(0).trimmed();
                 if (parts.size() >= 2 && parts.at(1).trimmed() == "fastboot") {
                     fastbootFound = true;
                     break; // Found 'serial\tfastboot' format
                 } else if (parts.size() == 1 && !serial.isEmpty()) {
                     // Sometimes fastboot just outputs the serial number
                     // This is less reliable, depends on fastboot version/OS
                     // Assume fastboot if a non-empty line with one part is found? Risky.
                     // Let's stick to the 'serial\tfastboot' format for robustness.
                     // If fastboot is the *only* command running and outputs *something*,
                     // it's probably in fastboot mode.
                     // A more reliable check might be 'fastboot getvar version'
                     // For this example, stick to 'serial\tfastboot' format primarily.
                 }
            }
        }
    }

    if (fastbootFound) {
        newMode = "fastboot";
    } else {
        // If no fastboot device was found, the mode remains whatever the ADB check found ('adb', 'recovery', 'sideload', or 'none')
        newMode = deviceMode; // Keep the existing mode from ADB check
        serial = "N/A"; // No fastboot serial found
    }

    updateDeviceMode(newMode); // Update mode (this might not change if ADB mode was already set)
    updateDeviceSerial(serial, true); // Update Fastboot serial
}


// --- Button Slots Implementation ---

void AndroidToolWidget::on_detectButton_clicked()
{
     appendOutput("[*] 'Detect Device Details' button clicked.", Qt::blue);
     // This button now primarily triggers a state check immediately
     // More advanced detection (getprop, getvar all) would be added here later,
     // likely involving starting the mainProcess asynchronously with those commands.
     checkDeviceState(); // Trigger an immediate state check
}

void AndroidToolWidget::on_unlockButton_clicked()
{
     appendOutput("[*] 'Unlock Bootloader' button clicked.", Qt::blue);
     if (deviceMode != "fastboot") {
         appendOutput("[WARNING] Unlock requires Fastboot mode.", Qt::darkYellow);
         return;
     }
     // TODO: Add confirmation dialog
     // TODO: Get command from config (YAML)
     mainProcess->start("fastboot", QStringList() << "oem" << "unlock"); // Example command
}

void AndroidToolWidget::on_lockButton_clicked()
{
     appendOutput("[*] 'Lock Bootloader' button clicked.", Qt::blue);
      if (deviceMode != "fastboot") {
         appendOutput("[WARNING] Lock requires Fastboot mode.", Qt::darkYellow);
         return;
     }
     // TODO: Add confirmation dialog
     // TODO: Get command from config (YAML)
     mainProcess->start("fastboot", QStringList() << "oem" << "lock"); // Example command
}

void AndroidToolWidget::on_recoveryButton_clicked()
{
    appendOutput("[*] 'Flash Recovery' button clicked.", Qt::blue);
     if (deviceMode != "fastboot") {
         appendOutput("[WARNING] Flash Recovery requires Fastboot mode.", Qt::darkYellow);
         return;
     }
    // TODO: Add file dialog
    // TODO: Get partition from config (YAML)
    // mainProcess->start("fastboot", QStringList() << "flash" << "recovery" << "path/to/recovery.img"); // Example
     appendOutput("[INFO] Flash Recovery not fully implemented yet.", Qt::gray);
}

void AndroidToolWidget::on_magiskButton_clicked()
{
    appendOutput("[*] 'Flash Patched Boot' button clicked.", Qt::blue);
     if (deviceMode != "fastboot") {
         appendOutput("[WARNING] Flash Patched Boot requires Fastboot mode.", Qt::darkYellow);
         return;
     }
     // TODO: Add file dialog
     // TODO: Get partition from config (YAML)
     // mainProcess->start("fastboot", QStringList() << "flash" << "boot" << "path/to/patched_boot.img"); // Example
      appendOutput("[INFO] Flash Patched Boot not fully implemented yet.", Qt::gray);
}

void AndroidToolWidget::on_rebootSystemButton_clicked()
{
    appendOutput("[*] 'Reboot System' button clicked.", Qt::blue);
     if (deviceMode == "adb" || deviceMode == "recovery") {
          // TODO: Get command from config (YAML)
          mainProcess->start("adb", QStringList() << "reboot");
     } else if (deviceMode == "fastboot") {
         // TODO: Get command from config (YAML)
         mainProcess->start("fastboot", QStringList() << "reboot");
     } else {
          appendOutput("[WARNING] Reboot System requires ADB, Recovery, or Fastboot mode.", Qt::darkYellow);
     }
}

void AndroidToolWidget::on_rebootRecoveryButton_clicked()
{
    appendOutput("[*] 'Reboot Recovery' button clicked.", Qt::blue);
     if (deviceMode == "adb" || deviceMode == "recovery") {
         // TODO: Get command from config (YAML)
         mainProcess->start("adb", QStringList() << "reboot" << "recovery");
     } else {
          appendOutput("[WARNING] Reboot Recovery requires ADB or Recovery mode.", Qt::darkYellow);
     }
}

void AndroidToolWidget::on_rebootBootloaderButton_clicked()
{
    appendOutput("[*] 'Reboot Bootloader' button clicked.", Qt::blue);
     if (deviceMode == "adb" || deviceMode == "recovery") {
         // TODO: Get command from config (YAML)
         mainProcess->start("adb", QStringList() << "reboot" << "bootloader");
     } else {
          appendOutput("[WARNING] Reboot Bootloader requires ADB or Recovery mode.", Qt::darkYellow);
     }
}

void AndroidToolWidget::on_sideloadButton_clicked()
{
    appendOutput("[*] 'ADB Sideload ZIP' button clicked.", Qt::blue);
     if (deviceMode != "sideload") {
         appendOutput("[WARNING] ADB Sideload requires the device to be in Sideload mode.", Qt::darkYellow);
         return;
     }
     // TODO: Add file dialog
     // mainProcess->start("adb", QStringList() << "sideload" << "path/to/rom.zip"); // Example
      appendOutput("[INFO] ADB Sideload not fully implemented yet.", Qt::gray);
}

void AndroidToolWidget::on_adbPushButton_clicked()
{
    appendOutput("[*] 'ADB Push File' button clicked.", Qt::blue);
     if (deviceMode != "adb" && deviceMode != "recovery") {
         appendOutput("[WARNING] ADB Push requires ADB or Recovery mode.", Qt::darkYellow);
         return;
     }
      // TODO: Add file dialog (source and destination)
      // mainProcess->start("adb", QStringList() << "push" << "path/to/local/file" << "path/on/device"); // Example
       appendOutput("[INFO] ADB Push not fully implemented yet.", Qt::gray);
}

void AndroidToolWidget::on_adbPullButton_clicked()
{
    appendOutput("[*] 'ADB Pull File' button clicked.", Qt::blue);
     if (deviceMode != "adb" && deviceMode != "recovery") {
         appendOutput("[WARNING] ADB Pull requires ADB or Recovery mode.", Qt::darkYellow);
         return;
     }
     // TODO: Add input dialog (source file on device)
     // TODO: Add file dialog (destination local folder)
     // mainProcess->start("adb", QStringList() << "pull" << "path/on/device" << "path/to/local/folder"); // Example
      appendOutput("[INFO] ADB Pull not fully implemented yet.", Qt::gray);
}