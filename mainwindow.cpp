#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : // То что произойдет в нулевой момент времени после запуска проги
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this); //Хз что это

    serial = new QSerialPort(); // переменная для подключения по COM порту

    serial->setDataBits(QSerialPort::Data8);//Значения по мануалу такие
    serial->setStopBits(QSerialPort::OneStop);//Работает не трогай
    serial->setParity(QSerialPort::NoParity);//Если сломается то скорее всего не здесь
    serial->setFlowControl(QSerialPort::HardwareControl);

    serial->setBaudRate(QSerialPort::Baud9600);//дефолт. ЭТО ТРОГАТЬ МОЖНО.




    // Заполнение ComboBox-a
    QStringList TC_Channels;
    TC_Channels.push_back("3A");
    TC_Channels.push_back("3B");
    TC_Channels.push_back("3C");
    TC_Channels.push_back("3D");
    ui->comboBoxTC_Channel->addItems(TC_Channels);

    // Заполнение ComboBox-a
    QStringList Colours;
    Colours.push_back("black");
    Colours.push_back("red");
    Colours.push_back("green");
    Colours.push_back("blue");
    ui->comboBox_Colour->addItems(Colours);


}

MainWindow::~MainWindow()
{
    delete serial;
    delete ui; // чисти, чисти
}
