#include "ptc10.h"
#include <QThread>

PTC10::PTC10(QObject *parent) : QObject(parent)//ЗДЕСЬ НЕ ДОЛЖНО БЫТЬ НИЧЕГО ПОТОМУ ЧТО ВСЁ ЧТО ЗДЕСЬ ПРОИСХОДИТ, ПРОИСХОДИТ В ДРУГОМ ПОТОКЕ
{
}

bool PTC10::isOpen() //На случай если я захочу вдруг извне проверить что COM порт открыт
{
    return serial->isOpen();
}

QString PTC10::readDataAction() //Считывание данных из буффера + ожидание новых данных. Если там оказалось что-то чего ты не ожидал увидеть - твои проблемы.
{
    QByteArray temp;
    QString reply;

    if (serial->isOpen()) //Если COM порт закрыт то и читать неоткуда
    {
        temp = serial->readAll(); //Предварительное вычищение буфера

        //Попытка немного подождать еще данных

        if(serial->waitForReadyRead(100))
        {
            temp += serial->readAll();
            while(serial->waitForReadyRead(additionalWaitTime))
            {
                temp += serial->readAll();
            }
        }

        //Приведение данных к "удобному" виду

        reply = QString(temp);
        reply.remove(QChar('\n'), Qt::CaseInsensitive);
        reply.remove(QChar('\r'), Qt::CaseInsensitive);
    }

    //Надо подумать стоит ли сюда добавлять какой-либо специфичный ответ на случай закрытости порта

    return reply;
}

void PTC10::sendDataAction(QString data) //Отправка данных пациенту. Ну или запросов. Ну или порнографии. Мало ли на что у меня совести хватит.
{
    if (serial->isOpen()) //Если COM порт закрыт то я даже пытаться отправлять что-либо не хочу
    {
        data.append('\n'); //Символ окончания команды для термоконтроллера
        serial->write(data.toLocal8Bit());
    }
    return;
}

void PTC10::calibrateWaitTime()
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

        while(answer == fullAnswer)
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

void PTC10::connect(QString portName)
{
    serial->setDataBits(QSerialPort::Data8);//Значения по мануалу такие
    serial->setStopBits(QSerialPort::OneStop);//Работает не трогай
    serial->setParity(QSerialPort::NoParity);//Если сломается то скорее всего не здесь
    serial->setFlowControl(QSerialPort::HardwareControl);

    serial->setBaudRate(QSerialPort::Baud9600);//дефолт. ЭТО ТРОГАТЬ МОЖНО.
    serial->setPortName(portName); //пока что выбирать порт будем лапками

    if (!serial->open(QIODevice::ReadWrite)) //попытка подключится с дефолтными параметрами
    {
        QSerialPort::SerialPortError getError = QSerialPort::NoError;
        getError = serial->error();
        if (getError == QSerialPort::DeviceNotFoundError)
        {
            emit responce("Device not found");
        } else
        {
            emit responce("SerialPort error number " + QString::number(getError));
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
        emit connected();
        readNames();
        readUnits();
        QApplication::processEvents();
        if(!OutputList.empty())
        {
            pidScan(OutputList.at(0));
        }
        return;
    }
}

void PTC10::disconnect()
{
    serial->close();
    return;
}

void PTC10::scanBauds()
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

void PTC10::readNames()
{
    QString message = "getOutput.Names";
    sendDataAction(message);

    QString reply = readDataAction();

    reply.remove(QChar(' '), Qt::CaseInsensitive);

    NameList = reply.split(",");

    emit readNamesResult(NameList);
}

void PTC10::readUnits()
{
    QString message = "getOutput.Units";
    int i;

    sendDataAction(message);
    QString reply = readDataAction();

    UnitList = reply.split(", ");

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
                OutputList.append(NameList.at(i));
            }
        }
    }
    emit readUnitsResult(UnitList, OutputList);
    return;
}

