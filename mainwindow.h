#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTime>
#include <QFile>
#include <QTest>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    QTime time;
    QSerialPort *serial;
    int additionalWaitTime = 50;
    QFile log_file;
    QFile reserve_file;
    double timeStart = 0;
    int checkBox_1_first = 0;
    int checkBox_2_first = 0;
    int start = 0;
    bool run = false;
    QStringList NameList;
    QStringList UnitList;
    double currentTime = 0;
    int index_1, index_2, index_3, index_4, index_5;

private slots:
    void on_pushButton_Connect_TC_clicked();
    void on_actionUpdate_available_ports_triggered();
    void on_pushButton_Recieve_clicked();
    void on_pushButton_Send_clicked();
    void scanBauds();
    QString readDataAction();
    void sendDataAction(QString data);
    void showResponceData(const QString data);
    void Plot();
    void ReadNames();
    void ReadUnits();
    void on_actionCalibrate_wait_time_triggered();

    void pid_Scan();

    void on_pushButton_Start_Power_clicked();

    void on_pushButton_Start_PID_clicked();

    void on_pushButton_Plot_clicked();

    void on_checkBox_1_clicked();

    void SetColour (QString colour, const int n, QString index);

    void on_pushButton_Check_clicked();

signals:
    void responce(const QString data);


public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();


private:
    Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
