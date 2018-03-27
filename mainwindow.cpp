#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) : // То что произойдет в нулевой момент времени при создании окошка
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
//"this" в скобочках символизирует то что данное окошко является родителем для всех этих переменных
//А значит раз оно их породило то оно их и убъет

    ui->setupUi(this); //Хз что это
    serial = new QSerialPort(this);
    log_file = new QFile(this); // Лог файл
    reserve_file = new QFile(this); // Резервный файл
    device = new PTC10(this); // класс в котором должны лежать все функции связанные с работой термоконтроллера

// Небесполезная тренировка пользования системой сигнал слот
    connect(device, SIGNAL(responce(QString)), this, SLOT(showResponceData(QString)));
    connect(device, SIGNAL(connected()), this, SLOT(gotConnected()));
    connect(device, SIGNAL(readNamesResult(QStringList)), this, SLOT(ReadNames(QStringList)));
    connect(device, SIGNAL(readUnitsResult(QStringList,QStringList)), this, SLOT(ReadUnits(QStringList,QStringList)));
    connect(device, SIGNAL(pidScanResult(QStringList)), this, SLOT(pid_Scan(QStringList)));
    connect(this, SIGNAL(sendIndexList(QList<int>)), device, SLOT(setIndexList(QList<int>)));
    connect(device, SIGNAL(dataForGraph_P(int,double,double)), this, SLOT(addDataToGraphP(int,double,double)));
    connect(device, SIGNAL(dataForGraph_T(int,double,double)), this , SLOT(addDataToGraphT(int,double,double)));
    connect(device, SIGNAL(updateGraphs()), this, SLOT(updateGraphs()));
    connect(this, SIGNAL(finishIt()), device, SLOT(finish()));


// Заполнение ComboBox-ов связанных с цветами графиков

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

//Установка изначальной позиции на разные цвета потому что так удобнее

    ui->comboBox_Colour_1->setCurrentIndex(0);
    ui->comboBox_Colour_2->setCurrentIndex(1);
    ui->comboBox_Colour_3->setCurrentIndex(2);
    ui->comboBox_Colour_4->setCurrentIndex(3);
    ui->comboBox_Colour_5->setCurrentIndex(4);

// Получение списка доступных портов

    on_actionUpdate_available_ports_triggered();

//+-бесполезная тренировка пользования системой сигнал слот

    connect(this, SIGNAL(responce(QString)),
            this, SLOT(showResponceData(QString)));

//В случае если в папке где находится ехе файл нет нужной директории куда складывать данные то надо бы её создать
//!!Чисто формально пользователь может её удалить и что-нить сломается
    QDir dir;
    dir = QDir::current();
    if(!dir.exists("data"))
    {
        dir.mkdir("data");
    }

//Создание лог файла

    log_file->setFileName("data/log.txt");
    if(!log_file->open(QIODevice::ReadWrite)) //на случай если не получится открыть(ну мало ли вдруг кто то параллельно юзает)
    {
        ui->textLineResponce->setText("Log file wrecked");
    }

//Запись времени начала измерения в лог файл

    QDateTime time = QDateTime::currentDateTime();
    log_file->readAll(); // перемещение текущей позиции в конец файла
    log_file->write(time.toString("dd.MM.yyyy hh:mm:ss").toLocal8Bit()); // начальная строчка с указанем текущей даты
    log_file->write(" \n");

// Установление стандартных параметров для графика температуры
// Нужно тоже самое для мощности (наверно)

    ui->widget_T->xAxis->setLabel("Time"); // Оси графика для температуры
    ui->widget_T->yAxis->setLabel("Value");
    ui->widget_T->yAxis->setRange(-1.5, 1.5); // временно
    ui->widget_T->clearGraphs();

// Чтобы гарантировать что мы всегда сможем обратиться к графику

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

