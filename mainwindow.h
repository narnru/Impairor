#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTime>
#include <QFile>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    QTime time;
    QSerialPort *serial;
    int additionalWaitTime = 50; //Надо добавить калибровку
    QFile log_file;
    double timeStart = 0;
    int checkBox_1_first = 1;
    bool run = false;

private slots:
    void on_pushButton_Connect_TC_clicked();
    void on_actionUpdate_available_ports_triggered();
    void on_pushButton_Recieve_clicked();
    void on_pushButton_Send_clicked();
    void scanBauds();
    QString readDataAction();
    void sendDataAction(QString data);
    void showResponceData(const QString data);
    void on_checkBox_1_toggled(bool checked);
    void Plot();
<<<<<<< HEAD
    void ReadNames(); //1. нужно ли делать слотом? void?  аргументы?
=======
    void on_actionCalibrate_wait_time_triggered();
>>>>>>> c242b9992d0b0cccb81fddf6b4ce18027a98d6bd


signals:
    void responce(const QString data);


public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();


private:
    Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
