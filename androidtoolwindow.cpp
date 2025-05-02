// Define file path at the top
#define YAML_FILE_PATH "devices.yaml" // <-- FIX 1: Define YAML file path here

#include "androidtoolwindow.h"

#include <QTextStream>
#include <QColor>
#include <QDebug>
#include <QStringList>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QPair>
#include <QMap>
#include <QFile>
#include <QTextStream>

#include <yaml-cpp/yaml.h> // Include YAML library header
#include <iostream>
#include <stdexcept>


// --- Helper to read YAML node safely ---
QString readYamlString(const YAML::Node& node, const QString& key, const QString& defaultValue = "") {
    if (node[key.toStdString()] && node[key.toStdString()].IsScalar()) {
        return QString::fromStdString(node[key.toStdString()].as<std::string>());
    }
    return defaultValue;
}

// --- Helper to load DeviceConfig from YAML Node ---
DeviceConfig loadDeviceConfigFromYaml(const YAML::Node& node) {
    DeviceConfig config; // Struct default constructor initializes members

    if (!node.IsMap()) return config;

    config.unlock_command = readYamlString(node, "unlock_command");
    config.lock_command = readYamlString(node, "lock_command");
    config.adb_reboot_bootloader_command = readYamlString(node, "adb_reboot_bootloader_command");
    config.adb_reboot_recovery_command = readYamlString(node, "adb_reboot_recovery_command");
    config.adb_reboot_system_command = readYamlString(node, "adb_reboot_system_command");
    config.fastboot_reboot_command = readYamlString(node, "fastboot_reboot_command");
    config.recovery_partition = readYamlString(node, "recovery_partition");
    config.boot_partition = readYamlString(node, "boot_partition");
    config.unlock_warning = readYamlString(node, "unlock_warning");
    config.lock_warning = readYamlString(node, "lock_warning");
    config.notes = readYamlString(node, "notes");

    return config;
}


AndroidToolWindow::AndroidToolWindow(QWidget *parent)
    : QMainWindow(parent),
      centralWidget(new QWidget(this)),
      mainLayout(new QVBoxLayout(centralWidget)),
      infoLayout(new QHBoxLayout()),
      buttonLayoutMain(new QHBoxLayout()),
      buttonLayoutReboot(new QHBoxLayout()),
      buttonLayoutFile(new QHBoxLayout()),
      manufacturerLabel(new QLabel("Manufacturer: Unknown")),
      modelLabel(new QLabel("Model: Unknown")),
      serialLabel(new QLabel("Serial: N/A")),
      modeLabel(new QLabel("Mode: Disconnected")),
      outputArea(new QTextEdit()),
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
      mainProcess(new QProcess(this)),
      stateCheckProcess(new QProcess(this)),
      stateTimer(new QTimer(this)),
      deviceMode("none"),
      deviceSerialAdb("N/A"),
      deviceSerialFastboot("N/A"),
      deviceManufacturer("Unknown"),
      deviceModel("Unknown"),
      currentRunningCommand(""),
      adbDetectedMode("none"),
      adbDetectedSerial("N/A"),
      adbCheckCompleted(false)
      // loadedConfig member is default-constructed
{
    setCentralWidget(centralWidget);
    setStatusBar(new QStatusBar(this));

    // --- Load Configuration ---
    loadConfig(YAML_FILE_PATH); // Load the YAML config on startup

    // --- UI Setup ---
    outputArea->setReadOnly(true);
    outputArea->setPlaceholderText("Command output and logs will appear here...");

    // Set up Context Menu for Output Area
    outputArea->setContextMenuPolicy(Qt::ActionsContextMenu);
    clearLogAction = new QAction("Clear Log", this);
    outputArea->addAction(clearLogAction);
    connect(clearLogAction, &QAction::triggered, this, &AndroidToolWindow::on_clearLogAction_triggered);


    // Info layout
    QFont boldFont;
    boldFont.setBold(true);
    modeLabel->setFont(boldFont);

    infoLayout->addWidget(manufacturerLabel);
    infoLayout->addWidget(modelLabel);
    infoLayout->addWidget(serialLabel);
    infoLayout->addStretch(1);
    infoLayout->addWidget(modeLabel);

    // Button layouts
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


    // Main layout (on the central widget)
    mainLayout->addLayout(infoLayout);
    mainLayout->addWidget(outputArea);
    mainLayout->addLayout(buttonLayoutMain);
    mainLayout->addLayout(buttonLayoutReboot);
    mainLayout->addLayout(buttonLayoutFile);
    mainLayout->addStretch(1);


    setGeometry(100, 100, 900, 700);

    // --- Connections ---
    connect(detectButton, &QPushButton::clicked, this, &AndroidToolWindow::on_detectButton_clicked);
    connect(unlockButton, &QPushButton::clicked, this, &AndroidToolWindow::on_unlockButton_clicked);
    connect(lockButton, &QPushButton::clicked, this, &AndroidToolWindow::on_lockButton_clicked);
    connect(recoveryButton, &QPushButton::clicked, this, &AndroidToolWindow::on_recoveryButton_clicked);
    connect(magiskButton, &QPushButton::clicked, this, &AndroidToolWindow::on_magiskButton_clicked);
    connect(rebootSystemButton, &QPushButton::clicked, this, &AndroidToolWindow::on_rebootSystemButton_clicked);
    // FIX: Corrected class name in connect call
    connect(rebootRecoveryButton, &QPushButton::clicked, this, &AndroidToolWindow::on_rebootRecoveryButton_clicked);
    connect(rebootBootloaderButton, &QPushButton::clicked, this, &AndroidToolWindow::on_rebootBootloaderButton_clicked);
    connect(sideloadButton, &QPushButton::clicked, this, &AndroidToolWindow::on_sideloadButton_clicked);
    connect(adbPushButton, &QPushButton::clicked, this, &AndroidToolWindow::on_adbPushButton_clicked);
    connect(adbPullButton, &QPushButton::clicked, this, &AndroidToolWindow::on_adbPullButton_clicked);

    // Main QProcess connections
    connect(mainProcess, &QProcess::readyReadStandardOutput, this, &AndroidToolWindow::processReadyReadStandardOutput);
    connect(mainProcess, &QProcess::readyReadStandardError, this, &AndroidToolWindow::processReadyReadStandardError);
    connect(mainProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &AndroidToolWindow::processFinished);
    connect(mainProcess, &QProcess::errorOccurred, this, &AndroidToolWindow::processErrorOccurred);

    // State Check QProcess connections
    // We don't need to read output directly here, parsing happens after finished
    // connect(stateCheckProcess, &QProcess::readyReadStandardOutput, this, &AndroidToolWindow::stateCheckProcessReadyReadStandardOutput);
    // connect(stateCheckProcess, &QProcess::readyReadStandardError, this, &AndroidToolWindow::stateCheckProcessReadyReadStandardError);
    connect(stateCheckProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &AndroidToolWindow::stateCheckProcessFinished);
    connect(stateCheckProcess, &QProcess::errorOccurred, this, &AndroidToolWindow::stateCheckProcessErrorOccurred);

    // Timer connection
    connect(stateTimer, &QTimer::timeout, this, &AndroidToolWindow::checkDeviceState);

    // --- Initial State ---
    stateTimer->start(3000); // Check every 3 seconds
    checkDeviceState(); // Perform an initial state check immediately

    // Initial button states are set by checkDeviceState -> updateButtonStates
    updateStatusBar("Ready. Ensure ADB/Fastboot drivers are installed.", 0);
}

