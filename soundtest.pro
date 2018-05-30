QT += core gui
QT += widgets
QT += network
QT += multimedia


CONFIG += debug
TARGET = soundtest
TEMPLATE = app

SOURCES += main.cpp mainwindow.cpp \
    logwindow.cpp
HEADERS  += mainwindow.h \
    logwindow.h
