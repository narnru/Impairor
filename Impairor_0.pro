#-------------------------------------------------
#
# Project created by QtCreator 2018-02-13T23:22:45
#
#-------------------------------------------------

QT          += core gui serialport
CONFIG   += c++11


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = Impairor_0
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qcustomplot.cpp

HEADERS  += mainwindow.h \
    qcustomplot.h

FORMS    += mainwindow.ui
