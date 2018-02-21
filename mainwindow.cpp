#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWidget>
#include <QTime>
#include <math.h>

MainWindow::MainWindow(QWidget *parent) : // То что произойдет в нулевой момент времени после запуска проги
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this); //Хз что это
    serial = new QSerialPort(); // переменная для подключения по COM порту

    // Заполнение ComboBox-a
    QStringList Colours;
    Colours.push_back("Black");
    Colours.push_back("Red");
    Colours.push_back("Green");
    Colours.push_back("Blue");
    ui->comboBox_Colour->clear();
    ui->comboBox_Colour->addItems(Colours);

    ui->comboBoxPortName->clear();// Получение списка доступных портов
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->comboBoxPortName->addItem(info.portName());
    }

    connect(this, SIGNAL(responce(QString)),
            this, SLOT(showResponceData(QString)));

}


QString MainWindow::readDataAction() //Считывание данных из буффера. Если там оказалось что-то чего ты не ожидал увидеть - твои проблемы.
{
    QByteArray temp;

    temp = serial->readAll();
    while(serial->waitForReadyRead(50)) {
        temp += serial->readAll();
    }

    return QString(temp);
}

void MainWindow::sendDataAction(QString data)//Отправка данных пациенту. Ну или запросов. Ну или порнографии. Мало ли на что у меня совести хватит.
{
    serial->write(data.toLocal8Bit());
    return;
}

void MainWindow::showResponceData(QString data)
{
    ui->textLineResponce->setText(data);
}


void MainWindow::on_pushButton_Connect_TC_clicked()
{

    serial = new QSerialPort(); // переменная для подключения по COM порту


    serial->setDataBits(QSerialPort::Data8);//Значения по мануалу такие
    serial->setStopBits(QSerialPort::OneStop);//Работает не трогай
    serial->setParity(QSerialPort::NoParity);//Если сломается то скорее всего не здесь
    serial->setFlowControl(QSerialPort::HardwareControl);

    serial->setBaudRate(QSerialPort::Baud9600);//дефолт. ЭТО ТРОГАТЬ МОЖНО.
    serial->setPortName(ui->comboBoxPortName->currentText()); //пока что выбирать порт будем лапками


    if (!serial->open(QIODevice::ReadWrite)) //попытка подключится с дефолтными параметрами
    {
        QSerialPort::SerialPortError getError = QSerialPort::NoError;
        serial->error(getError);
        return;
    }
    QString query = "*IDN?\n";
    QString answer;
    answer = readDataAction();
    emit responce(answer);

    sendDataAction(query);
    answer = readDataAction();
    if(!answer.contains("Stanford"))
    {
        scanBauds();
    }
    sendDataAction(query);
    answer = readDataAction();
    emit responce(answer);
}

void MainWindow::scanBauds()
{
    QList<qint32> bauds = QSerialPortInfo::standardBaudRates();
    QString query = "*IDN?\n";
    QString answer;

    foreach(qint32 baud, bauds)
    {
        serial->setBaudRate(baud);
        serial->open(QIODevice::ReadWrite);
        sendDataAction(query);
        answer = readDataAction();
        if((answer.contains("Stanford")) or (answer.contains("Error")))
        {
            return;
        }
    }
    return;
}



MainWindow::~MainWindow()
{
     //delete timer;

    serial->close();
    delete serial;
    delete ui; // чисти, чисти
}
