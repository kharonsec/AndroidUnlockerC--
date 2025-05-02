#ifndef ANDROIDTOOLWINDOW_H
#define ANDROIDTOOLWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QProcess> // Include QProcess
#include <QTimer>
#include <QColor>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QStatusBar>
#include <QPair>
#include <QMap>
#include <QFile>
#include <QTextStream>

#include <yaml-cpp/yaml.h>

// --- Configuration Data Structures ---
struct DeviceConfig {
    // Commands
    QString unlock_command;
    QString lock_command;
    QString adb_reboot_bootloader_command;
    QString adb_reboot_recovery_command;
    QString adb_reboot_system_command;
    QString fastboot_reboot_command;

    // Partitions
    QString recovery_partition;
    QString boot_partition;

    // Warnings
    QString unlock_warning;
    QString lock_warning;

    // Other notes from YAML
    QString notes;

    DeviceConfig() = default;
};

struct FullConfig {
    QMap<QString, DeviceConfig> devices;
    DeviceConfig default_config;

    FullConfig() = default;
};


class AndroidToolWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AndroidToolWindow(QWidget *parent = nullptr);
    ~AndroidToolWindow();

private slots:
    // --- Button Slots ---
    void on_detectButton_clicked();
    void on_unlockButton_clicked();
    void on_lockButton_clicked();
    void on_recoveryButton_clicked();
    void on_magiskButton_clicked();
    void on_rebootSystemButton_clicked();
    void on_rebootRecoveryButton_clicked();
    void on_rebootBootloaderButton_clicked();
    void on_sideloadButton_clicked();
    void on_adbPushButton_clicked();
    void on_adbPullButton_clicked();
    void on_cancelButton_clicked(); // <-- New Slot for Cancel Button

    // --- Main QProcess Slots ---
    void processReadyReadStandardOutput();
    void processReadyReadStandardError();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processErrorOccurred(QProcess::ProcessError error);
    void on_mainProcess_started(); // <-- New Slot for Process Start

    // --- State Check QProcess Slots ---
    void stateCheckProcessReadyReadStandardOutput();
    void stateCheckProcessReadyReadStandardError();
    void stateCheckProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void stateCheckProcessErrorOccurred(QProcess::ProcessError error);

    // --- Timer Slot ---
    void checkDeviceState();

    // --- UI Slots ---
    void on_clearLogAction_triggered();

private:
    // --- UI Elements ---
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QHBoxLayout *infoLayout;
    QHBoxLayout *buttonLayoutMain;
    QHBoxLayout *buttonLayoutReboot;
    QHBoxLayout *buttonLayoutFile;

    QLabel *manufacturerLabel;
    QLabel *modelLabel;
    QLabel *serialLabel;
    QLabel *modeLabel;

    QTextEdit *outputArea;
    QAction *clearLogAction;

    QPushButton *detectButton;
    QPushButton *unlockButton;
    QPushButton *lockButton;
    QPushButton *recoveryButton;
    QPushButton *magiskButton;
    QPushButton *rebootSystemButton;
    QPushButton *rebootRecoveryButton;
    QPushButton *rebootBootloaderButton;
    QPushButton *sideloadButton;
    QPushButton *adbPushButton;
    QPushButton *adbPullButton;
    QPushButton *cancelButton; // <-- New Cancel Button Member


    // --- Processes ---
    QProcess *mainProcess;
    QProcess *stateCheckProcess;

    // --- Timer ---
    QTimer *stateTimer;

    // --- State Variables ---
    QString deviceMode;
    QString deviceSerialAdb;
    QString deviceSerialFastboot;
    QString deviceManufacturer;
    QString deviceModel;

    QString currentRunningCommand; // Identifier for mainProcess

    // --- State Check Temp Variables ---
    QString adbDetectedMode;
    QString adbDetectedSerial;
    bool adbCheckCompleted;

    // --- Configuration ---
    FullConfig loadedConfig;

    // --- Helper Functions ---
    void appendOutput(const QString &text, const QColor &color = Qt::black);
    void updateDeviceMode(const QString& mode);
    void updateDeviceSerial(const QString& serial, bool isFastboot);
    void updateDeviceDetailsLabels();
    void updateButtonStates(); // Modified to handle cancel button
    void updateStatusBar(const QString& message, int timeout = 0);

    // --- State Check Logic Helpers ---
    void runAdbStateCheck();
    void runFastbootStateCheck();
    QPair<QString, QString> parseAdbDevicesOutput(const QString& output);
    QPair<bool, QString> parseFastbootDevicesOutput(const QString& output);

    // --- Detection Logic Helpers ---
    void runAdbPropertiesCheck();
    void runFastbootVariablesCheck();
    void parseAdbPropertiesOutput(const QString& output);
    void parseFastbootVariablesOutput(const QString& output);

    // --- YAML Configuration Logic ---
    void loadConfig(const QString& filePath);
    DeviceConfig getDeviceConfig() const;
};

#endif // ANDROIDTOOLWINDOW_H