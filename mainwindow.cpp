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
    ui->comboBox_Colour_1->clear();
    ui->comboBox_Colour_1->addItems(Colours);

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
        fullAnswer = readDataAction();
    } else
    {
        emit responce("Connect to smth first, please.");
    }
    return;
}

QString MainWindow::readDataAction() //Считывание данных из буффера + ожидание новых данных. Если там оказалось что-то чего ты не ожидал увидеть - твои проблемы.
{
    QByteArray temp;
    if (serial->isOpen())
    {
        temp = serial->readAll();
        if(serial->waitForReadyRead(100)) {
            temp += serial->readAll();
            while(serial->waitForReadyRead(additionalWaitTime))
            {
                temp += serial->readAll();
            }
        }
    }
    return QString(temp);
}

void MainWindow::sendDataAction(QString data)//Отправка данных пациенту. Ну или запросов. Ну или порнографии. Мало ли на что у меня совести хватит.
{
    if (serial->isOpen())
    {  
        data.append('\n');
        serial->write(data.toLocal8Bit());
    }
    return;
}

void MainWindow::showResponceData(QString data) // Слот для отображения чего нибудь в строчку responce и лог файл
{
    ui->textLineResponce->setText(data);
    if(log_file.isOpen())
    {
        data.append('\n');
        log_file.write(data.toLocal8Bit());
    } else
    {
        log_file.setFileName("log.txt"); //Создание лог файла
        if(!log_file.open(QIODevice::ReadWrite))
        {
            ui->textLineResponce->setText("Log file wrecked");
        }
        log_file.readAll(); // перемещение текущей позиции в конец файла
        log_file.write(QTime::currentTime().toString().toLocal8Bit()); // начальная строчка с указанем текущего времени(надо сделать еще и дату)
        log_file.write("It had wrecked \n");
    }
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
    if(serial->isOpen())
    {
        sendDataAction(msg);
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

        QString query = "\n *IDN?";  //команда запроса прибору от Stanford Research Systems чтобы узнать что он такое
        QString answer;

        sendDataAction(query); // запрос прибору...
        answer = readDataAction(); //попытка получить ответ

        if(!answer.contains("Stanford")) // если ответ не такой как хотелось бы то попробуем перебрать изменяемые внутри прибора характеристики
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
            ReadNames();
            ReadUnits();
            QApplication::processEvents();
            pid_Scan();
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

void MainWindow::scanBauds() // функция для перебора всех возможных baudrate, будет смешно если она случайно подключится не к стэнфордскому прибору.
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

    QString query = "\n *IDN?";//первый \n призван почистить все накопившееся на входе контроллера
    QString answer;

    foreach(qint32 baud, bauds) //для всех возможных baudrates попробуем получить информацию о приборе
    {
        serial->setBaudRate(baud);
        QTest::qWait(1000); //КОСТЫЛЬ. Я НЕ ЗНАЮ ПОЧЕМУ К СТАРОМУ КОНТРОЛЛЕРУ ОН БЕЗ ЭТОГО КРИВО ПОДКЛЮЧАЕТСЯ
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
    QApplication::processEvents(QEventLoop::AllEvents, 5);
    delete serial;
    delete ui; // чисти, чисти
}

void MainWindow::SetColour(QString colour, const int n, QString index) //цвет графика из checkbox
{
    if (index == "T")
    {
        if (colour == "Black")
        {
            ui->widget_T->graph(n)->setPen(QPen(Qt::black));
        }
        if (colour == "Red")
        {
            ui->widget_T->graph(n)->setPen(QPen(Qt::red));
        }
        if (colour == "Green")
        {
            ui->widget_T->graph(n)->setPen(QPen(Qt::green));
        }
        if (colour == "Blue")
        {
            ui->widget_T->graph(n)->setPen(QPen(Qt::blue));
        }
    }
    if (index == "P")
    {
        if (colour == "Black")
        {
            ui->widget_P->graph(n)->setPen(QPen(Qt::black));
        }
        if (colour == "Red")
        {
            ui->widget_P->graph(n)->setPen(QPen(Qt::red));
        }
        if (colour == "Green")
        {
            ui->widget_P->graph(n)->setPen(QPen(Qt::green));
        }
        if (colour == "Blue")
        {
            ui->widget_P->graph(n)->setPen(QPen(Qt::blue));
        }
    }
}

void MainWindow::Plot() //nothing
{

    QStringList ValueList;
    sendDataAction("getoutput");
    QString reply = readDataAction();
    ValueList = reply.split(",");

    currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 - timeStart;
    double value;

    if (ui->checkBox_1->isChecked())
    {
        reply = ValueList.at(index_1);
        value = reply.toDouble();
        if((UnitList.at(index_1).contains("C")) | (UnitList.at(index_1).contains("K")))
        {
            ui->widget_T->graph(0)->addData(currentTime, value);
            SetColour(ui->checkBox_1->text(), 0, "T");
        }else
        {
            ui->widget_P->graph(0)->addData(currentTime, value);
            SetColour(ui->checkBox_1->text(), 0, "P");
        }

    }
    if (ui->checkBox_2->isChecked())
    {
        reply = ValueList.at(index_2);
        value = reply.toDouble();
        if((UnitList.at(index_2).contains("C")) | (UnitList.at(index_2).contains("K")))
        {
            ui->widget_T->graph(1)->addData(currentTime, value);
            SetColour(ui->checkBox_2->text(), 1, "T");
        }else
        {
            ui->widget_P->graph(1)->addData(currentTime, value);
            SetColour(ui->checkBox_2->text(), 1, "P");
        }

    }
    if (ui->checkBox_3->isChecked())
    {
        reply = ValueList.at(index_3);
        value = reply.toDouble();
        if((UnitList.at(index_3).contains("C")) | (UnitList.at(index_3).contains("K")))
        {
            ui->widget_T->graph(2)->addData(currentTime, value);
            SetColour(ui->checkBox_3->text(), 2, "T");
        }else
        {
            ui->widget_P->graph(2)->addData(currentTime, value);
            SetColour(ui->checkBox_3->text(), 2, "P");
        }

    }
    if (ui->checkBox_4->isChecked())
    {
        reply = ValueList.at(index_4);
        value = reply.toDouble();
        if((UnitList.at(index_4).contains("C")) | (UnitList.at(index_4).contains("K")))
        {
            ui->widget_T->graph(3)->addData(currentTime, value);
            SetColour(ui->checkBox_4->text(), 3, "T");
        }else
        {
            ui->widget_P->graph(3)->addData(currentTime, value);
            SetColour(ui->checkBox_4->text(), 3, "P");
        }

    }
    if (ui->checkBox_5->isChecked())
    {
        reply = ValueList.at(index_5);
        value = reply.toDouble();
        if((UnitList.at(index_5).contains("C")) | (UnitList.at(index_5).contains("K")))
        {
            ui->widget_T->graph(4)->addData(currentTime, value);
            SetColour(ui->checkBox_5->text(), 4, "T");
        }else
        {
            ui->widget_P->graph(4)->addData(currentTime, value);
            SetColour(ui->checkBox_5->text(), 4, "P");
        }

    }
    ui->widget_T->rescaleAxes();
    ui->widget_T->replot();
}

void MainWindow::ReadNames() //Функция для считывания списка имен доступных каналов данных на PTC10
{
    QString message = "getOutput.Names";
    sendDataAction(message);

    QString reply = readDataAction();

    reply.remove(QChar('\n'), Qt::CaseInsensitive);
    reply.remove(QChar('\r'), Qt::CaseInsensitive);

    reply.remove(QChar(' '), Qt::CaseInsensitive);

    NameList = reply.split(",");



    ui->comboBox_OutPut_1->clear();
    ui->comboBox_OutPut_2->clear();
    ui->comboBox_OutPut_3->clear();
    ui->comboBox_OutPut_4->clear();
    ui->comboBox_OutPut_5->clear();
    ui->comboBox_Input_PID->clear();
    ui->comboBox_OutPut_1->addItems(NameList);
    ui->comboBox_OutPut_2->addItems(NameList);
    ui->comboBox_OutPut_3->addItems(NameList);
    ui->comboBox_OutPut_4->addItems(NameList);
    ui->comboBox_OutPut_5->addItems(NameList);
    ui->comboBox_Input_PID->addItems(NameList);
}

void MainWindow::ReadUnits() //Функция для считывания списка единиц измерения в доступных каналах данных на PTC10
{
    QString message = "getOutput.Units";
    sendDataAction(message);
    int i;

    QString reply = readDataAction();

    reply.remove(QChar('\n'), Qt::CaseInsensitive);
    reply.remove(QChar('\r'), Qt::CaseInsensitive);
    UnitList = reply.split(',');

    ui->comboBox_Output->clear();
    if(UnitList.length() != NameList.length())
    {
        emit responce("smth gone very wrong while reading units");
        return;
    }
    for(i = 0; i < UnitList.length(); i++)
    {
//        QApplication::processEvents();
        if (UnitList.at(i).contains("W") or UnitList.at(i).contains("A") or UnitList.at(i).contains("V"))
        {
            message = NameList.at(i);
            message.append(".list");

            sendDataAction(message);
            reply = readDataAction();

            if (reply.contains("pid"))
            {
                ui->comboBox_Output->addItem(NameList.at(i));
            }
        }
    }
    return;
}

void MainWindow::on_pushButton_Start_Power_clicked() //Функция для задания фиксированной мощности на термоконтроллере. По дороге выключается пид и включается возможность подавать мощность
{
    QString reply;
    QString message;
    float power;

    if(serial->isOpen())
    {
        if(ui->comboBox_Output->currentText() == "")
        {
            emit responce("No output detected");
            return;
        } else
        {
            message = ui->comboBox_Output->currentText();
            message.append(".list");

            sendDataAction(message);
            reply = readDataAction();
            if(reply.contains("pid"))
            {
                message = ui->comboBox_Output->currentText();
                message.append(".pid.mode = off");
                sendDataAction(message);
                readDataAction();

                message = "outputenable = on";
                sendDataAction(message);
                readDataAction();

                message = ui->comboBox_Output->currentText();
                message.append(".value = ");
                reply = ui->lineEdit_power->text();
                power = reply.toFloat();
                message.append(QString::number(power, 'f', 2));
                sendDataAction(message);
                readDataAction();


            }else
            {
                emit responce("This is not an output, smth gone very wrong");
                return;
            }
        }
    }else
    {
        emit responce("Connect to smth first, please");
        return;
    }
    return;
}

void MainWindow::pid_Scan()
{
    QString message;
    QString reply;

    if(serial->isOpen())
    {
        message = ui->comboBox_Output->currentText();
        message.append(".pid.p?");
        sendDataAction(message);
        reply = readDataAction();

        ui->pid_LineEdit_P->setText(reply);

        message = ui->comboBox_Output->currentText();
        message.append(".pid.d?");
        sendDataAction(message);
        reply = readDataAction();

        ui->pid_LineEdit_D->setText(reply);

        message = ui->comboBox_Output->currentText();
        message.append(".pid.i?");
        sendDataAction(message);
        reply = readDataAction();

        ui->pid_LineEdit_I->setText(reply);

        message = ui->comboBox_Output->currentText();
        message.append(".pid.setpoint?");
        sendDataAction(message);
        reply = readDataAction();

        ui->pid_LineEdit_Setpoint->setText(reply);

        message = ui->comboBox_Output->currentText();
        message.append(".pid.input?");
        sendDataAction(message);
        reply = readDataAction();

        ui->comboBox_Input_PID->setCurrentText(reply);

    }else
    {
        emit responce("pid scan failed");
    }
}

void MainWindow::on_pushButton_Start_PID_clicked()
{
    QString reply;
    QString message;
    float pid;

    if(serial->isOpen())
    {
        if(ui->comboBox_Output->currentText() == "")
        {
            emit responce("No output detected");
        }else
        {
            message = ui->comboBox_Output->currentText();
            message.append(".list");

            sendDataAction(message);
            reply = readDataAction();
            if(reply.contains("pid"))
            {
                message = ui->comboBox_Output->currentText();
                message.append(".pid.p = ");
                pid = ui->pid_LineEdit_P->text().toFloat();
                message.append(QString::number(pid, 'f', 2));
                sendDataAction(message);
                readDataAction();

                message = ui->comboBox_Output->currentText();
                message.append(".pid.i = ");
                pid = ui->pid_LineEdit_I->text().toFloat();
                message.append(QString::number(pid, 'f', 2));
                sendDataAction(message);
                readDataAction();

                message = ui->comboBox_Output->currentText();
                message.append(".pid.d = ");
                pid = ui->pid_LineEdit_D->text().toFloat();
                message.append(QString::number(pid, 'f', 2));
                sendDataAction(message);
                readDataAction();

                message = "outputenable = on";
                sendDataAction(message);
                readDataAction();

                message = ui->comboBox_Output->currentText();
                message.append(".pid.mode = on");
                sendDataAction(message);
                readDataAction();

                return;
            }else
            {
                emit responce("This is not an output, smth gone very wrong");
                return;
            }
        }
    }else
    {
        emit responce("Connect to smth first, please");
    }
    return;
}

void MainWindow::on_pushButton_Plot_clicked()
{
    if(ui->pushButton_Plot->text() == "PLOT")
    {
        if (start == 0)
        {
            ui->widget_T->addGraph();
            ui->widget_T->addGraph();
            ui->widget_T->addGraph();
            ui->widget_T->addGraph();
            ui->widget_T->addGraph();

            ui->widget_P->addGraph();
            ui->widget_P->addGraph();
            ui->widget_P->addGraph();
            ui->widget_P->addGraph();
            ui->widget_P->addGraph();

            start ++;
        }


        timeStart = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;


        index_1 = NameList.indexOf(ui->comboBox_OutPut_1->currentText());
        index_2 = NameList.indexOf(ui->comboBox_OutPut_2->currentText());

        ui->pushButton_Plot->setText("STOP");
        run = true;

        while (run)
        {
            Plot();
            QTest::qWait(50);
        }
    } else
    {
        run = false;
        ui->pushButton_Plot->setText("PLOT");
        ui->widget_T->clearGraphs();
    }
}

void MainWindow::on_checkBox_1_clicked()
{
    if(ui->checkBox_1->isChecked())
    {
        ui->comboBox_OutPut_1->setDisabled(true);

    } else
    {
        ui->comboBox_OutPut_1->setEnabled(true);
        ui->widget_P->graph(0)->data().data()->clear();

    }
}