AndroidToolWindow::~AndroidToolWindow()
{
    // Child objects with 'this' as parent are automatically deleted by Qt's object tree.
    // Consider killing processes on application exit if necessary.
    // if (mainProcess->state() != QProcess::NotRunning) mainProcess->kill();
    // if (stateCheckProcess->state() != QProcess::NotRunning) stateCheckProcess->kill();
}

// --- Helper Functions Implementation ---

void AndroidToolWindow::appendOutput(const QString &text, const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);

    QTextCursor cursor = outputArea->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text + "\n", format);

    outputArea->verticalScrollBar()->setValue(outputArea->verticalScrollBar()->maximum());
}

void AndroidToolWindow::updateDeviceMode(const QString& mode)
{
    if (deviceMode != mode) {
        deviceMode = mode;
        QString capitalizedMode = deviceMode.left(1).toUpper() + deviceMode.mid(1).toLower();
        modeLabel->setText("Mode: " + capitalizedMode);
        appendOutput(QString("[*] Device mode updated to: %1").arg(capitalizedMode), Qt::gray);
        updateButtonStates();
        // When mode changes, reset manufacturer/model/serial until re-detected in the new mode
        if (deviceMode == "none" || deviceMode == "tools_missing" || deviceMode == "check_failed") {
            deviceManufacturer = "Unknown"; // Reset Mfr/Model
            deviceModel = "Unknown";
            updateDeviceDetailsLabels(); // Update Mfr/Model labels
            serialLabel->setText("Serial: N/A"); // Clear serial label
        }
         // Manufacturer/Model are kept across ADB modes (adb, recovery, sideload)
         // and reset when going to/from fastboot or none/error states.
         // Serial is updated specifically by updateDeviceSerial.
    }
}

void AndroidToolWindow::updateDeviceSerial(const QString& serial, bool isFastboot)
{
    QString* currentSerial = isFastboot ? &deviceSerialFastboot : &deviceSerialAdb;
    if (*currentSerial != serial) {
        *currentSerial = serial;
         // Update the displayed serial if the currently displayed mode matches this serial type
        if ((isFastboot && deviceMode == "fastboot") || (!isFastboot && (deviceMode == "adb" || deviceMode == "recovery" || deviceMode == "sideload"))) {
             serialLabel->setText("Serial: " + serial);
             if (serial != "N/A") { // Only log if a serial was actually found
                 appendOutput(QString("[*] Device serial (%1) updated to: %2").arg(isFastboot ? "Fastboot" : "ADB").arg(serial), Qt::gray);
             }
        } else if (deviceMode == "none") {
            // Clear serial if mode goes to none
            serialLabel->setText("Serial: N/A");
        }
    }
}

void AndroidToolWindow::updateDeviceDetailsLabels()
{
    // Update Manufacturer and Model labels based on current member variables
    manufacturerLabel->setText("Manufacturer: " + deviceManufacturer);
    modelLabel->setText("Model: " + deviceModel);
    // Serial is handled by updateDeviceSerial
}


void AndroidToolWindow::updateButtonStates()
{
    bool mainProcessRunning = mainProcess->state() != QProcess::NotRunning;
    bool stateCheckProcessRunning = stateCheckProcess->state() != QProcess::NotRunning;
    bool disableActions = mainProcessRunning || stateCheckProcessRunning;

    bool is_adb = deviceMode == "adb";
    bool is_fastboot = deviceMode == "fastboot";
    bool is_sideload = deviceMode == "sideload";
    bool is_recovery = deviceMode == "recovery";
    // bool is_connected = deviceMode != "none"; // Unused

    // Main Control Buttons (mostly Fastboot)
    unlockButton->setEnabled(is_fastboot && !disableActions);
    lockButton->setEnabled(is_fastboot && !disableActions);
    recoveryButton->setEnabled(is_fastboot && !disableActions);
    magiskButton->setEnabled(is_fastboot && !disableActions);

    // Reboot Buttons
    rebootSystemButton->setEnabled((is_adb || is_fastboot || is_recovery || is_sideload) && !disableActions);
    rebootRecoveryButton->setEnabled((is_adb || is_recovery || is_sideload) && !disableActions);
    rebootBootloaderButton->setEnabled((is_adb || is_recovery || is_sideload) && !disableActions);

    // Sideload / File Operations
    sideloadButton->setEnabled(is_sideload && !disableActions);
    adbPushButton->setEnabled((is_adb || is_recovery || is_sideload) && !disableActions);
    adbPullButton->setEnabled((is_adb || is_recovery || is_sideload) && !disableActions);

    // Detect button
    detectButton->setEnabled(!disableActions);
}

void AndroidToolWindow::updateStatusBar(const QString& message, int timeout)
{
    if (statusBar()) {
        statusBar()->showMessage(message, timeout);
    } else {
        qDebug() << "STATUS:" << message;
    }
}


// --- Main QProcess Slots Implementation ---

void AndroidToolWindow::processReadyReadStandardOutput()
{
    QTextStream stream(mainProcess->readAllStandardOutput());
    while (!stream.atEnd()) {
        appendOutput("[STDOUT] " + stream.readLine(), Qt::darkCyan);
    }
}

void AndroidToolWindow::processReadyReadStandardError()
{
    QTextStream stream(mainProcess->readAllStandardError());
     while (!stream.atEnd()) {
        appendOutput("[STDERR] " + stream.readLine(), Qt::red);
    }
}

void AndroidToolWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString commandId = currentRunningCommand;
    currentRunningCommand.clear();

    QString statusText = (exitStatus == QProcess::NormalExit) ? "Normal Exit" : "Crash";
    appendOutput(QString("[*] Command finished: %1 (Exit Code: %2, Status: %3)").arg(commandId, QString::number(exitCode), statusText), Qt::blue);

    bool commandSuccess = (exitStatus == QProcess::NormalExit && exitCode == 0);

    if (!commandSuccess) {
         appendOutput("[ERROR] Command failed. Check output above.", Qt::darkRed);
         updateStatusBar("Command '" + commandId + "' failed.", 5000);
         // TODO: Add more specific error parsing here based on 'commandId' and stderr
    } else {
         appendOutput("[SUCCESS] Command completed successfully.", Qt::darkGreen);
         updateStatusBar("Command '" + commandId + "' successful.", 5000);
          // TODO: Add more specific success messages based on 'commandId'
          if (commandId == "unlock" || commandId == "lock") {
              appendOutput("[*] Check device screen for confirmation!", Qt::darkYellow);
          }
           if (commandId == "flash_recovery" || commandId == "flash_boot") {
              appendOutput("[*] Flash command sent. Check device and output for progress.", Qt::darkYellow);
          }
           if (commandId == "sideload") {
              appendOutput("[*] Sideload command sent. Check device and output for progress.", Qt::darkYellow);
          }
           if (commandId == "adb_push") {
               appendOutput("[*] Push command sent. Check output for progress.", Qt::darkYellow);
           }
            if (commandId == "adb_pull") {
               appendOutput("[*] Pull command sent. Check output for progress.", Qt::darkYellow);
           }
    }

    // --- Post-Command Logic ---
    if (commandId == "detect_adb_props") {
        if (commandSuccess) {
            parseAdbPropertiesOutput(mainProcess->readAllStandardOutput());
        } else {
            appendOutput("[WARNING] Failed to get ADB properties.", Qt::darkYellow);
             if (deviceManufacturer == "Unknown" || manufacturerLabel->text().contains("Checking")) deviceManufacturer = "Unknown (ADB Fail)";
             if (deviceModel == "Unknown" || modelLabel->text().contains("Checking")) deviceModel = "Unknown (ADB Fail)"; // Corrected variable name
             updateDeviceDetailsLabels();
        }
        // After ADB properties, if device is in Fastboot mode according to state check, get Fastboot variables
        if (deviceMode == "fastboot") {
             runFastbootVariablesCheck(); // This will set currentRunningCommand again
        } else {
             updateButtonStates();
             updateStatusBar("Detection complete.", 5000);
        }

    } else if (commandId == "detect_fastboot_vars") {
         if (commandSuccess) {
             parseFastbootVariablesOutput(mainProcess->readAllStandardOutput());
         } else {
            appendOutput("[WARNING] Failed to get Fastboot variables.", Qt::darkYellow);
            if (deviceManufacturer == "Unknown" || manufacturerLabel->text().contains("Checking")) deviceManufacturer = "Unknown (Fastboot Fail)";
             modelLabel->setText("Model: Unknown (Fastboot Fail)"); // FIX: Use ->setText
             updateDeviceDetailsLabels();
         }
         updateButtonStates();
         updateStatusBar("Detection complete.", 5000);

    } else {
        updateButtonStates();
        QTimer::singleShot(500, this, &AndroidToolWindow::checkDeviceState);
    }
}

void AndroidToolWindow::processErrorOccurred(QProcess::ProcessError error)
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
    appendOutput(QString("[ERROR] Main Process Error: %1 (%2)").arg(errorString, mainProcess->errorString()), Qt::darkRed);
    updateStatusBar("Main Process Error.", 5000);

    currentRunningCommand.clear();
    updateButtonStates();
    QTimer::singleShot(500, this, &AndroidToolWindow::checkDeviceState);
}


// --- State Check QProcess Slots Implementation ---
void AndroidToolWindow::stateCheckProcessReadyReadStandardOutput() { /* Optional: append to log if verbose */ }
void AndroidToolWindow::stateCheckProcessReadyReadStandardError() { /* Optional: append errors to log if verbose */ }

void AndroidToolWindow::stateCheckProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (mainProcess->state() != QProcess::NotRunning) {
        qDebug() << "State check finished, but main process is running. Skipping parsing.";
        return;
    }

    QString command = stateCheckProcess->program();
    qDebug() << "State check command finished:" << command << "Exit Code:" << exitCode;

    QString output = stateCheckProcess->readAllStandardOutput();

    if (command == "adb") {
        adbCheckCompleted = true;

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
             QPair<QString, QString> adbResult = parseAdbDevicesOutput(output);
             adbDetectedMode = adbResult.first;
             adbDetectedSerial = adbResult.second;
             updateDeviceSerial(adbDetectedSerial, false);
        } else {
              adbDetectedMode = "none";
              adbDetectedSerial = "N/A";
              updateDeviceSerial("N/A", false);
         }
         runFastbootStateCheck();

    } else if (command == "fastboot") {
        bool fastbootFound = false;
        QString fastbootSerial = "N/A";

         if (exitStatus == QProcess::NormalExit && exitCode == 0) {
             QPair<bool, QString> fastbootResult = parseFastbootDevicesOutput(output);
             fastbootFound = fastbootResult.first;
             fastbootSerial = fastbootResult.second;
             updateDeviceSerial(fastbootSerial, true);

         } else {
              fastbootFound = false;
              fastbootSerial = "N/A";
              updateDeviceSerial("N/A", true);
         }

        QString finalMode = "none";
        if (fastbootFound) {
            finalMode = "fastboot";
        } else {
            finalMode = adbDetectedMode;
        }

        updateDeviceMode(finalMode);

        adbCheckCompleted = false;
        adbDetectedMode = "none";
        adbDetectedSerial = "N/A";

         updateStatusBar("Device state checked.", 2000);

    }
}

void AndroidToolWindow::stateCheckProcessErrorOccurred(QProcess::ProcessError error)
{
     QString errorString;
    switch (error) {
        case QProcess::FailedToStart: errorString = "Failed to Start (Executable not found)"; break;
        case QProcess::Crashed: errorString = "Crashed"; break;
        case QProcess::Timedout: errorString = "Timed Out"; break;
        case QProcess::ReadError: errorString = "Read Error"; break;
        case QProcess::WriteError: errorString = "WriteError"; break;
        case QProcess::UnknownError:
        default: errorString = "Unknown Error"; break;
    }
    qDebug() << "State Check Process Error: " << errorString << stateCheckProcess->errorString();

    if (error == QProcess::FailedToStart) {
         QString program = stateCheckProcess->program();
         if (program == "adb" || program == "fastboot") {
             if (program == "adb") manufacturerLabel->setText("Manufacturer: ADB Not Found");
             if (program == "fastboot") modelLabel->setText("Model: Fastboot Not Found");

             static bool toolsCriticalWarningShown = false;
             if (!toolsCriticalWarningShown) {
                 appendOutput(QString("[CRITICAL] '%1' command not found. Ensure Android SDK Platform-Tools are in your PATH.").arg(program), Qt::darkRed);
                 QMessageBox::critical(this, "Tool Error", QString("'%1' executable not found.\nPlease ensure the Android SDK Platform-Tools are installed and added to your system's PATH environment variable.").arg(program));
                 toolsCriticalWarningShown = true;
             }
         }
    }
}

// --- State Check Logic Helpers Implementation ---

void AndroidToolWindow::checkDeviceState()
{
    if (mainProcess->state() == QProcess::NotRunning && stateCheckProcess->state() != QProcess::Starting && stateCheckProcess->state() != QProcess::Running && !adbCheckCompleted) {
        runAdbStateCheck();
    } else {
        // qDebug() << "checkDeviceState skipped: another process running or adb check pending fastboot.";
    }
}

void AndroidToolWindow::runAdbStateCheck()
{
     qDebug() << "Starting ADB state check...";
     if (manufacturerLabel->text().contains("Not Found") || manufacturerLabel->text().startsWith("Manufacturer: Unknown") || manufacturerLabel->text().contains("Checking")) manufacturerLabel->setText("Manufacturer: Checking...");
     if (modelLabel->text().contains("Not Found") || modelLabel->text().startsWith("Model: Unknown") || modelLabel->text().contains("Checking")) modelLabel->setText("Model: Checking...");

     adbCheckCompleted = false;
     adbDetectedMode = "none";
     adbDetectedSerial = "N/A";

     stateCheckProcess->start("adb", QStringList() << "devices");
     updateStatusBar("Checking device state (ADB)...", 0);
}

