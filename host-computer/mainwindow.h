#pragma once
#include <QMainWindow>
#include <QSerialPort>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QElapsedTimer>
#include <QTimer>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include "sensorprotocol.h"
#include "scrollablechartview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onStartClicked();
    void onStopClicked();
    void onReadRegClicked();
    void onWriteRegClicked();
    void onSaveClicked();
    void onClearClicked();
    void onSerialDataReady();
    void onFrameReady(SensorFrame frame);
    void refreshPorts();
    void onFollowClicked();
    void onApplyOSRClicked();   // 新增：发送采样配置

private:
    void setupUI();
    void sendFrame(quint8 cmd, const QByteArray &data = {});
    void setConnected(bool on);
    void addTableRow(const QString &press, const QString &temp, const QString &note);
    void updateFollowButton(bool following);

    // ── 串口 ─────────────────────────────────
    QSerialPort *m_serial = nullptr;
    FrameParser *m_parser = nullptr;

    // ── 图表数据 ─────────────────────────────
    static constexpr double VIEW_WINDOW = 600.0;    // 10分钟滚动窗口

    QLineSeries  *m_pressSeries = nullptr;
    QLineSeries  *m_tempSeries  = nullptr;
    QValueAxis   *m_axisX1      = nullptr;           // 气压图X轴
    QValueAxis   *m_axisX2      = nullptr;           // 温度图X轴
    QValueAxis   *m_axisPress   = nullptr;
    QValueAxis   *m_axisTemp    = nullptr;

    ScrollableChartView *m_pressView = nullptr;
    ScrollableChartView *m_tempView  = nullptr;

    QElapsedTimer m_elapsed;
    double m_dataMaxT = VIEW_WINDOW;                 // 最新时间戳(s)
    double m_pMin =  1e9, m_pMax = -1e9;
    double m_tMin =  1e9, m_tMax = -1e9;
    int    m_cnt  = 0;

    // ── 串口控件 ─────────────────────────────
    QComboBox   *m_cbPort   = nullptr;
    QComboBox   *m_cbBaud   = nullptr;
    QPushButton *m_btnConn  = nullptr;

    // ── 采集控件 ─────────────────────────────
    QPushButton *m_btnStart  = nullptr;
    QPushButton *m_btnStop   = nullptr;
    QPushButton *m_btnFollow = nullptr;

    // ── 采样配置控件（新增）─────────────────
    // STM32数据映射: press_rate(ODR) 0-7→1/2/4/10/20/40/70/200Hz
    //               temp_rate(ODR)  0-6→1/2/4/10/20/40/70Hz (固件限制≤6)
    //               press_osr/temp_osr 0-7→LLP/LP/STD/HP/HHP
    QComboBox   *m_cbPressODR  = nullptr;
    QComboBox   *m_cbPressOSR  = nullptr;
    QComboBox   *m_cbTempODR   = nullptr;
    QComboBox   *m_cbTempOSR   = nullptr;
    QPushButton *m_btnApplyOSR = nullptr;

    // ── 寄存器控件 ───────────────────────────
    QLineEdit   *m_edAddr = nullptr;
    QLineEdit   *m_edVal  = nullptr;

    // ── 传感器信息标签 ───────────────────────
    QLabel      *m_lblType     = nullptr;
    QLabel      *m_lblPress    = nullptr;
    QLabel      *m_lblPressRaw = nullptr;
    QLabel      *m_lblTemp     = nullptr;
    QLabel      *m_lblTempRaw  = nullptr;
    QLabel      *m_lblStatus   = nullptr;

    QTableWidget *m_table    = nullptr;
    QTimer       *m_portTimer = nullptr;
};