// Просто так

    ui->widget_T->setInteractions(QCP::iSelectPlottables);
    ui->widget_P->setInteractions(QCP::iSelectPlottables);

    ui->groupBoxOutput1->hide();
    ui->groupBoxOutput2->hide();
    ui->groupBoxOutput3->hide();
    ui->groupBoxOutput4->hide();
    ui->groupBoxOutput5->hide();
    ui->groupBoxPID->hide();
    ui->groupBoxPower->hide();
    ui->pushButton_Plot->hide();
    ui->comboBox_Output->hide();
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
    device->calibrateWaitTime();
    return;
}

QString MainWindow::readDataAction() //Считывание данных из буффера + ожидание новых данных. Если там оказалось что-то чего ты не ожидал увидеть - твои проблемы.
{
    return device->readDataAction();
}

void MainWindow::sendDataAction(QString data)
{
    device->sendDataAction(data);
    return;
}

void MainWindow::showResponceData(QString data) // Слот для отображения чего нибудь в строчку responce и лог файл
{
    ui->textLineResponce->setText(data);
    if(log_file->isOpen())
    {
        data.append('\n');
        log_file->write(data.toLocal8Bit());
    } else
    {
        log_file->setFileName("log.txt"); //Создание лог файла
        if(!log_file->open(QIODevice::ReadWrite))
        {
            ui->textLineResponce->setText("Log file wrecked");
        }
        log_file->readAll(); // перемещение текущей позиции в конец файла
        log_file->write(QTime::currentTime().toString().toLocal8Bit()); // начальная строчка с указанем текущего времени(надо сделать еще и дату)
        log_file->write("It had wrecked \n");
    }
    return;
}

void MainWindow::on_pushButton_Recieve_clicked()//кнопачка чтобы считать данные из буфера. Нет не подождать данных. Считать из буффера
{
    QByteArray temp = "";
    ui->textLineResponce->setText(QString(temp));
    return;
}

void MainWindow::on_pushButton_Send_clicked() // Для желающих общаться с теромоконтроллером лапками. МНУ. ЭТО МНУ! Я ОДИН ТАКОЙ.
{
    QString msg;
    msg = ui->textLineSend->text();
    device->sendAndRead(msg);
    return;
}

void MainWindow::on_pushButton_Connect_TC_clicked()//кнопачка чтобы совокупить прогу и контроллер
{
    if(ui->pushButton_Connect_TC->text() == "Connect")
    {
        device->connect(ui->comboBoxPortName->currentText());

    } else //Дисконнект
    {
        device->disconnect();
        ui->pushButton_Connect_TC->setText("Connect");
        return;
    }
    return; //Я конечно не знаю как сюда можно попасть но пусть будет.
}

MainWindow::~MainWindow()//При закрытии окошка
{
     //delete timer;.
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
    double currentSystemTime;
    QStringList ValueList;
    sendDataAction("getoutput");
    QString reply = readDataAction();
    if(reply == "")
    {
        return;
    }
    ValueList = reply.split(",");

    currentSystemTime = QTime::currentTime().msecsSinceStartOfDay()/1000.0;
    currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0 - timeStart;
    double value;

    reply.append(", " + QString::number(currentTime) + ", " + QString::number(currentSystemTime, 'g', 8));
    reserve_file->write(reply.append("\n").toLocal8Bit());
    reserve_file->flush();

    if (ui->checkBox_1->isChecked())
    {
        reply = ValueList.at(index_1);
        ui->lineEdit_Channel_1->setText(reply);
        value = reply.toDouble();
        if((UnitList.at(index_1).contains("C")) | (UnitList.at(index_1).contains("K")))
        {
            ui->widget_T->graph(0)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_1->currentText(), 0, "T");
            ui->label_unit_1->setText("C");
        }else
        {
            ui->widget_P->graph(0)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_1->currentText(), 0, "P");
            ui->label_unit_1->setText("W");
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
            ui->label_unit_2->setText("C");
        }else
        {
            ui->widget_P->graph(1)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_2->currentText(), 1, "P");
            ui->label_unit_2->setText("W");
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
            ui->label_unit_3->setText("C");
        }else
        {
            ui->widget_P->graph(2)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_3->currentText(), 2, "P");
            ui->label_unit_3->setText("W");
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
            ui->label_unit_4->setText("C");
        }else
        {
            ui->widget_P->graph(3)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_4->currentText(), 3, "P");
            ui->label_unit_4->setText("W");
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
            ui->label_unit_5->setText("C");
        }else
        {
            ui->widget_P->graph(4)->addData(currentTime, value);
            SetColour(ui->comboBox_Colour_5->currentText(), 4, "P");
            ui->label_unit_5->setText("W");
        }

    }
    if (!ui->checkBox_fixPlot_P->isChecked())
    {
        ui->widget_P->rescaleAxes();
        if(ui->widget_P->yAxis->range().size()<0.01)
        {
            ui->widget_P->yAxis->setRange(ui->widget_P->yAxis->range().center()-0.1, ui->widget_P->yAxis->range().center()+0.1);
        }
        ui->widget_P->replot();
    }

    if (!ui->checkBox_fixPlot_T->isChecked())
    {
        ui->widget_T->rescaleAxes();
        if(ui->widget_T->yAxis->range().size()<0.01)
        {
            ui->widget_T->yAxis->setRange(ui->widget_T->yAxis->range().center()-0.1, ui->widget_T->yAxis->range().center()+0.1);
        }
        ui->widget_T->replot();
    }
}