void AndroidToolWindow::runFastbootStateCheck()
{
    qDebug() << "Starting Fastboot state check...";
    if (manufacturerLabel->text().contains("Not Found") || manufacturerLabel->text().startsWith("Manufacturer: Unknown") || manufacturerLabel->text().contains("Checking")) manufacturerLabel->setText("Manufacturer: Checking...");
    if (modelLabel->text().contains("Not Found") || modelLabel->text().startsWith("Model: Unknown") || modelLabel->text().contains("Checking")) modelLabel->setText("Model: Checking...");

    stateCheckProcess->start("fastboot", QStringList() << "devices");
    updateStatusBar("Checking device state (Fastboot)...", 0);
}

QPair<QString, QString> AndroidToolWindow::parseAdbDevicesOutput(const QString& output)
{
    QString detectedMode = "none";
    QString detectedSerial = "N/A";

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    bool deviceFound = false;
    bool recoveryFound = false;
    bool sideloadFound = false;

    if (lines.size() > 1 && lines.first().contains("List of devices attached")) {
        for (int i = 1; i < lines.size(); ++i) {
            QString line = lines.at(i).trimmed();
            if (line.isEmpty() || line.startsWith('#') || line.startsWith('*') || line.startsWith(' ')) continue;

            QStringList parts = line.split('\t', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                detectedSerial = parts.at(0).trimmed();
                QString status = parts.at(1).trimmed();

                if (status == "device") {
                    deviceFound = true;
                    break;
                } else if (status == "sideload") {
                    sideloadFound = true;
                } else if (status == "recovery") {
                    recoveryFound = true;
                }
            }
        }
    }

    if (deviceFound) {
        detectedMode = "adb";
    } else if (sideloadFound) {
        detectedMode = "sideload";
    } else if (recoveryFound) {
        detectedMode = "recovery";
    } else {
        detectedMode = "none";
        detectedSerial = "N/A";
    }

    return qMakePair(detectedMode, detectedSerial);
}

QPair<bool, QString> AndroidToolWindow::parseFastbootDevicesOutput(const QString& output)
{
    QString detectedSerial = "N/A";
    bool fastbootFound = false;

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    if (!lines.isEmpty()) {
        for (const QString& line : lines) {
            QString trimmedLine = line.trimmed();
             if (trimmedLine.isEmpty() || trimmedLine.startsWith('#') || trimmedLine.startsWith('*')) continue;

            QStringList parts = trimmedLine.split('\t', Qt::SkipEmptyParts);
            if (parts.size() >= 1) {
                 QString currentSerial = parts.at(0).trimmed();
                 if (!currentSerial.isEmpty()) {
                    if (parts.size() >= 2 && parts.at(1).trimmed() == "fastboot") {
                         fastbootFound = true;
                         detectedSerial = currentSerial;
                         break;
                     }
                 }
            }
        }
    }
    return qMakePair(fastbootFound, detectedSerial);
}


// --- Detection Logic Helpers Implementation ---
void AndroidToolWindow::runAdbPropertiesCheck()
{
    if (mainProcess->state() != QProcess::NotRunning) {
         qWarning() << "Attempted to start AdbPropertiesCheck while mainProcess is running.";
         return;
    }
    appendOutput("[*] Running adb shell getprop...", Qt::blue);
    currentRunningCommand = "detect_adb_props";
    mainProcess->start("adb", QStringList() << "shell" << "getprop");
    updateButtonStates();
    updateStatusBar("Getting ADB properties...", 0);
    manufacturerLabel->setText("Manufacturer: Getting...");
    modelLabel->setText("Model: Getting...");
}

void AndroidToolWindow::runFastbootVariablesCheck()
{
     if (mainProcess->state() != QProcess::NotRunning) {
         qWarning() << "Attempted to start FastbootVariablesCheck while mainProcess is running.";
         return;
    }
     appendOutput("[*] Running fastboot getvar all...", Qt::blue);
     currentRunningCommand = "detect_fastboot_vars";
     mainProcess->start("fastboot", QStringList() << "getvar" << "all");
     updateButtonStates();
     updateStatusBar("Getting Fastboot variables...", 0);
     manufacturerLabel->setText("Manufacturer: Getting...");
     modelLabel->setText("Model: Getting...");
}

void AndroidToolWindow::parseAdbPropertiesOutput(const QString& output)
{
    QString tempManufacturer = "Unknown";
    QString tempModel = "Unknown";

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty() || !trimmedLine.startsWith('[') || !trimmedLine.endsWith(']')) continue;

        int colonIndex = trimmedLine.indexOf("]: [");
        if (colonIndex != -1) {
            QString key = trimmedLine.mid(1, colonIndex - 1);
            QString value = trimmedLine.mid(colonIndex + 4, trimmedLine.length() - colonIndex - 5);

            if (key == "ro.product.manufacturer") {
                tempManufacturer = value;
            } else if (key == "ro.product.model") {
                tempModel = value;
            }
        }
    }

    if (tempManufacturer != "Unknown") deviceManufacturer = tempManufacturer;
    if (tempModel != "Unknown") deviceModel = tempModel;

    appendOutput(QString("[*] ADB Properties Found: Manufacturer='%1', Model='%2'").arg(deviceManufacturer, deviceModel), Qt::gray);
    updateDeviceDetailsLabels();
}

void AndroidToolWindow::parseFastbootVariablesOutput(const QString& output)
{
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    QString tempModel = "Unknown";

    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty() || !trimmedLine.startsWith('(')) continue;

        int colonIndex = trimmedLine.indexOf(':');
        int spaceAfterPrefix = trimmedLine.indexOf(')');

        if (colonIndex != -1 && spaceAfterPrefix != -1 && colonIndex > spaceAfterPrefix) {
             QString key = trimmedLine.mid(spaceAfterPrefix + 1, colonIndex - spaceAfterPrefix - 1).trimmed();
             QString value = trimmedLine.mid(colonIndex + 1).trimmed();

             if (key == "product") {
                 tempModel = value + " (codename)";
             }
        }
    }

     if (deviceModel == "Unknown" || (tempModel != "Unknown" && tempModel.contains("(codename)"))) {
          deviceModel = tempModel;
     }

    appendOutput(QString("[*] Fastboot Variables Parsed. Manufacturer='%1', Model='%2'").arg(deviceManufacturer, deviceModel), Qt::gray);
    updateDeviceDetailsLabels();
}

