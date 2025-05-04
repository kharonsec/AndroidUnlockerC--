# Enhanced Android Bootloader & Recovery Tool (C++ Qt)

## Project Description

This project is a graphical user interface (GUI) tool designed to simplify common Android device operations related to the bootloader, recovery, and file transfer. It wraps command-line tools like ADB (Android Debug Bridge) and Fastboot, providing a user-friendly interface.

This is a C++ implementation utilizing the cross-platform **Qt 5** framework for the GUI and **yaml-cpp** for configuration loading.

**Please Note:** This project is a work in progress developed as an example to demonstrate C++ and Qt programming for interacting with external command-line tools. Some advanced features or edge cases may not be fully implemented or tested.

## Features

* Automatic detection of connected Android devices in various modes (ADB, Fastboot, Recovery, Sideload).
* Dynamic enabling/disabling of GUI buttons based on the detected device mode.
* Display of detected device serial number.
* Attempted detection of device Manufacturer and Model using ADB (`getprop`) and Fastboot (`getvar`).
* Loading device-specific or default configurations from a `devices.yaml` file.
* Core operations via ADB and Fastboot:
    * Unlock Bootloader (Fastboot, requires confirmation).
    * Lock Bootloader (Fastboot, requires confirmation).
    * Flash Recovery image (Fastboot, requires file selection and confirmation).
    * Flash Patched Boot image (Fastboot, requires file selection and confirmation).
    * ADB Sideload ZIP (Sideload mode, requires file selection and confirmation).
    * ADB Push File (ADB/Recovery/Sideload mode, requires local source file selection, device destination input, and confirmation).
    * ADB Pull File (ADB/Recovery/Sideload mode, requires device source input, local destination folder selection, and confirmation).
    * Reboot device to System, Recovery, or Bootloader (requires appropriate ADB/Fastboot mode).
* Real-time command output display in the GUI.
* Status bar providing feedback on ongoing operations.
* Clear log output option (via right-click context menu).

## Prerequisites

Before building and running this application, you need to have the following installed on your system:

1.  **C++ Compiler:** A standard C++ compiler like g++ (GCC), Clang, or MSVC.
2.  **Qt 5 Development Kit:** The Qt 5 libraries, headers, and build tools (qmake or CMake).
    * On Debian/Ubuntu: `sudo apt update && sudo apt install qtbase5-dev qtchooser` (and optionally `qtcreator` for an IDE).
    * Follow instructions for your specific OS if not Debian/Ubuntu.
3.  **yaml-cpp Development Library:** The `yaml-cpp` library headers and compiled library files.
    * On Debian/Ubuntu: `sudo apt install libyaml-cpp-dev`
    * Follow instructions for your specific OS if not Debian/Ubuntu.
4.  **Android SDK Platform-Tools (ADB & Fastboot):** The `adb` and `fastboot` executables must be installed and accessible in your system's PATH environment variable.
5.  **Android Device Drivers:** Ensure your computer has the necessary drivers to communicate with your Android device in ADB, Fastboot, and Sideload modes.

## Building

This project uses `qmake` as the build system.

1.  Navigate to the project directory in your terminal:
    ```bash
    cd /path/to/your/AndroidUnlockerC++
    ```

2.  **Configure the build project (`.pro` file) for yaml-cpp:** Edit the `androidtool.pro` file and add the necessary `INCLUDEPATH` and `LIBS` lines for your `yaml-cpp` installation.

    ```pro
    # androidtool.pro
    QT += widgets

    SOURCES = main.cpp \
              androidtoolwindow.cpp

    HEADERS = androidtoolwindow.h

    # --- yaml-cpp Configuration ---
    # Add the include path to your yaml-cpp headers (where the 'yaml-cpp' directory is)
    # You might need to adjust this path based on where libyaml-cpp-dev installed headers
    INCLUDEPATH += /usr/include # Common path if headers are in /usr/include/yaml-cpp/...
    # OR
    # INCLUDEPATH += /usr/local/include # Common path if headers are in /usr/local/include/yaml-cpp/...
    # OR a more specific path if needed

    # Add the linker flag for the yaml-cpp library
    LIBS += -lyaml-cpp
    # --- End yaml-cpp Configuration ---

    CONFIG += c++11 # Or c++14, c++17
    ```
    * **Find your `yaml-cpp` include path:** Use `dpkg -L libyaml-cpp-dev` on Debian/Ubuntu to see where the headers were installed (look for `/usr/include/yaml-cpp/` or similar). Your `INCLUDEPATH` should be the directory *containing* the `yaml-cpp` folder.

3.  Run `qmake` to generate the Makefile:
    ```bash
    qmake
    ```

4.  Run `make` to compile and link the application:
    ```bash
    make
    ```
    * On Windows with MinGW or MSYS2, use `make`.
    * On Windows with MSVC (Visual Studio command prompt), use `nmake`.

