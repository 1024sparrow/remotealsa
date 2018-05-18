QT += core gui
QT += widgets
QT += network
QT += multimedia


TARGET = soundtest
TEMPLATE = app

SOURCES += main.cpp mainwindow.cpp \
    logwindow.cpp
HEADERS  += mainwindow.h \
    logwindow.h