// --- YAML Configuration Logic ---
void AndroidToolWindow::loadConfig(const QString& filePath) {
    QFileInfo configFile(filePath);
    if (!configFile.exists()) {
        appendOutput(QString("[WARNING] Configuration file not found: %1. Using default commands only.").arg(filePath), Qt::darkYellow);
        updateStatusBar(QString("Config not found: %1. Using defaults.").arg(filePath), 5000);
        // Populate the default_config with built-in values mirroring Python if file not found
        loadedConfig.default_config.unlock_command = "fastboot oem unlock";
        loadedConfig.default_config.lock_command = "fastboot oem lock";
        loadedConfig.default_config.adb_reboot_bootloader_command = "adb reboot bootloader";
        loadedConfig.default_config.adb_reboot_recovery_command = "adb reboot recovery";
        loadedConfig.default_config.adb_reboot_system_command = "adb reboot";
        loadedConfig.default_config.fastboot_reboot_command = "fastboot reboot";
        loadedConfig.default_config.recovery_partition = "recovery";
        loadedConfig.default_config.boot_partition = "boot";
        loadedConfig.default_config.unlock_warning = "Unlocking the bootloader will erase all data (factory reset) and may void your warranty! Proceed with caution.";
        loadedConfig.default_config.lock_warning = "Locking the bootloader will erase all data. Ensure you have backups before proceeding.";
        loadedConfig.default_config.notes = "Built-in default configuration.";

        return;
    }

    try {
        YAML::Node config = YAML::LoadFile(filePath.toStdString());

        if (config["default"]) {
            loadedConfig.default_config = loadDeviceConfigFromYaml(config["default"]);
            appendOutput("[*] Loaded default configuration from file.", Qt::gray);
        } else {
             appendOutput("[WARNING] 'default' entry not found in config file. Using built-in defaults for commands/warnings.", Qt::darkYellow);
              // Manually populate the default_config with built-in values if file didn't have a default
             loadedConfig.default_config.unlock_command = "fastboot oem unlock";
             loadedConfig.default_config.lock_command = "fastboot oem lock";
             loadedConfig.default_config.adb_reboot_bootloader_command = "adb reboot bootloader";
             loadedConfig.default_config.adb_reboot_recovery_command = "adb reboot recovery";
             loadedConfig.default_config.adb_reboot_system_command = "adb reboot";
             loadedConfig.default_config.fastboot_reboot_command = "fastboot reboot";
             loadedConfig.default_config.recovery_partition = "recovery";
             loadedConfig.default_config.boot_partition = "boot";
             loadedConfig.default_config.unlock_warning = "Unlocking the bootloader will erase all data (factory reset) and may void your warranty! Proceed with caution.";
             loadedConfig.default_config.lock_warning = "Locking the bootloader will erase all data. Ensure you have backups before proceeding.";
             loadedConfig.default_config.notes = "Built-in default configuration.";
        }

        // Load device-specific configs
        for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {
            QString deviceName = QString::fromStdString(it->first.as<std::string>());
            if (deviceName.toLower() == "default") continue;

            if (it->second.IsMap()) {
                loadedConfig.devices[deviceName.toLower()] = loadDeviceConfigFromYaml(it->second);
                appendOutput(QString("[*] Loaded configuration for device: %1").arg(deviceName), Qt::gray);
            } else {
                 appendOutput(QString("[WARNING] Ignoring invalid entry for device: %1 (Not a map)").arg(deviceName), Qt::darkYellow);
            }
        }

        appendOutput(QString("[SUCCESS] Configuration loaded from %1.").arg(filePath), Qt::darkGreen);
        updateStatusBar(QString("Config loaded from %1.").arg(filePath), 5000);

    } catch (const YAML::BadFile& e) {
        appendOutput(QString("[ERROR] Failed to load config file %1: %2").arg(filePath, e.what()), Qt::darkRed);
        updateStatusBar(QString("Config load error: %1").arg(e.what()), 5000);
        // Default config is already populated if file didn't exist/failed to load
    } catch (const YAML::ParserException& e) {
        appendOutput(QString("[ERROR] Failed to parse config file %1: %2").arg(filePath, e.what()), Qt::darkRed);
        updateStatusBar(QString("Config parse error: %1").arg(e.what()), 5000);
         // Default config is already populated
    } catch (const std::exception& e) {
        appendOutput(QString("[ERROR] An unexpected error occurred loading config: %1").arg(e.what()), Qt::darkRed);
        updateStatusBar(QString("Config error: %1").arg(e.what()), 5000);
         // Default config is already populated
    } catch (...) {
         appendOutput("[ERROR] An unknown error occurred loading config.", Qt::darkRed);
         updateStatusBar("Config error: Unknown.", 5000);
         // Default config is already populated
    }
}

// --- FIX: getDeviceConfig implementation (return by value) ---
DeviceConfig AndroidToolWindow::getDeviceConfig() const { // Return by value
    // Try exact match (manufacturer + model)
    QString exactKey = deviceManufacturer.toLower() + " " + deviceModel.toLower();
    if (!deviceManufacturer.isEmpty() && !deviceModel.isEmpty() && loadedConfig.devices.contains(exactKey)) {
        qDebug() << "Using config for:" << exactKey;
        return loadedConfig.devices.value(exactKey); // Returns a copy
    }

    // Try manufacturer match
    QString manufacturerKey = deviceManufacturer.toLower();
    if (!deviceManufacturer.isEmpty() && loadedConfig.devices.contains(manufacturerKey)) {
         qDebug() << "Using config for manufacturer:" << manufacturerKey;
         return loadedConfig.devices.value(manufacturerKey); // Returns a copy
    }

    // Fallback to default
    qDebug() << "Using default config.";
    return loadedConfig.default_config; // Returns a copy
}


// --- Button Slots Implementation ---

void AndroidToolWindow::on_detectButton_clicked()
{
     appendOutput("[*] 'Detect Device Details' button clicked.", Qt::blue);
     if (mainProcess->state() != QProcess::NotRunning || stateCheckProcess->state() != QProcess::NotRunning) {
         appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
         return;
     }

     updateStatusBar("Starting device detection...", 0);

     // Reset known details before starting detection process
     deviceManufacturer = "Unknown";
     deviceModel = "Unknown";
     updateDeviceDetailsLabels();


     // Start detection based on current mode or try ADB first
     if (deviceMode == "adb" || deviceMode == "recovery" || deviceMode == "sideload") {
          runAdbPropertiesCheck();
     } else if (deviceMode == "fastboot") {
          runFastbootVariablesCheck();
     } else {
          checkDeviceState();
          updateStatusBar("No device detected. Triggering state check.", 3000);
     }
}

