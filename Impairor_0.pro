#-------------------------------------------------
#
# Project created by QtCreator 2018-02-13T23:22:45
#
#-------------------------------------------------

QT          += core gui serialport testlib

QT.testlib.CONFIG -= console

CONFIG   += c++11

RC_FILE = Rc.rc

QMAKE_LFLAGS += -static -static-libgcc

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = Impairor_0
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qcustomplot.cpp

HEADERS  += mainwindow.h \
    qcustomplot.h

FORMS    += mainwindow.ui