void PTC10::pidScan(QString output)
{
    QString message;
    QString reply;
    QStringList pidStatus;

    if(serial->isOpen())
    {
        message = output;
        message.append(".pid.p?");
        sendDataAction(message);
        reply = readDataAction();

        pidStatus.append(reply);
        //ui->pid_LineEdit_P->setText(reply);

        message = output;
        message.append(".pid.d?");
        sendDataAction(message);
        reply = readDataAction();

        pidStatus.append(reply);
//        ui->pid_LineEdit_D->setText(reply);

        message = output;
        message.append(".pid.i?");
        sendDataAction(message);
        reply = readDataAction();

        pidStatus.append(reply);
//        ui->pid_LineEdit_I->setText(reply);

        message = output;
        message.append(".pid.setpoint?");
        sendDataAction(message);
        reply = readDataAction();

        pidStatus.append(reply);
//        ui->pid_LineEdit_Setpoint->setText(reply);

        message = output;
        message.append(".pid.input?");
        sendDataAction(message);
        reply = readDataAction();

        reply.remove(QChar(' '), Qt::CaseInsensitive);
        pidStatus.append(reply);
//        ui->comboBox_Input_PID->setCurrentIndex(NameList.indexOf(reply));

        emit pidScanResult(pidStatus);
    }else
    {
        emit responce("pid scan failed");
    }
}

