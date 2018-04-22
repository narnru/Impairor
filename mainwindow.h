#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ptc10.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTime>
#include <QFile>
#include <QTest>
#include <QWidget>
#include <math.h>
#include <iostream>
#include <QDir>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    int additionalWaitTime = 50;
    QSerialPort *serial;
    QFile *log_file;
    QFile *reserve_file;
    double timeStart = 0;
    int checkBox_1_first = 0;
    int checkBox_2_first = 0;
    int start = 0;
    bool run = false;
    QStringList NameList;
    QStringList UnitList;
    QStringList PidStatus;
    double currentTime = 0;
    int index_1, index_2, index_3, index_4, index_5;
    PTC10 *device;
    QList <int> plotIndexList;
    QThread *tread;


private slots:
    void on_pushButton_Connect_TC_clicked();
    void on_actionUpdate_available_ports_triggered();
    void on_pushButton_Recieve_clicked();
    void on_pushButton_Send_clicked();
//    QString readDataAction();
//    void sendDataAction(QString data);
    void showResponceData(const QString data);
//    void Plot();
    void ReadNames(QStringList nameList);
    void ReadUnits(QStringList unitList, QStringList outputList);
    void on_actionCalibrate_wait_time_triggered();
    void pid_Scan(QStringList pidStatus);
    void on_pushButton_Start_Power_clicked();
    void on_pushButton_Start_PID_clicked();
    void on_pushButton_Plot_clicked();
    void on_checkBox_1_clicked();
    void on_checkBox_2_clicked();
    void on_checkBox_3_clicked();
    void on_checkBox_4_clicked();
    void on_checkBox_5_clicked();
    void SetColour (QString colour, const int n, QString index);
    void on_pushButton_Check_clicked();
    void on_checkBox_fixPlot_T_clicked();
    void on_checkBox_fixPlot_P_clicked();
    void on_pushButton_Export_clicked();
    void closeEvent(QCloseEvent *event);
    void gotConnected();
    void addDataToGraphT(const int index, double value, double time);
    void addDataToGraphP(const int index, double value, double time);
    void updateGraphs();
    void plotHadStopped();

signals:
    void responce(const QString data);
    void sendIndexList(QList <int> indexList);
    void finishIt();
    void requestForCalibrateWaitTime();
    void requestForSendAndRead(QString data);
    void requestForConnect(QString);
    void requestForDisconnect();
    void requestForPowerStart(QString output, float power);
    void requestForPIDStart(QString output, QStringList pidStatus);
    void requestForPlotStart();
    void requestForPlotStop();
    void requestForPIDScan(QString output);
    void requestForExport(QString Name);

public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();


private:
    Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