void MainWindow::ReadNames(QStringList nameList) //Функция для считывания списка имен доступных каналов данных на PTC10
{
    NameList = nameList;

    ui->comboBox_OutPut_1->clear();
    ui->comboBox_OutPut_2->clear();
    ui->comboBox_OutPut_3->clear();
    ui->comboBox_OutPut_4->clear();
    ui->comboBox_OutPut_5->clear();
    ui->comboBox_Input_PID->clear();
    ui->comboBox_OutPut_1->addItems(nameList);
    ui->comboBox_OutPut_2->addItems(nameList);
    ui->comboBox_OutPut_3->addItems(nameList);
    ui->comboBox_OutPut_4->addItems(nameList);
    ui->comboBox_OutPut_5->addItems(nameList);
    ui->comboBox_Input_PID->addItems(nameList);
    return;
}

void MainWindow::ReadUnits(QStringList unitList, QStringList outputList) //Функция для считывания списка единиц измерения в доступных каналах данных на PTC10
{
    UnitList = unitList;

    ui->comboBox_Output->clear();
    if(UnitList.length() != NameList.length())
    {
        emit responce("smth gone very wrong while reading units");
        return;
    }

    ui->comboBox_Output->addItems(outputList);
    return;
}

void MainWindow::on_pushButton_Start_Power_clicked() //Функция для задания фиксированной мощности на термоконтроллере. По дороге выключается пид и включается возможность подавать мощность
{
    QString output = ui->comboBox_Output->currentText();
    float power = ui->lineEdit_power->text().toFloat();

    if (output != "")
    {
        device->powerStart(output, power);
    }

    return;
}

void MainWindow::pid_Scan(QStringList pidStatus) //Функция для считывания текущих параметров сПИДа
{
    PidStatus = pidStatus;
    ui->pid_LineEdit_P->setText(pidStatus.at(0));
    ui->pid_LineEdit_D->setText(pidStatus.at(1));
    ui->pid_LineEdit_I->setText(pidStatus.at(2));
    ui->pid_LineEdit_Setpoint->setText(pidStatus.at(3));
    ui->comboBox_Input_PID->setCurrentIndex(NameList.indexOf(pidStatus.at(4)));
}

void MainWindow::on_pushButton_Start_PID_clicked() //Запуск сПИДа
{
    QStringList pidStatus;
    QString output = ui->comboBox_Output->currentText();

    pidStatus.append(ui->pid_LineEdit_P->text());
    pidStatus.append(ui->pid_LineEdit_I->text());
    pidStatus.append(ui->pid_LineEdit_D->text());
    pidStatus.append(ui->pid_LineEdit_Setpoint->text());
    pidStatus.append(ui->comboBox_Input_PID->currentText());

    device->pidStart(output, pidStatus);

    return;
}