// --- FIX: on_unlockButton_clicked implementation (uses config) ---
void AndroidToolWindow::on_unlockButton_clicked()
{
     appendOutput("[*] 'Unlock Bootloader' button clicked.", Qt::blue);
     if (mainProcess->state() != QProcess::NotRunning) {
         appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
         return;
     }
     if (deviceMode != "fastboot") {
         appendOutput("[WARNING] Unlock requires Fastboot mode.", Qt::darkYellow);
         updateStatusBar("Unlock failed: Wrong mode.", 5000);
         return;
     }

     // --- Get config and show warning ---
     const DeviceConfig& config = getDeviceConfig(); // Use const reference here
     QString warning = config.unlock_warning;
     if (warning.isEmpty()) warning = "Unlocking will erase all data. Proceed with caution.";

     QMessageBox::StandardButton reply = QMessageBox::warning(this, "Unlock Bootloader - WARNING",
                                    warning + "\n\n"
                                    "THIS ACTION CANNOT BE UNDONE AND WILL ERASE YOUR PHONE.\n\n"
                                    "Do you want to proceed?",
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

     if (reply == QMessageBox::No) {
         appendOutput("[*] Bootloader unlock cancelled by user.", Qt::gray);
         updateStatusBar("Unlock cancelled.", 2000);
         return;
     }

     appendOutput("[*] User confirmed unlock warning.", Qt::darkYellow);
     updateStatusBar("Unlocking bootloader...", 0);

     // --- Get command from config and start process ---
     QString command = config.unlock_command;
     if (command.isEmpty()) {
         appendOutput("[ERROR] Unlock command not defined in config or default!", Qt::darkRed);
         updateStatusBar("Unlock failed: Config missing.", 5000);
         return;
     }

     QStringList commandParts = command.split(' ', Qt::SkipEmptyParts);
     if (commandParts.isEmpty()) {
          appendOutput("[ERROR] Invalid unlock command in config!", Qt::darkRed);
          updateStatusBar("Unlock failed: Invalid config.", 5000);
          return;
     }
     QString program = commandParts.first();
     QStringList arguments = commandParts.mid(1);

     currentRunningCommand = "unlock";
     mainProcess->start(program, arguments);
     updateButtonStates();
}

// --- FIX: on_lockButton_clicked implementation (uses config) ---
void AndroidToolWindow::on_lockButton_clicked()
{
     appendOutput("[*] 'Lock Bootloader' button clicked.", Qt::blue);
     if (mainProcess->state() != QProcess::NotRunning) {
         appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
         return;
     }
      if (deviceMode != "fastboot") {
         appendOutput("[WARNING] Lock requires Fastboot mode.", Qt::darkYellow);
         updateStatusBar("Lock failed: Wrong mode.", 5000);
         return;
     }

     // --- Get config and show warning ---
     const DeviceConfig& config = getDeviceConfig(); // Use const reference here
     QString warning = config.lock_warning;
     if (warning.isEmpty()) warning = "Locking will erase all data. Ensure backups.";

     QMessageBox::StandardButton reply = QMessageBox::warning(this, "Lock Bootloader - WARNING",
                                    warning + "\n\n"
                                    "THIS ACTION CANNOT BE UNDONE AND WILL ERASE YOUR PHONE.\n\n"
                                    "Do you want to proceed?",
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

     if (reply == QMessageBox::No) {
         appendOutput("[*] Bootloader lock cancelled by user.", Qt::gray);
         updateStatusBar("Lock cancelled.", 2000);
         return;
     }

     appendOutput("[*] User confirmed lock warning.", Qt::darkYellow);
     updateStatusBar("Locking bootloader...", 0);

     // --- Get command from config and start process ---
     QString command = config.lock_command;
      if (command.isEmpty()) {
         appendOutput("[ERROR] Lock command not defined in config or default!", Qt::darkRed);
         updateStatusBar("Lock failed: Config missing.", 5000);
         return;
     }
     QStringList commandParts = command.split(' ', Qt::SkipEmptyParts);
     if (commandParts.isEmpty()) {
          appendOutput("[ERROR] Invalid lock command in config!", Qt::darkRed);
          updateStatusBar("Lock failed: Invalid config.", 5000);
          return;
     }
     QString program = commandParts.first();
     QStringList arguments = commandParts.mid(1);

     currentRunningCommand = "lock";
     mainProcess->start(program, arguments);
     updateButtonStates();
}

// --- FIX: on_recoveryButton_clicked implementation (uses config) ---
void AndroidToolWindow::on_recoveryButton_clicked()
{
    appendOutput("[*] 'Flash Recovery' button clicked.", Qt::blue);
    if (mainProcess->state() != QProcess::NotRunning) {
        appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
        return;
    }
     if (deviceMode != "fastboot") {
         appendOutput("[WARNING] Flash Recovery requires Fastboot mode.", Qt::darkYellow);
         updateStatusBar("Flash failed: Wrong mode.", 5000);
         return;
     }

    // --- Get config ---
    const DeviceConfig& config = getDeviceConfig();
    QString partition = config.recovery_partition;
    if (partition.isEmpty()) {
         appendOutput("[ERROR] Recovery partition not defined in config!", Qt::darkRed);
         updateStatusBar("Flash failed: Config missing.", 5000);
         return;
    }


    // --- Get File Path ---
    // TODO: Get file filter and default dir from config (YAML)
    QString recoveryPath = QFileDialog::getOpenFileName(this, QString("Select Recovery Image (.img) for '%1'").arg(partition), QDir::homePath(), "Image Files (*.img);;All Files (*)");

    if (recoveryPath.isEmpty()) {
        appendOutput("[*] File selection cancelled.", Qt::gray);
         updateStatusBar("Flash cancelled.", 2000);
        return;
    }

    appendOutput(QString("[*] Selected file: %1").arg(recoveryPath), Qt::blue);

    // --- Show Confirmation ---
    // TODO: Get warning from config (YAML) if different from generic
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Flash Recovery",
                                     QString("Are you sure you want to flash '%1' to the '%2' partition?\n\n").arg(QFileInfo(recoveryPath).fileName(), partition) +
                                     "WARNING: Flashing an incorrect image can brick your device!",
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::No) {
        appendOutput("[*] Recovery flash cancelled by user.", Qt::gray);
         updateStatusBar("Flash cancelled.", 2000);
        return;
    }

    appendOutput("[*] User confirmed recovery flash.", Qt::darkYellow);
    updateStatusBar(QString("Flashing %1...").arg(QFileInfo(recoveryPath).fileName()), 0);

    // --- Send Flash Command ---
    // TODO: Get command template from config (YAML) if needed, or hardcode 'fastboot flash'
    QString program = "fastboot";
    QStringList arguments;
    arguments << "flash" << partition << recoveryPath;

    currentRunningCommand = "flash_recovery";
    mainProcess->start(program, arguments);
    updateButtonStates();
}

// --- FIX: on_magiskButton_clicked implementation (uses config) ---
void AndroidToolWindow::on_magiskButton_clicked()
{
    appendOutput("[*] 'Flash Patched Boot' button clicked.", Qt::blue);
    if (mainProcess->state() != QProcess::NotRunning) {
        appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
        return;
    }
     if (deviceMode != "fastboot") {
         appendOutput("[WARNING] Flash Patched Boot requires Fastboot mode.", Qt::darkYellow);
          updateStatusBar("Flash failed: Wrong mode.", 5000);
         return;
     }

     // --- Get config ---
    const DeviceConfig& config = getDeviceConfig();
    QString partition = config.boot_partition;
    if (partition.isEmpty()) {
         appendOutput("[ERROR] Boot partition not defined in config!", Qt::darkRed);
         updateStatusBar("Flash failed: Config missing.", 5000);
         return;
    }

     // --- Get File Path ---
    // TODO: Get file filter and default dir from config (YAML)
    QString bootPath = QFileDialog::getOpenFileName(this, QString("Select Patched Boot Image (.img) for '%1'").arg(partition), QDir::homePath(), "Image Files (*.img);;All Files (*)");

    if (bootPath.isEmpty()) {
        appendOutput("[*] File selection cancelled.", Qt::gray);
        updateStatusBar("Flash cancelled.", 2000);
        return;
    }

    appendOutput(QString("[*] Selected file: %1").arg(bootPath), Qt::blue);

     // --- Show Confirmation ---
    // TODO: Get warning from config (YAML) if different from generic
     QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Flash Patched Boot",
                                     QString("Are you sure you want to flash '%1' to the '%2' partition?\n\n").arg(QFileInfo(bootPath).fileName(), partition) +
                                     "WARNING: Flashing an incorrect or unpatched boot image can cause boot loops!",
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::No) {
        appendOutput("[*] Patched boot flash cancelled by user.", Qt::gray);
        updateStatusBar("Flash cancelled.", 2000);
        return;
    }

    appendOutput("[*] User confirmed patched boot flash.", Qt::darkYellow);
    updateStatusBar(QString("Flashing %1...").arg(QFileInfo(bootPath).fileName()), 0);


    // --- Send Flash Command ---
    // TODO: Get command template from config (YAML) if needed, or hardcode 'fastboot flash'
    QString program = "fastboot";
    QStringList arguments;
    arguments << "flash" << partition << bootPath;

    currentRunningCommand = "flash_boot";
    mainProcess->start(program, arguments);
    updateButtonStates();
}

