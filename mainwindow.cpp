#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWidget>
#include <QTime>
#include <math.h>
#include <iostream>
#include <QDir>

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
    Colours.push_back("Yellow");
    ui->comboBox_Colour_1->clear();
    ui->comboBox_Colour_1->addItems(Colours);
    ui->comboBox_Colour_2->clear();
    ui->comboBox_Colour_2->addItems(Colours);
    ui->comboBox_Colour_3->clear();
    ui->comboBox_Colour_3->addItems(Colours);
    ui->comboBox_Colour_4->clear();
    ui->comboBox_Colour_4->addItems(Colours);
    ui->comboBox_Colour_5->clear();
    ui->comboBox_Colour_5->addItems(Colours);

    ui->comboBox_Colour_1->setCurrentIndex(0);
    ui->comboBox_Colour_2->setCurrentIndex(1);
    ui->comboBox_Colour_3->setCurrentIndex(2);
    ui->comboBox_Colour_4->setCurrentIndex(3);
    ui->comboBox_Colour_5->setCurrentIndex(4);

    on_actionUpdate_available_ports_triggered();// Получение списка доступных портов

    connect(this, SIGNAL(responce(QString)),
            this, SLOT(showResponceData(QString)));

    QDir dir;
    dir = QDir::current();
    if(!dir.exists("data"))
    {
        dir.mkdir("data");
    }

    log_file.setFileName("data/log.txt"); //Создание лог файла
    if(!log_file.open(QIODevice::ReadWrite))
    {
        ui->textLineResponce->setText("Log file wrecked");
    }

    log_file.readAll(); // перемещение текущей позиции в конец файла
    log_file.write(time.toString("dd.MM.yyyy hh:mm:ss").toLocal8Bit()); // начальная строчка с указанем текущего времени(надо сделать еще и дату)
    log_file.write(" \n");


    ui->widget_T->xAxis->setLabel("Time"); // Оси графика для температуры
    ui->widget_T->yAxis->setLabel("Value");
    ui->widget_T->yAxis->setRange(-1.5, 1.5); // временно
    ui->widget_T->clearGraphs();

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
    QString reply;
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
    reply = QString(temp);
    reply.remove(QChar('\n'), Qt::CaseInsensitive);
    reply.remove(QChar('\r'), Qt::CaseInsensitive);

    return reply;
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
    reserve_file.close();
    serial->close();
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
        if (colour == "Yellow")
        {
            ui->widget_T->graph(n)->setPen(QPen(Qt::yellow));
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
        if (colour == "Yellow")
        {
            ui->widget_P->graph(n)->setPen(QPen(Qt::yellow));
        }
    }
}

void MainWindow::Plot() //Одна итерация перестроения графиков
{

    QStringList ValueList;
    sendDataAction("getoutput");
    QString reply = readDataAction();
    ValueList = reply.split(",");

    currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 - timeStart;
    double value;

    reply.append(", " + QString::number(currentTime));
    reserve_file.write(reply.append("\n").toLocal8Bit());
    reserve_file.flush();

    if (ui->checkBox_1->isChecked())
    {
        reply = ValueList.at(index_1);
        ui->lineEdit_Channel_1->setText(reply);
        value = reply.toDouble();
        if((UnitList.at(index_1).contains("C")) | (UnitList.at(index_1).contains("K")))
        {
            ui->widget_T->graph(0)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_1->currentText(), 0, "T");
        }else
        {
            ui->widget_P->graph(0)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_1->currentText(), 0, "P");
        }

    }
    if (ui->checkBox_2->isChecked())
    {
        reply = ValueList.at(index_2);
        ui->lineEdit_Channel_2->setText(reply);
        value = reply.toDouble();
        if((UnitList.at(index_2).contains("C")) | (UnitList.at(index_2).contains("K")))
        {
            ui->widget_T->graph(1)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_2->currentText(), 1, "T");
        }else
        {
            ui->widget_P->graph(1)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_2->currentText(), 1, "P");
        }

    }
    if (ui->checkBox_3->isChecked())
    {
        reply = ValueList.at(index_3);
        ui->lineEdit_Channel_3->setText(reply);
        value = reply.toDouble();
        if((UnitList.at(index_3).contains("C")) | (UnitList.at(index_3).contains("K")))
        {
            ui->widget_T->graph(2)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_3->currentText(), 2, "T");
        }else
        {
            ui->widget_P->graph(2)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_3->currentText(), 2, "P");
        }

    }
    if (ui->checkBox_4->isChecked())
    {
        reply = ValueList.at(index_4);
        ui->lineEdit_Channel_4->setText(reply);
        value = reply.toDouble();
        if((UnitList.at(index_4).contains("C")) | (UnitList.at(index_4).contains("K")))
        {
            ui->widget_T->graph(3)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_4->currentText(), 3, "T");
        }else
        {
            ui->widget_P->graph(3)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_4->currentText(), 3, "P");
        }

    }
    if (ui->checkBox_5->isChecked())
    {
        reply = ValueList.at(index_5);
        ui->lineEdit_Channel_5->setText(reply);
        value = reply.toDouble();
        if((UnitList.at(index_5).contains("C")) | (UnitList.at(index_5).contains("K")))
        {
            ui->widget_T->graph(4)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_5->currentText(), 4, "T");
        }else
        {
            ui->widget_P->graph(4)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_5->currentText(), 4, "P");
        }

    }
    ui->widget_T->rescaleAxes();
    ui->widget_T->replot();
    ui->widget_P->rescaleAxes();
    ui->widget_P->replot();

}