void MainWindow::on_pushButton_Plot_clicked()//Вечный(нет) цикл
{
    if(ui->pushButton_Plot->text() == "PLOT")
    {
        if(!plotIndexList.isEmpty())
        {
            emit sendIndexList(plotIndexList);
            ui->pushButton_Plot->setText("STOP");
            device->plotStart();
            }else
        {
            emit responce("What should I plot?");
        }
    }else
    {
        device->plotStop();
        ui->pushButton_Plot->setText("PLOT");
        for (int i = 0; i<5; i++)
        {
            ui->widget_T->graph(i)->data().data()->clear();
        }
        for (int i = 0; i<5; i++)
        {
            ui->widget_P->graph(i)->data().data()->clear();
        }
    }
}

void MainWindow::on_checkBox_1_clicked() //попытка отключить один график(правда радикально и безвозвратно)
{
    if(ui->checkBox_1->isChecked())
    {
        ui->comboBox_OutPut_1->setDisabled(true);
        if(plotIndexList.size() < 1)
        {
            plotIndexList.append(NameList.indexOf(ui->comboBox_OutPut_1->currentText()));
        } else
        {
            plotIndexList.replace(0, NameList.indexOf(ui->comboBox_OutPut_1->currentText()));
        }

        ui->groupBoxOutput2->show();
        emit sendIndexList(plotIndexList);
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
        if(plotIndexList.size() < 2)
        {
            plotIndexList.append(NameList.indexOf(ui->comboBox_OutPut_2->currentText()));
        } else
        {
            plotIndexList.replace(1, NameList.indexOf(ui->comboBox_OutPut_2->currentText()));
        }
        ui->groupBoxOutput3->show();
        emit sendIndexList(plotIndexList);
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
        if(plotIndexList.size() < 3)
        {
            plotIndexList.append(NameList.indexOf(ui->comboBox_OutPut_3->currentText()));
        } else
        {
            plotIndexList.replace(2, NameList.indexOf(ui->comboBox_OutPut_3->currentText()));
        }
        ui->groupBoxOutput4->show();
        emit sendIndexList(plotIndexList);
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
        if(plotIndexList.size() < 4)
        {
            plotIndexList.append(NameList.indexOf(ui->comboBox_OutPut_4->currentText()));
        } else
        {
            plotIndexList.replace(3, NameList.indexOf(ui->comboBox_OutPut_4->currentText()));
        }
        ui->groupBoxOutput5->show();
        emit sendIndexList(plotIndexList);
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
        if(plotIndexList.size() < 5)
        {
            plotIndexList.append(NameList.indexOf(ui->comboBox_OutPut_5->currentText()));
        }else
        {
            plotIndexList.replace(4, NameList.indexOf(ui->comboBox_OutPut_5->currentText()));
        }
        emit sendIndexList(plotIndexList);
    } else
    {
        ui->comboBox_OutPut_5->setEnabled(true);
        ui->widget_P->graph(4)->data().data()->clear();
        ui->widget_T->graph(4)->data().data()->clear();

    }
}

void MainWindow::on_pushButton_Check_clicked()//Проверка сПИДа
{
    device->pidScan(ui->comboBox_Output->currentText());
    return;
}

void MainWindow::on_checkBox_fixPlot_T_clicked()//Разрешение пользователю крутить график при нажатии фикс плота
{
    if(ui->checkBox_fixPlot_T->isChecked())
    {
        ui->widget_T->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    }
    else
    {
        ui->widget_T->setInteractions(QCP::iSelectPlottables);
    }
}

void MainWindow::on_checkBox_fixPlot_P_clicked()//Разрешение пользователю крутить график при нажатии фикс плота
{
    if(ui->checkBox_fixPlot_P->isChecked())
    {
         ui->widget_P->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    }
    else
    {
        ui->widget_P->setInteractions(QCP::iSelectPlottables);
    }
}