// --- FIX: on_rebootSystemButton_clicked implementation (uses config) ---
void AndroidToolWindow::on_rebootSystemButton_clicked()
{
    appendOutput("[*] 'Reboot System' button clicked.", Qt::blue);
    if (mainProcess->state() != QProcess::NotRunning) {
        appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
        return;
    }
     QString program;
     QStringList arguments;
     QString commandId;

     const DeviceConfig& config = getDeviceConfig();

     if (deviceMode == "adb" || deviceMode == "recovery" || deviceMode == "sideload") {
          // Use command from config, fallback to hardcoded
          QString command = config.adb_reboot_system_command;
          if (command.isEmpty()) command = "adb reboot"; // Fallback

          QStringList commandParts = command.split(' ', Qt::SkipEmptyParts);
          if (commandParts.isEmpty()) {
               appendOutput("[ERROR] Invalid reboot system (ADB) command in config!", Qt::darkRed);
               updateStatusBar("Reboot failed: Invalid config.", 5000);
               return;
          }
          program = commandParts.first();
          arguments = commandParts.mid(1);
          commandId = "reboot_adb";

     } else if (deviceMode == "fastboot") {
         // Use command from config, fallback to hardcoded
         QString command = config.fastboot_reboot_command;
         if (command.isEmpty()) command = "fastboot reboot"; // Fallback

          QStringList commandParts = command.split(' ', Qt::SkipEmptyParts);
          if (commandParts.isEmpty()) {
               appendOutput("[ERROR] Invalid reboot system (Fastboot) command in config!", Qt::darkRed);
               updateStatusBar("Reboot failed: Invalid config.", 5000);
               return;
          }
          program = commandParts.first();
          arguments = commandParts.mid(1);
          commandId = "reboot_fastboot";

     } else {
          appendOutput("[WARNING] Reboot System requires ADB, Recovery, Sideload, or Fastboot mode.", Qt::darkYellow);
           updateStatusBar("Reboot failed: Wrong mode.", 5000);
          return;
     }

    currentRunningCommand = commandId;
    mainProcess->start(program, arguments);
    updateButtonStates();
    updateStatusBar("Rebooting system...", 0);
}

// --- FIX: on_rebootRecoveryButton_clicked implementation (uses config) ---
void AndroidToolWindow::on_rebootRecoveryButton_clicked()
{
    appendOutput("[*] 'Reboot Recovery' button clicked.", Qt::blue);
    if (mainProcess->state() != QProcess::NotRunning) {
        appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
        return;
    }
     if (deviceMode == "adb" || deviceMode == "recovery" || deviceMode == "sideload") {
         const DeviceConfig& config = getDeviceConfig();
         QString command = config.adb_reboot_recovery_command;
         if (command.isEmpty()) command = "adb reboot recovery"; // Fallback

         QStringList commandParts = command.split(' ', Qt::SkipEmptyParts);
         if (commandParts.isEmpty()) {
              appendOutput("[ERROR] Invalid reboot recovery command in config!", Qt::darkRed);
              updateStatusBar("Reboot failed: Invalid config.", 5000);
              return;
         }
         QString program = commandParts.first();
         QStringList arguments = commandParts.mid(1);


         currentRunningCommand = "reboot_recovery";
         mainProcess->start(program, arguments);
         updateButtonStates();
         updateStatusBar("Rebooting to recovery...", 0);
     } else {
          appendOutput("[WARNING] Reboot Recovery requires ADB, Recovery, or Sideload mode.", Qt::darkYellow);
          updateStatusBar("Reboot failed: Wrong mode.", 5000);
     }
}

// --- FIX: on_rebootBootloaderButton_clicked implementation (uses config) ---
void AndroidToolWindow::on_rebootBootloaderButton_clicked()
{
    appendOutput("[*] 'Reboot Bootloader' button clicked.", Qt::blue);
    if (mainProcess->state() != QProcess::NotRunning) {
        appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
        return;
    }
     if (deviceMode == "adb" || deviceMode == "recovery" || deviceMode == "sideload") {
         const DeviceConfig& config = getDeviceConfig();
         QString command = config.adb_reboot_bootloader_command;
         if (command.isEmpty()) command = "adb reboot bootloader"; // Fallback

         QStringList commandParts = command.split(' ', Qt::SkipEmptyParts);
         if (commandParts.isEmpty()) {
              appendOutput("[ERROR] Invalid reboot bootloader command in config!", Qt::darkRed);
              updateStatusBar("Reboot failed: Invalid config.", 5000);
              return;
         }
         QString program = commandParts.first();
         QStringList arguments = commandParts.mid(1);

         currentRunningCommand = "reboot_bootloader";
         mainProcess->start(program, arguments);
         updateButtonStates();
         updateStatusBar("Rebooting to bootloader...", 0);
     } else {
          appendOutput("[WARNING] Reboot Bootloader requires ADB, Recovery, or Sideload mode.", Qt::darkYellow);
          updateStatusBar("Reboot failed: Wrong mode.", 5000);
     }
}

void AndroidToolWindow::on_sideloadButton_clicked()
{
    appendOutput("[*] 'ADB Sideload ZIP' button clicked.", Qt::blue);
    if (mainProcess->state() != QProcess::NotRunning) {
        appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
        return;
    }
     if (deviceMode != "sideload") {
         appendOutput("[WARNING] ADB Sideload requires the device to be in Sideload mode.", Qt::darkYellow);
          updateStatusBar("Sideload failed: Wrong mode.", 5000);
         return;
     }

     // --- Get File Path ---
    // TODO: Get file filter and default dir from config (YAML)
     QString zipPath = QFileDialog::getOpenFileName(this, "Select ROM/ZIP File (.zip)", QDir::homePath(), "ZIP Files (*.zip);;All Files (*)");

     if (zipPath.isEmpty()) {
         appendOutput("[*] File selection cancelled.", Qt::gray);
         updateStatusBar("Sideload cancelled.", 2000);
         return;
     }
     appendOutput(QString("[*] Selected file: %1").arg(zipPath), Qt::blue);


     // --- Show Confirmation ---
     // TODO: Get warning message from config (YAML)
     QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm ADB Sideload",
                                      QString("Are you sure you want to sideload '%1'?\n\n").arg(QFileInfo(zipPath).fileName()) +
                                      "Ensure your device is in ADB Sideload mode!",
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

     if (reply == QMessageBox::No) {
         appendOutput("[*] ADB Sideload cancelled by user.", Qt::gray);
          updateStatusBar("Sideload cancelled.", 2000);
         return;
     }
     appendOutput("[*] User confirmed ADB Sideload.", Qt::darkYellow);
     updateStatusBar(QString("Sideloading %1...").arg(QFileInfo(zipPath).fileName()), 0);


     // --- Send Sideload Command ---
     // TODO: Check config for custom sideload command? Usually just "adb sideload"
     QString program = "adb";
     QStringList arguments;
     arguments << "sideload" << zipPath;

     currentRunningCommand = "sideload";
     mainProcess->start(program, arguments);
     updateButtonStates();
}

