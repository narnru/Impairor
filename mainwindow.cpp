#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWidget>
#include <QTime>
#include <math.h>
#include <iostream>

MainWindow::MainWindow(QWidget *parent) : // То что произойдет в нулевой момент времени при создании окошка
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

    on_actionUpdate_available_ports_triggered();// Получение списка доступных портов

    connect(this, SIGNAL(responce(QString)),
            this, SLOT(showResponceData(QString)));

    log_file.setFileName("log.txt"); //Создание лог файла
    if(!log_file.open(QIODevice::ReadWrite))
    {
        ui->textLineResponce->setText("Log file wrecked");
    }
    log_file.readAll(); // перемещение текущей позиции в конец файла
    log_file.write(QTime::currentTime().toString().toLocal8Bit()); // начальная строчка с указанем текущего времени(надо сделать еще и дату)
    log_file.write(" \n");

    ui->widget_T->xAxis->setLabel("Time"); // Оси графика для температуры
    ui->widget_T->yAxis->setLabel("Value");
    ui->widget_T->yAxis->setRange(-1.5, 1.5); // временно
    ui->widget_T->clearGraphs();
    ui->widget_T->addGraph();
}


void MainWindow::on_actionUpdate_available_ports_triggered()// кнопачка чтобы обновить список доступных портов
{
    ui->comboBoxPortName->clear();

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) // запрос компу про доступные порты
    {
        ui->comboBoxPortName->addItem(info.portName());
    }
    return;
}

void MainWindow::on_actionCalibrate_wait_time_triggered()// кнопачка чтобы попытаться откалибровать время
{
    if(serial->isOpen())
    {
        QString query = "*IDN?\n";
        QString fullAnswer;
        QString answer;

        readDataAction();
        sendDataAction(query);
        fullAnswer = readDataAction();
        answer = fullAnswer;

        while(answer  == fullAnswer)
        {
            sendDataAction(query);
            answer = readDataAction();
            additionalWaitTime -= 1;

            if (additionalWaitTime == 1)
            {
                break;
            }
            QApplication::processEvents();

        }
        additionalWaitTime += 5;
    } else
    {
        emit responce("Connect to smth first, please.");
    }
    return;
}

QString MainWindow::readDataAction() //Считывание данных из буффера + ожидание новых данных. Если там оказалось что-то чего ты не ожидал увидеть - твои проблемы.
{
    QByteArray temp;

    temp = serial->readAll();
    if(serial->waitForReadyRead(100)) {
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

void MainWindow::showResponceData(QString data) // Слот для отображения чего нибудь в строчку responce и лог файл
{
    ui->textLineResponce->setText(data);
    data.append('\n');
    log_file.write(data.toLocal8Bit());
    return;
}

void MainWindow::on_pushButton_Recieve_clicked()//кнопачка чтобы считать данные из буфера. Нет не подождать данных. Считать из буффера
{
    QByteArray temp;
    if(serial->isOpen())
    {
        temp = serial->readAll();
        ui->textLineResponce->setText(QString(temp));
    } else
    {
        ui->textLineResponce->setText("Connect to something first, please");
    }
    return;
}

void MainWindow::on_pushButton_Send_clicked() // Для желающих общаться с теромоконтроллером лапками
{
    QString msg;
    msg = ui->textLineSend->text();
    msg.append('\n');
    if(serial->isOpen())
    {
        serial->write(msg.toLocal8Bit());
        showResponceData(readDataAction());
    } else
    {
        ui->textLineResponce->setText("Connect to something first, please");
    }
    return;
}

void MainWindow::on_pushButton_Connect_TC_clicked()//кнопачка чтобы совокупить прогу и контроллер
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

        QString query = "\n *IDN?\n";  //команда запроса прибору от Stanford Research Systems чтобы узнать что он такое
        QString answer;

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
    QList<qint32> bauds;
    bauds.append(QSerialPort::Baud19200);
    bauds.append(QSerialPort::Baud4800);
    bauds.append(QSerialPort::Baud1200);
    bauds.append(QSerialPort::Baud2400);
    bauds.append(QSerialPort::Baud38400);
    bauds.append(QSerialPort::Baud57600);
    bauds.append(QSerialPort::Baud115200);
    bauds.append(qint32(230400)); //это извращение увидел в настройках контроллера

    QString query = "\n *IDN?\n";//первый \n призван почистить все накопившееся на входе контроллера
    QString answer;

    foreach(qint32 baud, bauds) //для всех возможных baudrates попробуем получить информацию о приборе
    {
        serial->setBaudRate(baud);
        QThread::msleep(1000); //КОСТЫЛЬ. Я НЕ ЗНАЮ ПОЧЕМУ К СТАРОМУ КОНТРОЛЛЕРУ ОН БЕЗ ЭТОГО КРИВО ПОДКЛЮЧАЕТСЯ
        if (serial->isOpen())
        {
            sendDataAction(query);
            QApplication::processEvents();
            answer = readDataAction();

            if(answer.contains("Stanford")) //если ответ нормальный то выйдем из сканирования
            {
                return;
            }
        } else
        {
            if(serial->open(QIODevice::ReadWrite))
            {
                sendDataAction(query);
                answer = readDataAction();

                if(answer.contains("Stanford")) //если ответ нормальный то выйдем из сканирования
                {
                    return;
                }
            }
        }
    }
    return; //если не получилось то ну и ладно. Тут по хорошему нужно бы сделать возврат ошибки но мне лень
}

MainWindow::~MainWindow()//При закрытии окошка
{
     //delete timer;
    log_file.write("Closed\n");
    log_file.close();
    serial->close();
    delete serial;
    delete ui; // чисти, чисти
}


void MainWindow::on_checkBox_1_toggled(bool checked)    //строить график, если checkBox нажат
{
    this->run = ui->checkBox_1->isChecked();

    if (checkBox_1_first == 1)
    {
        timeStart = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
        checkBox_1_first++;
    }
    double currentTime = 0;
    double value = 0;

    while(run)
    {
        currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 - timeStart;
        value = sin(currentTime);
        ui->widget_T->graph(0)->addData(currentTime, value);
        QApplication::processEvents(QEventLoop::AllEvents, 5);

        ui->widget_T->xAxis->setRange(0, 40);
        ui->widget_T->replot();
    }

    return;
}

void MainWindow::Plot()
{



}