void MainWindow::on_pushButton_Export_clicked()
{
    QString Name;
    Name = ui->lineEdit_FileName_Export->text();
    device->exportData(Name);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    log_file->write("Closed\n");
    emit finishIt();
    QTest::qWait(200);
    QMainWindow::closeEvent(event);
}

void MainWindow::gotConnected()
{
    ui->pushButton_Connect_TC->setText("Disconnect");
    ui->groupBoxOutput1->show();
    ui->groupBoxPID->show();
    ui->groupBoxFile->show();
    ui->groupBoxPower->show();
    ui->pushButton_Plot->show();
    ui->comboBox_Output->show();
}

void MainWindow::addDataToGraphT(const int index, double value, double time)
{
    ui->widget_T->graph(index)->addData(time, value);
    if(index == 0)
    {
        ui->lineEdit_Channel_1->setText(QString::number(value));
        SetColour(ui->comboBox_Colour_1->currentText(), index, "T");
        ui->label_unit_1->setText("C");
    }
    if(index == 1)
    {
        ui->lineEdit_Channel_2->setText(QString::number(value));
        SetColour(ui->comboBox_Colour_2->currentText(), index, "T");
        ui->label_unit_2->setText("C");
    }
    if(index == 2)
    {
        ui->lineEdit_Channel_3->setText(QString::number(value));
        SetColour(ui->comboBox_Colour_3->currentText(), index, "T");
        ui->label_unit_3->setText("C");
    }
    if(index == 3)
    {
        ui->lineEdit_Channel_4->setText(QString::number(value));
        SetColour(ui->comboBox_Colour_4->currentText(), index, "T");
        ui->label_unit_4->setText("C");
    }
    if(index == 4)
    {
        ui->lineEdit_Channel_5->setText(QString::number(value));
        SetColour(ui->comboBox_Colour_5->currentText(), index, "T");
        ui->label_unit_5->setText("C");
    }
}

void MainWindow::addDataToGraphP(const int index, double value, double time)
{
    ui->widget_P->graph(index)->addData(time, value);
    if(index == 0)
    {
        ui->lineEdit_Channel_1->setText(QString::number(value));
        SetColour(ui->comboBox_Colour_1->currentText(), index, "P");
        ui->label_unit_1->setText("W");
    }
    if(index == 1)
    {
        ui->lineEdit_Channel_2->setText(QString::number(value));
        SetColour(ui->comboBox_Colour_2->currentText(), index, "P");
        ui->label_unit_2->setText("W");
    }
    if(index == 2)
    {
        ui->lineEdit_Channel_3->setText(QString::number(value));
        SetColour(ui->comboBox_Colour_3->currentText(), index, "P");
        ui->label_unit_3->setText("W");
    }
    if(index == 3)
    {
        ui->lineEdit_Channel_4->setText(QString::number(value));
        SetColour(ui->comboBox_Colour_4->currentText(), index, "P");
        ui->label_unit_4->setText("W");
    }
    if(index == 4)
    {
        ui->lineEdit_Channel_5->setText(QString::number(value));
        SetColour(ui->comboBox_Colour_5->currentText(), index, "P");
        ui->label_unit_5->setText("W");
    }
}

void MainWindow::updateGraphs()
{
    if(!ui->checkBox_fixPlot_P->isChecked())
    {
        ui->widget_P->rescaleAxes();
        if(ui->widget_P->yAxis->range().size()<0.01)
        {
            ui->widget_P->yAxis->setRange(ui->widget_P->yAxis->range().center()-0.1, ui->widget_P->yAxis->range().center()+0.1);
        }
        ui->widget_P->replot();
    }
    if (!ui->checkBox_fixPlot_T->isChecked())
    {
        ui->widget_T->rescaleAxes();
        if(ui->widget_T->yAxis->range().size()<0.01)
        {
            ui->widget_T->yAxis->setRange(ui->widget_T->yAxis->range().center()-0.1, ui->widget_T->yAxis->range().center()+0.1);
        }
        ui->widget_T->replot();
    }
}