void PTC10::pidStart(QString output, QStringList pidStatus)
{
    QString reply;
    QString message;
    float pid;

    if(serial->isOpen())
    {
        if(output == "")
        {
            emit responce("No output detected");
        }else
        {
            message = output;
            message.append(".list");

            sendDataAction(message);
            reply = readDataAction();
            if(reply.contains("pid"))
            {
                message = output;
                message.append(".pid.p = ");
                pid = pidStatus.at(0).toFloat();
                message.append(QString::number(pid, 'f', 2));
                sendDataAction(message);
                readDataAction();

                message = output;
                message.append(".pid.i = ");
                pid = pidStatus.at(1).toFloat();
                message.append(QString::number(pid, 'f', 2));
                sendDataAction(message);
                readDataAction();

                message = output;
                message.append(".pid.d = ");
                pid = pidStatus.at(2).toFloat();
                message.append(QString::number(pid, 'f', 2));
                sendDataAction(message);
                readDataAction();


                message = output;
                message.append(".pid.setpoint = ");
                pid = pidStatus.at(3).toFloat();
                message.append(QString::number(pid, 'f', 2));
                sendDataAction(message);
                readDataAction();

                message = output;
                message.append(".pid.input = ");
                message.append(pidStatus.at(4));
                sendDataAction(message);
                readDataAction();

                message = "outputenable = on";
                sendDataAction(message);
                readDataAction();

                message = output;
                message.append(".pid.mode = on");
                sendDataAction(message);
                readDataAction();

                emit pidStarted();
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

void PTC10::OutputEnable(QString message)
{
    if(serial->isOpen())
    {
        sendDataAction(message);
    }
    else
    {
        emit responce("Connect to smth first, please");
        return;
    }
    return;
}

void PTC10::powerStart(QString output, float power)
{
    QString reply;
    QString message;

    if(serial->isOpen())
    {
        if(output == "")
        {
            emit responce("No output detected");
            return;
        } else
        {
            message = output;
            message.append(".list");

            sendDataAction(message);
            reply = readDataAction();
            if(reply.contains("pid"))
            {
                message = output;
                message.append(".pid.mode = off");
                sendDataAction(message);
                readDataAction();

                message = "outputenable = on";
                sendDataAction(message);
                readDataAction();

                message = output;
                message.append(".value = ");
                message.append(QString::number(power, 'f', 2));
                sendDataAction(message);
                readDataAction();

                emit powerStarted();
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

void PTC10::plotStart()
{
   if(serial->isOpen())
    {
        if(reserveFile->isOpen())
        {
            reserveFile->close();
        }
        QString name;
        QDateTime time;
        time = QDateTime::currentDateTime();

        name = "dataImpairor/Reserve_file_"+time.toString("dd_MM_yyyy_hh_mm_ss") + ".dat";
        reserveFile->setFileName(name);
        if(!reserveFile->open(QIODevice::WriteOnly))
        {
            emit responce("Reserve file wrecked");
        }

        foreach (QString Name, NameList) {
            reserveFile->write(Name.append(", ").toLocal8Bit());
        }
        reserveFile->write("Time, SystemTime\n");

        foreach (QString Unit, UnitList) {
            reserveFile->write(Unit.append(", ").toLocal8Bit());
        }
        reserveFile->write("s\n");




        timeStart = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;


//        index_1 = NameList.indexOf(ui->comboBox_OutPut_1->currentText());
//        index_2 = NameList.indexOf(ui->comboBox_OutPut_2->currentText());
//        index_3 = NameList.indexOf(ui->comboBox_OutPut_3->currentText());
//        index_4 = NameList.indexOf(ui->comboBox_OutPut_4->currentText());
//        index_5 = NameList.indexOf(ui->comboBox_OutPut_5->currentText());

//        ui->pushButton_Plot->setText("STOP");
        run = true;

        while (run)
        {
            plot();
            QTest::qWait(50);
            QApplication::processEvents();
        }
    }
}

void PTC10::plot()
{
    double currentSystemTime;
    double currentTime;
    QStringList ValueList;
    sendDataAction("getoutput");
    QString reply = readDataAction();
    QStringList plottedValueList;

    ValueList = reply.split(",");
    if(ValueList.size() != NameList.size())
    {
        return;
    }

    currentSystemTime = QTime::currentTime().msecsSinceStartOfDay()/1000.0;
    currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 - timeStart;
    double value;

    reply.append(", " + QString::number(currentTime) + ", " + QString::number(currentSystemTime, 'g', 8));
    reserveFile->write(reply.append("\n").toLocal8Bit());
    reserveFile->flush();

    for(int i = 0; i<plotIndexList.size(); i++)
    {
        if(plotIndexList.at(i) == -1)
        {
            break;
        }
        //plottedValueList.append(ValueList.at(plotIndexList.at(i)));
        value = ValueList.at(plotIndexList.at(i)).toDouble();
        if(UnitList.at(plotIndexList.at(i)).contains("C") | (UnitList.at(plotIndexList.at(i)).contains("K")))
        {
            emit dataForGraph_T(i, value, currentTime);
        } else
        {
            emit dataForGraph_P(i, value, currentTime);
        }
    }
    emit updateGraphs();
}

void PTC10::plotStop()
{
    run = false;
    emit plotStopped();
}

void PTC10::exportData(QString Name)
{
    Name.prepend("dataImpairor/");
    Name.append(".dat");
    if(QFile::exists(Name))
    {
        emit responce("Set another name");
        return;
    }
    if (reserveFile->isOpen())
    {
        QString Name1 = Name;
        if (Name1.remove(" ", Qt::CaseInsensitive)!="")
        {
            if(reserveFile->copy(Name))
            {
                reserveFile->open(QIODevice::ReadWrite);
                reserveFile->readAll();
                emit responce("Exported");
                return;
            }
        }
    }
    emit responce("File wasn't created");
}

QString PTC10::sendAndRead(QString data)
{
    if(serial->isOpen())
    {
        QString answer;
        sendDataAction(data);
        answer = readDataAction();
        emit responce(answer);
        return answer;
    } else
    {
        emit responce("connect to smth first please");
        return "";
    }
}

void PTC10::setIndexList(QList <int> indexList)
{
    plotIndexList = indexList;
    return;
}

void PTC10::finish()
{
    run = false;
    serial->close();
    reserveFile->close();
    this->~PTC10();
}

void PTC10::create()
{
    serial = new QSerialPort(this); // переменная для подключения по COM порту
    reserveFile = new QFile(this);
}

PTC10::~PTC10()
{
    this->thread()->exit();
    delete serial;
}

