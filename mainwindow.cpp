#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : // То что произойдет в нулевой момент времени после запуска проги
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this); //Хз что это
    serial = new QSerialPort(); // переменная для подключения по COM порту
    //timer = new QTimer(this); // таймер

    // Заполнение ComboBox-a
    QStringList Colours;
    Colours.push_back("black");
    Colours.push_back("red");
    Colours.push_back("green");
    Colours.push_back("blue");
    ui->comboBox_Colour->clear();
    ui->comboBox_Colour->addItems(Colours);

    on_actionUpdate_available_ports_triggered();// Получение списка доступных портов


    connect(this, SIGNAL(responce(QString)),
            this, SLOT(showResponceData(QString)));
}

void MainWindow::on_actionUpdate_available_ports_triggered()
{
    ui->comboBoxPortName->clear();

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->comboBoxPortName->addItem(info.portName());
    }
    return;
}

QString MainWindow::readDataAction() //Считывание данных из буффера. Если там оказалось что-то чего ты не ожидал увидеть - твои проблемы.
{
    QByteArray temp;

    temp = serial->readAll();
    if(serial->waitForReadyRead(50)) {
        temp += serial->readAll();
        while(serial->waitForReadyRead(additionalWaitTime))
        {
            temp += serial->readAll();
        }
    }

    return QString(temp);
}

void MainWindow::sendDataAction(QString data)//Отправка данных пациенту. Ну или запросов. Ну или порнографии. Мало ли на что у меня совести хватит.
{
    serial->write(data.toLocal8Bit());
    return;
}

void MainWindow::showResponceData(QString data) // Слот для отображения чего нибудь в строчку responce
{
    ui->textLineResponce->setText(data);
}


void MainWindow::on_pushButton_Connect_TC_clicked()
{
    if(ui->pushButton_Connect_TC->text() == "Connect")
        {
        serial->setDataBits(QSerialPort::Data8);//Значения по мануалу такие
        serial->setStopBits(QSerialPort::OneStop);//Работает не трогай
        serial->setParity(QSerialPort::NoParity);//Если сломается то скорее всего не здесь
        serial->setFlowControl(QSerialPort::HardwareControl);

        serial->setBaudRate(QSerialPort::Baud9600);//дефолт. ЭТО ТРОГАТЬ МОЖНО.
        serial->setPortName(ui->comboBoxPortName->currentText()); //пока что выбирать порт будем лапками


        if (!serial->open(QIODevice::ReadWrite)) //попытка подключится с дефолтными параметрами
        {
            QSerialPort::SerialPortError getError = QSerialPort::NoError;
            getError = serial->error();
            if (getError == QSerialPort::DeviceNotFoundError)
            {
                emit responce("Device not found");
            } else
            {
                emit responce ("SerialPort error number " + QString::number(getError));
            }
            return; // если попытка подключения умерла на взлете
        }

        QString query = "*IDN?\n";  //команда запроса прибору от Stanford Research Systems чтобы узнать что он такое
        QString answer;
    //    answer = readDataAction(); //проверка что в буффере нет лишнего
    //    emit responce(answer); //Если есть то покажи интересно жи

        sendDataAction(query); // запрос прибору...
        answer = readDataAction(); //попытка получить ответ

        if(!answer.contains("Stanford")) // если ответ не такой как хотелось бы то попробуем перебрать измениямые внутри прибора характеристики
        {
            scanBauds();
            sendDataAction(query);
            answer = readDataAction();
        }

        if(!answer.contains("PTC10"))  // финальная проверка тот ли ответ
        {
            if(answer.contains("Stanford"))
            {
                emit responce("It's another Stanford device");
            }else
            {
                emit responce("Unknown Device");
            }
            serial->close();
            return;
        } else
        {
            ui->pushButton_Connect_TC->setText("Disconnect");
            return;
        }
    } else //Дисконнект
    {
        serial->close();
        ui->pushButton_Connect_TC->setText("Connect");
        return;
    }
    return; //Я конечно не знаю как сюда можно попасть но пусть будет.
}

void MainWindow::scanBauds() // функция для перебора всех возможных baudrate, будет смешно если она случайно подключится не к стэндфордскому прибору.
{
    QList<qint32> bauds = QSerialPortInfo::standardBaudRates(); //костыль ибо мне лень делать нормальный список
    bauds.append(qint32(230400)); //это извращение увидел в настройках контроллера

    QString query = "*IDN?\n";
    QString answer;

    foreach(qint32 baud, bauds) //для всех возможных baud попробуем получить информацию о приборе
    {
        serial->setBaudRate(baud);
        if (serial->open(QIODevice::ReadWrite))
        {
            sendDataAction(query);
            answer = readDataAction();

            if((answer.contains("Stanford")) or (answer.contains("Error"))) //если ответ нормальный или что более вероятно нормальный но не тот то выйдем из сканирования
            {
                return;
            }
        }
    }
    return; //если не получилось то ну и ладно. Тут по хорошему нужно бы сделать возврат ошибки но мне лень
}



MainWindow::~MainWindow()
{
     //delete timer;

    serial->close();
    delete serial;
    delete ui; // чисти, чисти
}