5.  If the build is successful, an executable file named `androidtool` (or `androidtool.exe` on Windows) will be created in the project directory.

## Usage

1.  Place your `devices.yaml` configuration file in the same directory as the compiled `androidtool` executable. See the Configuration section for an example.
2.  Run the executable from your terminal or file explorer:
    ```bash
    ./androidtool
    ```
3.  Connect your Android device to your computer.
4.  The tool will periodically check for connected devices and update the "Mode" label.
5.  Ensure your device is in the correct mode (ADB, Fastboot, Recovery) for the operation you wish to perform. The buttons will enable/disable accordingly.
6.  Click the appropriate buttons to perform tasks. Follow any prompts for file selection or confirmation.
7.  Observe the command output and status bar for feedback.

## Configuration (`devices.yaml`)

The tool reads commands, partition names, and warnings from a `devices.yaml` file located in the same directory as the executable. This allows customization for different devices.

```yaml
# devices.yaml - Example Configuration File

# Default configuration (used if no specific device matches)
default:
  # Commands (basic examples - adjust as needed for your device)
  unlock_command: "fastboot flashing unlock" # Common command
  lock_command: "fastboot flashing lock"   # Common command
  adb_reboot_bootloader_command: "adb reboot bootloader"
  adb_reboot_recovery_command: "adb reboot recovery"
  adb_reboot_system_command: "adb reboot"
  fastboot_reboot_command: "fastboot reboot"
  # ADB Push/Pull/Sideload commands usually don't need custom config,
  # but you could add them here if a device needed special arguments:
  # adb_sideload_command: "adb sideload"
  # adb_push_command: "adb push"
  # adb_pull_command: "adb pull"


  # Partition names (common names)
  recovery_partition: "recovery"
  boot_partition: "boot"
  # Add other partitions if needed, e.g., "dtbo", "vendor_boot"

  # Warning messages for risky operations
  unlock_warning: "Unlocking the bootloader will wipe all user data (factory reset) and may void your warranty! Proceed with caution."
  lock_warning: "Locking the bootloader will wipe all user data. Ensure you have backups before proceeding."

  # Notes about this configuration
  notes: "This is the default configuration. Device-specific commands may differ."

# Example configuration for a specific device (Manufacturer Model)
# The tool tries to match "manufacturer model" (case-insensitive)
# If model is unknown, it tries just "manufacturer"
# If no match, it falls back to 'default'
"Google Pixel 5": # Replace with your device's Manufacturer and Model
  # Override default commands/partitions/warnings for this specific device
  unlock_command: "fastboot flashing unlock" # Example: Pixel uses 'flashing' command
  lock_command: "fastboot flashing lock"
  recovery_partition: "boot" # Example: Some devices flash recovery to the boot partition (A/B slots)
  boot_partition: "boot" # Example: Still 'boot' for patched boot

  unlock_warning: "Unlocking your Pixel will wipe data and might affect SafetyNet. Proceed?" # Custom warning

  notes: "Configuration for Google Pixel 5."

# Add more device entries as needed (Manufacturer Model: or Manufacturer:)
# "OnePlus":
#   unlock_command: "fastboot oem unlock"
#   lock_command: "fastboot oem lock"
#   # Inherits other settings from 'default'
#   notes: "Configuration for OnePlus devices (using older oem commands)."

Known Issues / TODO
Multi-device handling is not implemented (tool operates on the first detected device).
Advanced error message parsing is basic; many ADB/Fastboot errors will only show raw output.
Detailed progress reporting (e.g., percentages) is not implemented for most commands.
Cancellation of running commands is not implemented.
File dialog default directories and filters are not yet configurable via devices.yaml.
Manufacturer/Model detection might not work for all devices depending on available ADB/Fastboot properties.
No dedicated installer or packaging is provided; requires manual compilation.
Licensing
This project is licensed under the MIT License. See the LICENSE file for details.
(Note: Create a LICENSE file in your repository with the MIT license text)

Credits
Developed by [Your Name/Alias, e.g., Eliott] with assistance from Google Gemini.


---

**Next Steps After Creating the README:**

1.  **Create `devices.yaml`:** Create the actual `devices.yaml` file in your project directory with the example content (and modify it for your specific device if you know the commands).
2.  **Create `LICENSE` file:** Create a file named `LICENSE` in your project directory and paste the text of the MIT License (or your chosen license) into it. You can easily find MIT license text templates online.
3.  **Place in Repository:** If you're using a Git repository (like on GitHub, GitLab, etc.), place all the source files (`.cpp`, `.h`, `.pro`), the `devices.yaml`, and the `LICENSE` file in the root of the repository. The README.md will be automatically displayed on the repository page.
4.  **Continue Developing:** Use the "Known Issues / TODO" list as a starting point for adding more features to your C++ application.
