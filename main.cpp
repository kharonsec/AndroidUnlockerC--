#include <QApplication>
#include "androidtoolwindow.h" // Changed to reflect QMainWindow
#include <QDebug> // Added for qWarning()

int main(int argc, char *argv[])
{
    // Create the QApplication instance
    QApplication app(argc, argv);

    // Apply Material Design theme *after* QApplication is created
    // Note: apply_stylesheet is from the qt_material library (Python), not standard C++.
    // If you are using a C++ equivalent, you would include its header and call its function here.
    // For this C++ example, we rely on standard Qt styling unless a C++ styling library is integrated.
    try {
        // Assuming no C++ qt_material for this example
    } catch (const std::exception& e) {
        qWarning() << "Could not apply stylesheet:" << e.what();
    } catch (...) {
        qWarning() << "An unknown error occurred while applying stylesheet.";
    }

    AndroidToolWindow window; // Use the new QMainWindow class

    window.setWindowTitle("Enhanced Android Bootloader & Recovery Tool (C++ Qt)");
    window.show();

    // Start the application event loop
    return app.exec();
}