#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTime>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    QTime time;
    QSerialPort *serial;    

private slots:
    void on_pushButton_Connect_TC_clicked();

    void scanBauds();

    QString readDataAction();
    void sendDataAction(QString data);

public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();


private:
    Ui::MainWindow *ui;

};

#endif // MAINWINDOW_H