void AndroidToolWindow::on_adbPushButton_clicked()
{
    appendOutput("[*] 'ADB Push File' button clicked.", Qt::blue);
    if (mainProcess->state() != QProcess::NotRunning) {
        appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
        return;
    }
     if (deviceMode != "adb" && deviceMode != "recovery" && deviceMode != "sideload") {
         appendOutput("[WARNING] ADB Push requires ADB, Recovery, or Sideload mode.", Qt::darkYellow);
         updateStatusBar("Push failed: Wrong mode.", 5000);
         return;
     }

     // --- Get Source File Path (Local) ---
     // TODO: Get file filter and default dir from config (YAML)
     QString sourcePath = QFileDialog::getOpenFileName(this, "Select Source File to Push", QDir::homePath(), "All Files (*)");

     if (sourcePath.isEmpty()) {
         appendOutput("[*] Source file selection cancelled.", Qt::gray);
         updateStatusBar("Push cancelled.", 2000);
         return;
     }
     appendOutput(QString("[*] Selected source file: %1").arg(sourcePath), Qt::blue);

     // --- Get Destination Path (On Device) ---
     // Use QInputDialog to ask for the destination path on the device
     // TODO: Provide example paths or default based on config (YAML)
     bool ok;
     QString destinationPathOnDevice = QInputDialog::getText(this, "ADB Push Destination",
                                         "Enter destination path on device:", QLineEdit::Normal,
                                         "/sdcard/", &ok); // Example default

     if (!ok || destinationPathOnDevice.isEmpty()) {
         appendOutput("[*] Destination path selection cancelled or empty.", Qt::gray);
         updateStatusBar("Push cancelled.", 2000);
         return;
     }
     appendOutput(QString("[*] Destination path on device: %1").arg(destinationPathOnDevice), Qt::blue);


     // --- Show Confirmation (Optional but Recommended for Push) ---
      // TODO: Get warning message from config (YAML)
      QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm ADB Push",
                                      QString("Are you sure you want to push '%1' to '%2' on the device?\n\n").arg(QFileInfo(sourcePath).fileName(), destinationPathOnDevice),
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

     if (reply == QMessageBox::No) {
         appendOutput("[*] ADB Push cancelled by user.", Qt::gray);
         updateStatusBar("Push cancelled.", 2000);
         return;
     }
     appendOutput("[*] User confirmed ADB Push.", Qt::darkYellow);
     updateStatusBar(QString("Pushing %1...").arg(QFileInfo(sourcePath).fileName()), 0);


     // --- Send Push Command ---
     // TODO: Get command from config (YAML) - Assumes 'adb push <source> <destination>' format
     // Check config for custom push command? Usually just "adb push"
     QString program = "adb";
     QStringList arguments;
     arguments << "push" << sourcePath << destinationPathOnDevice;

     currentRunningCommand = "adb_push";
     mainProcess->start(program, arguments);
     updateButtonStates();
}

void AndroidToolWindow::on_adbPullButton_clicked()
{
    appendOutput("[*] 'ADB Pull File' button clicked.", Qt::blue);
    if (mainProcess->state() != QProcess::NotRunning) {
        appendOutput("[WARNING] A command is already running. Please wait.", Qt::darkYellow);
        return;
    }
     if (deviceMode != "adb" && deviceMode != "recovery" && deviceMode != "sideload") {
         appendOutput("[WARNING] ADB Pull requires ADB, Recovery, or Sideload mode.", Qt::darkYellow);
         updateStatusBar("Pull failed: Wrong mode.", 5000);
         return;
     }

     // --- Get Source Path on Device (Input Dialog) ---
     // Use QInputDialog to ask for the source path on the device
     // TODO: Provide example paths or default based on config (YAML)
     bool ok;
     QString sourcePathOnDevice = QInputDialog::getText(this, "ADB Pull Source",
                                         "Enter source path on device:", QLineEdit::Normal,
                                         "/sdcard/mylog.txt", &ok); // Example default

     if (!ok || sourcePathOnDevice.isEmpty()) {
         appendOutput("[*] Source path selection cancelled or empty.", Qt::gray);
         updateStatusBar("Pull cancelled.", 2000);
         return;
     }
     appendOutput(QString("[*] Source path on device: %1").arg(sourcePathOnDevice), Qt::blue);


     // --- Get Destination Local Folder Path ---
     // Use getExistingDirectory to select a folder
     // TODO: Get default dir from config (YAML)
     QString destinationFolderPath = QFileDialog::getExistingDirectory(this, "Select Destination Folder to Save File", QDir::homePath());

     if (destinationFolderPath.isEmpty()) {
         appendOutput("[*] Destination folder selection cancelled.", Qt::gray);
          updateStatusBar("Pull cancelled.", 2000);
         return;
     }
     appendOutput(QString("[*] Selected destination folder: %1").arg(destinationFolderPath), Qt::blue);

     // --- Show Confirmation ---
      // TODO: Get warning message from config (YAML)
      QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm ADB Pull",
                                      QString("Are you sure you want to pull '%1' from the device to '%2' on the device?\n\n").arg(sourcePathOnDevice, destinationFolderPath),
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

     if (reply == QMessageBox::No) {
         appendOutput("[*] ADB Pull cancelled by user.", Qt::gray);
         updateStatusBar("Pull cancelled.", 2000);
         return;
     }
     appendOutput("[*] User confirmed ADB Pull.", Qt::darkYellow);
     QFileInfo sourceFileInfo(sourcePathOnDevice);
     updateStatusBar(QString("Pulling %1...").arg(sourceFileInfo.fileName()), 0);


     // --- Send Pull Command ---
     // TODO: Get command from config (YAML) - Assumes 'adb pull <source> <destination>' format
     // Check config for custom pull command? Usually just "adb pull"
     QString program = "adb";
     QStringList arguments;
     arguments << "pull" << sourcePathOnDevice << destinationFolderPath;

     currentRunningCommand = "adb_pull";
     mainProcess->start(program, arguments);
     updateButtonStates();
}

// --- UI Slots Implementation ---
void AndroidToolWindow::on_clearLogAction_triggered()
{
     outputArea->clear();
     appendOutput("[*] Log cleared.", Qt::gray);
     updateStatusBar("Log cleared.", 2000);
}