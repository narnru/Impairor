#ifndef PTC10_H
#define PTC10_H

#include <QString>
#include <QObject>
#include <QSerialPort>
#include <QApplication>
#include <QTest>
#include <QFile>
#include <QDateTime>


class PTC10 : public QObject
{
    Q_OBJECT
private:
        QSerialPort *serial;
        int additionalWaitTime = 50;
        QStringList NameList;
        QStringList UnitList;
        QStringList OutputList;
        QList <int> plotIndexList;
        bool run = false;
        QFile *reserveFile;
        double timeStart;


private slots:

public:
        explicit PTC10(QObject *parent = 0);
        ~PTC10();

signals:
        void responce(const QString data);
        void connected();
        void pidScanResult(QStringList result);
        void pidStarted();
        void powerStarted();
        void plotStopped();
        void dataForGraph_P(const int graphNumber, double value, double time);
        void dataForGraph_T(const int graphNumber, double value, double time);
        void updateGraphs();
        void readNamesResult(QStringList nameList);
        void readUnitsResult(QStringList unitList, QStringList outputList);      

public slots:
        bool isOpen();
        QString readDataAction();
        void sendDataAction(QString data);
        void calibrateWaitTime();
        void connect(QString portName);
        void disconnect();
        void scanBauds();
        void readNames();
        void readUnits();
        void pidScan(QString output);
        void pidStart(QString output, QStringList pidStatus);
        void powerStart(QString output, float power);
        void plotStart();
        void plotStop();
        void plot();
        void exportData(QString Name);
        QString sendAndRead(QString data);
        void setIndexList(QList<int> indexList);
        void finish();
        void create();
        void OutputEnable(QString message);
};

#endif // PTC10_H