void MainWindow::ReadNames() //Функция для считывания списка имен доступных каналов данных на PTC10
{
    QString message = "getOutput.Names";
    sendDataAction(message);

    QString reply = readDataAction();

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

    UnitList = reply.split(", ");

    reply.append(", ms\n");
    reserve_file.write(reply.toLocal8Bit());

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

void MainWindow::pid_Scan() //Функция для считывания текущих параметров сПИДа
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

        reply.remove(QChar(' '), Qt::CaseInsensitive);
        ui->comboBox_Input_PID->setCurrentIndex(NameList.indexOf(reply));

    }else
    {
        emit responce("pid scan failed");
    }
}

void MainWindow::on_pushButton_Start_PID_clicked() //Запуск сПИДа
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


                message = ui->comboBox_Output->currentText();
                message.append(".pid.setpoint = ");
                pid = ui->pid_LineEdit_Setpoint->text().toFloat();
                message.append(QString::number(pid, 'f', 2));
                sendDataAction(message);
                readDataAction();

                message = ui->comboBox_Output->currentText();
                message.append(".pid.input = ");
                message.append(ui->comboBox_Input_PID->currentText());


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

void MainWindow::on_pushButton_Plot_clicked()//Вечный(нет) цикл
{
    if(serial->isOpen())
    {
        if(ui->pushButton_Plot->text() == "PLOT")
        {
            QString name;
            QDateTime time;
            time = QDateTime::currentDateTime();

            name = "data/Reserve_file_"+time.toString("dd_MM_yyyy_hh_mm_ss") + ".txt";
            reserve_file.setFileName(name);
            if(!reserve_file.open(QIODevice::WriteOnly))
            {
                emit responce("Reserve file wrecked");
            }

            foreach (QString Name, NameList) {
                reserve_file.write(Name.append(", ").toLocal8Bit());
            }
            reserve_file.write(", Time\n");

            foreach (QString Unit, UnitList) {
                reserve_file.write(Unit.append(", ").toLocal8Bit());
            }
            reserve_file.write(", ms\n");




            timeStart = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;


            index_1 = NameList.indexOf(ui->comboBox_OutPut_1->currentText());
            index_2 = NameList.indexOf(ui->comboBox_OutPut_2->currentText());
            index_3 = NameList.indexOf(ui->comboBox_OutPut_3->currentText());
            index_4 = NameList.indexOf(ui->comboBox_OutPut_4->currentText());
            index_5 = NameList.indexOf(ui->comboBox_OutPut_5->currentText());

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
            for (int i = 0; i<5; i++)
            {
                ui->widget_T->graph(i)->data().data()->clear();
            }
            for (int i = 0; i<5; i++)
            {
                ui->widget_P->graph(i)->data().data()->clear();
            }
        reserve_file.close();
        }
    }
}

void MainWindow::on_checkBox_1_clicked() //попытка отключить один график(правда радикально и безвозвратно)
{
    if(ui->checkBox_1->isChecked())
    {
        ui->comboBox_OutPut_1->setDisabled(true);
        index_1 = NameList.indexOf(ui->comboBox_OutPut_1->currentText());

    } else
    {
        ui->comboBox_OutPut_1->setEnabled(true);
        ui->widget_P->graph(0)->data().data()->clear();
        ui->widget_T->graph(0)->data().data()->clear();
    }
}

void MainWindow::on_checkBox_2_clicked() //попытка отключить один график(правда радикально и безвозвратно)
{
    if(ui->checkBox_2->isChecked())
    {
        ui->comboBox_OutPut_2->setDisabled(true);
        index_2 = NameList.indexOf(ui->comboBox_OutPut_2->currentText());

    } else
    {
        ui->comboBox_OutPut_2->setEnabled(true);
        ui->widget_P->graph(1)->data().data()->clear();
        ui->widget_T->graph(1)->data().data()->clear();

    }
}

void MainWindow::on_checkBox_3_clicked() //попытка отключить один график(правда радикально и безвозвратно)
{
    if(ui->checkBox_3->isChecked())
    {
        ui->comboBox_OutPut_3->setDisabled(true);
        index_3 = NameList.indexOf(ui->comboBox_OutPut_3->currentText());

    } else
    {
        ui->comboBox_OutPut_3->setEnabled(true);
        ui->widget_P->graph(2)->data().data()->clear();
        ui->widget_T->graph(2)->data().data()->clear();

    }
}

void MainWindow::on_checkBox_4_clicked() //попытка отключить один график(правда радикально и безвозвратно)
{
    if(ui->checkBox_4->isChecked())
    {
        ui->comboBox_OutPut_4->setDisabled(true);
        index_4 = NameList.indexOf(ui->comboBox_OutPut_4->currentText());

    } else
    {
        ui->comboBox_OutPut_4->setEnabled(true);
        ui->widget_P->graph(3)->data().data()->clear();
        ui->widget_T->graph(3)->data().data()->clear();

    }
}

void MainWindow::on_checkBox_5_clicked() //попытка отключить один график(правда радикально и безвозвратно)
{
    if(ui->checkBox_5->isChecked())
    {
        ui->comboBox_OutPut_5->setDisabled(true);
        index_5 = NameList.indexOf(ui->comboBox_OutPut_5->currentText());

    } else
    {
        ui->comboBox_OutPut_5->setEnabled(true);
        ui->widget_P->graph(4)->data().data()->clear();
        ui->widget_T->graph(4)->data().data()->clear();

    }
}

void MainWindow::on_pushButton_Check_clicked()//Проверка сПИДа
{
    pid_Scan();
    return;
}
