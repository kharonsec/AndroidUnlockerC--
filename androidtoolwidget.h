#ifndef ANDROIDTOOLWIDGET_H
#define ANDROIDTOOLWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QProcess> // For running external commands
#include <QTimer>   // For periodic tasks
#include <QColor>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QScrollBar> // For auto-scrolling QTextEdit
#include <QAbstractScrollArea> // Required for verticalScrollBar()

class AndroidToolWidget : public QWidget
{
    Q_OBJECT // Macro needed for Signals and Slots

public:
    explicit AndroidToolWidget(QWidget *parent = nullptr);
    ~AndroidToolWidget(); // Destructor

private slots:
    // --- Button Slots ---
    void on_detectButton_clicked();
    void on_unlockButton_clicked();
    void on_lockButton_clicked();
    void on_recoveryButton_clicked();
    void on_magiskButton_clicked();
    void on_rebootSystemButton_clicked();
    void on_rebootRecoveryButton_clicked();
    void on_rebootBootloaderButton_clicked(); // Implement this one
    void on_sideloadButton_clicked();
    void on_adbPushButton_clicked(); // Placeholder
    void on_adbPullButton_clicked(); // Placeholder

    // --- Main QProcess Slots ---
    void processReadyReadStandardOutput();
    void processReadyReadStandardError();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processErrorOccurred(QProcess::ProcessError error);

    // --- State Check QProcess Slots ---
    void stateCheckProcessReadyReadStandardOutput(); // Might not use for parsing, but good practice
    void stateCheckProcessReadyReadStandardError(); // Might not use for parsing
    void stateCheckProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void stateCheckProcessErrorOccurred(QProcess::ProcessError error); // Handle state check errors

    // --- Timer Slot ---
    void checkDeviceState();

private:
    // --- UI Elements ---
    QVBoxLayout *mainLayout;
    QHBoxLayout *infoLayout;
    QHBoxLayout *buttonLayoutMain; // New layout for main buttons
    QHBoxLayout *buttonLayoutReboot; // New layout for reboot buttons
    QHBoxLayout *buttonLayoutFile;   // New layout for file buttons

    QLabel *manufacturerLabel;
    QLabel *modelLabel;
    QLabel *serialLabel; // Added serial label
    QLabel *modeLabel;

    QTextEdit *outputArea;

    QPushButton *detectButton;
    QPushButton *unlockButton;      // Added
    QPushButton *lockButton;        // Added
    QPushButton *recoveryButton;    // Added
    QPushButton *magiskButton;      // Added
    QPushButton *rebootSystemButton;  // Added
    QPushButton *rebootRecoveryButton; // Added
    QPushButton *rebootBootloaderButton; // Added
    QPushButton *sideloadButton;    // Added
    QPushButton *adbPushButton;     // Added
    QPushButton *adbPullButton;     // Added


    // --- Processes ---
    QProcess *mainProcess; // Used for main actions (flash, unlock, etc.)
    QProcess *stateCheckProcess; // Used specifically for 'adb devices' and 'fastboot devices' checks

    // --- Timer ---
    QTimer *stateTimer;

    // --- State Variables ---
    QString deviceMode; // 'none', 'adb', 'fastboot', 'recovery', 'sideload'
    QString deviceSerialAdb;
    QString deviceSerialFastboot;
    // TODO: Add manufacturer, model members if needed

    // --- Helper Functions ---
    void appendOutput(const QString &text, const QColor &color = Qt::black); // Use QColor for flexibility
    void updateDeviceMode(const QString& mode);
    void updateDeviceSerial(const QString& serial, bool isFastboot);
    void updateButtonStates(); // Enable/disable buttons based on deviceMode

    // Helper function to start a process asynchronously
    // bool startProcess(QProcess* process, const QString& program, const QStringList& arguments);

    // --- State Check Logic Helpers ---
    void runAdbStateCheck();
    void runFastbootStateCheck();
    void parseAdbDevicesOutput(const QString& output);
    void parseFastbootDevicesOutput(const QString& output);
};

#endif // ANDROIDTOOLWIDGET_H