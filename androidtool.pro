QT += widgets

SOURCES = main.cpp \
          androidtoolwindow.cpp

HEADERS = androidtoolwindow.h

INCLUDEPATH += /usr/include # Example path if yaml-cpp headers are directly in /usr/include
# OR
# INCLUDEPATH += /usr/local/include # Example path if installed via source to /usr/local
# OR specific yaml-cpp paths if headers are deeper:
# INCLUDEPATH += /usr/include/yaml-cpp # Example if headers are in /usr/include/yaml-cpp/yaml-cpp/...

LIBS += -lyaml-cpp # <-- Make sure this line is also present

CONFIG += c++11