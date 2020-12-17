#ifndef NETWORKEMULATOR_H
#define NETWORKEMULATOR_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <QFile>
#include <QFileDialog>
#include <QSaveFile>
#include <QTextStream>
#include <QStandardItemModel>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QValueAxis>
#include <QChartView>
#include <QMainWindow>
#include <QStandardItemModel>

#include <QUdpSocket>

#define NETWORK_DELAY_MS            30
#define MIN_NETWORK_DELAY_MS        5
#define MAX_NETWORK_DELAY_MS        100
#define ERROR_RATE_PERCENT          5
#define MIN_ERROR_RATE_PERCENT      4
#define MAX_ERROR_RATE_PERCENT      100
#define INITIAL_MAX_X               3
#define INITIAL_MAX_Y               5

QT_BEGIN_NAMESPACE
namespace Ui { class NetworkEmulator; }
QT_END_NAMESPACE
using namespace QtCharts;

class NetworkEmulator : public QMainWindow
{
    Q_OBJECT

public:
    // constructor
    NetworkEmulator(QWidget *parent = nullptr);
    // destructor
    ~NetworkEmulator();

public slots:
    void on_startButton_clicked();

    void on_stopButton_clicked();

    bool on_saveButton_clicked();

    void on_resetButton_clicked();

    void processPendingDatagram();

    void onNetworkDelaySliderChange();

    void onBitErrorRateSliderChange();

private:
    Ui::NetworkEmulator *ui;
    QChart *chart = nullptr;
    QChartView *chartView = nullptr;
    QLineSeries *series = nullptr;
    QStandardItemModel *settingTableModel = nullptr;
    QString statusLabelTextActive = "Active";
    QString statusLabelTextStopped = "Stopped";
    QString statusLabelStyleStopped = "font: 75 10pt \"MS Shell Dlg 2\"; color: rgb(255, 86, 56); font-weight: bold;";
    QString statusLabelStyleActive = "font: 75 10pt \"MS Shell Dlg 2\"; color: rgb(43, 189, 83); font-weight: bold;";
    QStandardItemModel *packetTableModel = nullptr;
    QStandardItemModel *summaryTableModel = nullptr;
    QString sourceIP;
    QString destinationIP;
    QValueAxis* axisX = nullptr;
    QValueAxis* axisY = nullptr;
    QUdpSocket* udpSocket = nullptr;

    int droppedPackets = 0;
    int retransmits = 0;
    int packetTableRowIndex = 0;
    bool pause;
    const int minX = 0;
    const int minY = 0;
    int maxX = INITIAL_MAX_X;
    int maxY = INITIAL_MAX_Y;
    int networkDelay = NETWORK_DELAY_MS;
    int errorRatePercent = ERROR_RATE_PERCENT;

    struct packet* pkt = nullptr;
    int packetSize = 0;

    struct timeval start{0,0}, end{0,0};

    void init();
    void resetFiguresState();
    QStandardItemModel* convertAbstractModelToStandard(QAbstractItemModel* model);
    bool dropPkt(int prob);
    void averagePktDelay(int delay);
    void relayPacket(QHostAddress* sender, quint16 senderPort, QTime* relTime, QString relTimeString);
    void recordPacket(QHostAddress* sender, quint16 senderPort, QTime* relTime, QString relTimeString);
    void updatePacketTable(struct packet* pkt, QHostAddress* sourceIP, quint16 sourcePort, const char* destinationIP, int destinationPort, bool isDropped, QString relTime, QColor rowColor);
    void updateNetworkSummaryTable(QString relTime);
    void updateTimeSequence(struct packet* pkt, QHostAddress* sourceIP, QTime* relTime);
};
#endif // NETWORKEMULATOR_H
