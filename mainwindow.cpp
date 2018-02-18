#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : // То что произойдет в нулевой момент времени после запуска проги
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this); //Хз что это

   /* serial = new QSerialPort(); // переменная для подключения по COM порту

    serial->setDataBits(QSerialPort::Data8);//Значения по мануалу такие
    serial->setStopBits(QSerialPort::OneStop);//Работает не трогай
    serial->setParity(QSerialPort::NoParity);//Если сломается то скорее всего не здесь
    serial->setFlowControl(QSerialPort::HardwareControl);

    serial->setBaudRate(QSerialPort::Baud9600);//дефолт. ЭТО ТРОГАТЬ МОЖНО.*/

    //timer = new QTimer(this);

}

MainWindow::~MainWindow()
{
  //  delete serial;
    delete ui; // чисти, чисти
}
