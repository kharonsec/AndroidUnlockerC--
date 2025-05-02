#include <QApplication>
#include "androidtoolwidget.h" // We will define our main widget here

int main(int argc, char *argv[])
{
    // Create the QApplication instance
    QApplication app(argc, argv);

    // Create the main widget
    AndroidToolWidget window;

    // Set the window title
    window.setWindowTitle("Basic Android Tool (C++ Qt Example)");

    // Show the window
    window.show();

    // Start the application event loop
    return app.exec();
}