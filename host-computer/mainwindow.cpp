#include "mainwindow.h"
#include <QApplication>
#include <QSerialPortInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDateTime>
#include <QStatusBar>
#include <cstring>

// ─────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Sensor Monitor v2.0");
    resize(1300, 800);

    m_serial = new QSerialPort(this);
    m_parser = new FrameParser(this);

    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::onSerialDataReady);
    connect(m_parser, &FrameParser::frameReady, this, &MainWindow::onFrameReady);

    setupUI();

    m_portTimer = new QTimer(this);
    connect(m_portTimer, &QTimer::timeout, this, &MainWindow::refreshPorts);
    m_portTimer->start(2000);
    refreshPorts();
    setConnected(false);
}

MainWindow::~MainWindow()
{
    if (m_serial->isOpen()) m_serial->close();
}

// ─────────────────────────────────────────────────
void MainWindow::setupUI()
{
    // ══ 左侧面板 ════════════════════════════════
    auto *leftW = new QWidget;
    auto *leftV = new QVBoxLayout(leftW);
    leftW->setFixedWidth(280);

    // ── Serial Port ──────────────────────────────
    auto *grpPort = new QGroupBox("Serial Port");
    auto *portG   = new QGridLayout(grpPort);
    m_cbPort = new QComboBox;
    m_cbBaud = new QComboBox;
    m_cbBaud->addItems({"9600","19200","38400","57600","115200","230400"});
    m_cbBaud->setCurrentText("115200");
    m_btnConn = new QPushButton("Connect");
    portG->addWidget(new QLabel("Port:"), 0, 0);
    portG->addWidget(m_cbPort,            0, 1);
    portG->addWidget(new QLabel("Baud:"), 1, 0);
    portG->addWidget(m_cbBaud,            1, 1);
    portG->addWidget(m_btnConn,           2, 0, 1, 2);
    connect(m_btnConn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);

    // ── Sensor Info ──────────────────────────────
    auto *grpInfo = new QGroupBox("Sensor Info");
    auto *infoV   = new QVBoxLayout(grpInfo);
    m_lblType     = new QLabel("Type: --");
    m_lblPress    = new QLabel("Pressure: -- Pa");
    m_lblPressRaw = new QLabel("Press RAW: --");
    m_lblTemp     = new QLabel("Temp: -- C");
    m_lblTempRaw  = new QLabel("Temp RAW: --");
    m_lblStatus   = new QLabel("Status REG: --");

    m_lblPress   ->setStyleSheet("font-size:13px;font-weight:bold;color:#1565C0;");
    m_lblTemp    ->setStyleSheet("font-size:13px;font-weight:bold;color:#C62828;");
    m_lblPressRaw->setStyleSheet("font-size:11px;color:#1565C0;");
    m_lblTempRaw ->setStyleSheet("font-size:11px;color:#C62828;");
    m_lblStatus  ->setStyleSheet("font-size:11px;color:#555555;");

    infoV->addWidget(m_lblType);
    infoV->addWidget(m_lblPress);
    infoV->addWidget(m_lblPressRaw);
    infoV->addWidget(m_lblTemp);
    infoV->addWidget(m_lblTempRaw);
    infoV->addWidget(m_lblStatus);

    // ── Acquisition ──────────────────────────────
    auto *grpCtrl = new QGroupBox("Acquisition");
    auto *ctrlV   = new QVBoxLayout(grpCtrl);
    m_btnStart  = new QPushButton("Start Continuous");
    m_btnStop   = new QPushButton("Stop");
    m_btnFollow = new QPushButton("Follow Live");
    m_btnFollow->setToolTip("Return to live data after panning history");
    ctrlV->addWidget(m_btnStart);
    ctrlV->addWidget(m_btnStop);
    ctrlV->addWidget(m_btnFollow);
    connect(m_btnStart,  &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(m_btnStop,   &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_btnFollow, &QPushButton::clicked, this, &MainWindow::onFollowClicked);

    // ── Sampling Config ──────────────────────────
    // CMD 0x0B 数据格式: [0]=press_rate [1]=press_osr [2]=temp_rate [3]=temp_osr
    auto *grpSample = new QGroupBox("Sampling Config");
    auto *sampleG   = new QGridLayout(grpSample);

    m_cbPressODR = new QComboBox;
    m_cbPressOSR = new QComboBox;
    m_cbTempODR  = new QComboBox;
    m_cbTempOSR  = new QComboBox;

    // 压力ODR: 索引0-7 → 1/2/4/10/20/40/70/200 Hz (STM32 s_odr_map)
    const QStringList pressOdrList = {
        "0 - 1 Hz","1 - 2 Hz","2 - 4 Hz","3 - 10 Hz",
        "4 - 20 Hz","5 - 40 Hz","6 - 70 Hz","7 - 200 Hz"
    };
    m_cbPressODR->addItems(pressOdrList);

    // 温度ODR: 索引0-6 → 1/2/4/10/20/40/70 Hz (固件限制 temp_rate<=6)
    const QStringList tempOdrList = {
        "0 - 1 Hz","1 - 2 Hz","2 - 4 Hz","3 - 10 Hz",
        "4 - 20 Hz","5 - 40 Hz","6 - 70 Hz"
    };
    m_cbTempODR->addItems(tempOdrList);

    // OSR: 索引0-7 → LLP/LP/STD/HP/HHP (STM32 s_posr_map / s_tosr_map)
    const QStringList osrList = {
        "0 - LLP","1 - LP","2 - STD","3 - HP",
        "4 - HHP","5 - HHP","6 - HHP","7 - HHP"
    };
    m_cbPressOSR->addItems(osrList);
    m_cbTempOSR ->addItems(osrList);

    m_btnApplyOSR = new QPushButton("Apply");
    m_btnApplyOSR->setToolTip(
        "Send ODR/OSR to sensor via CMD 0x0B\n"
        "Press ODR 0-7: 1/2/4/10/20/40/70/200 Hz\n"
        "Temp  ODR 0-6: 1/2/4/10/20/40/70 Hz (max)\n"
        "OSR 0-7: LLP/LP/STD/HP/HHP");

    // 紧凑网格布局: 表头行 + Press行 + Temp行 + Apply行
    sampleG->addWidget(new QLabel(""),         0, 0);
    sampleG->addWidget(new QLabel("ODR"),      0, 1, Qt::AlignCenter);
    sampleG->addWidget(new QLabel("OSR"),      0, 2, Qt::AlignCenter);
    sampleG->addWidget(new QLabel("Press:"),   1, 0);
    sampleG->addWidget(m_cbPressODR,           1, 1);
    sampleG->addWidget(m_cbPressOSR,           1, 2);
    sampleG->addWidget(new QLabel("Temp:"),    2, 0);
    sampleG->addWidget(m_cbTempODR,            2, 1);
    sampleG->addWidget(m_cbTempOSR,            2, 2);
    sampleG->addWidget(m_btnApplyOSR,          3, 0, 1, 3);
    sampleG->setColumnStretch(1, 1);
    sampleG->setColumnStretch(2, 1);

    connect(m_btnApplyOSR, &QPushButton::clicked, this, &MainWindow::onApplyOSRClicked);

    // ── Register (Hex) ───────────────────────────
    auto *grpReg = new QGroupBox("Register (Hex)");
    auto *regG   = new QGridLayout(grpReg);
    m_edAddr = new QLineEdit("0D");
    m_edVal  = new QLineEdit("00");
    auto *btnR = new QPushButton("Read");
    auto *btnW = new QPushButton("Write");
    regG->addWidget(new QLabel("Addr:"), 0, 0);
    regG->addWidget(m_edAddr,            0, 1);
    regG->addWidget(new QLabel("Val:"),  1, 0);
    regG->addWidget(m_edVal,             1, 1);
    regG->addWidget(btnR,                2, 0);
    regG->addWidget(btnW,                2, 1);
    connect(btnR, &QPushButton::clicked, this, &MainWindow::onReadRegClicked);
    connect(btnW, &QPushButton::clicked, this, &MainWindow::onWriteRegClicked);

    // ── Data Management ──────────────────────────
    auto *grpSave = new QGroupBox("Data");
    auto *saveV   = new QVBoxLayout(grpSave);
    auto *btnSave  = new QPushButton("Save CSV");
    auto *btnClear = new QPushButton("Clear");
    saveV->addWidget(btnSave);
    saveV->addWidget(btnClear);
    connect(btnSave,  &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::onClearClicked);

    leftV->addWidget(grpPort);
    leftV->addWidget(grpInfo);
    leftV->addWidget(grpCtrl);
    leftV->addWidget(grpSample);
    leftV->addWidget(grpReg);
    leftV->addWidget(grpSave);
    leftV->addStretch();

    // ══ 气压图表 ════════════════════════════════
    m_pressSeries = new QLineSeries;
    m_pressSeries->setName("Press (Pa)");
    m_pressSeries->setPen(QPen(QColor(21, 101, 192), 2));

    auto *pressChart = new QChart;
    pressChart->addSeries(m_pressSeries);
    pressChart->setTitle("Pressure Monitor");
    pressChart->legend()->setVisible(true);

    m_axisX1    = new QValueAxis;
    m_axisPress = new QValueAxis;
    m_axisX1   ->setTitleText("Time (s)");
    m_axisPress->setTitleText("Press (Pa)");
    m_axisX1   ->setLabelFormat("%.0f");
    m_axisPress->setLabelFormat("%.0f");
    m_axisX1   ->setRange(0, VIEW_WINDOW);
    m_axisPress->setRange(99000, 102000);

    pressChart->addAxis(m_axisX1,    Qt::AlignBottom);
    pressChart->addAxis(m_axisPress, Qt::AlignLeft);
    m_pressSeries->attachAxis(m_axisX1);
    m_pressSeries->attachAxis(m_axisPress);

    m_pressView = new ScrollableChartView(pressChart);
    m_pressView->setXAxis(m_axisX1);
    m_pressView->setMinimumHeight(250);

    // ══ 温度图表 ════════════════════════════════
    m_tempSeries = new QLineSeries;
    m_tempSeries->setName("Temp (degC)");
    m_tempSeries->setPen(QPen(QColor(198, 40, 40), 2));

    auto *tempChart = new QChart;
    tempChart->addSeries(m_tempSeries);
    tempChart->setTitle("Temperature Monitor");
    tempChart->legend()->setVisible(true);

    m_axisX2   = new QValueAxis;
    m_axisTemp = new QValueAxis;
    m_axisX2  ->setTitleText("Time (s)");
    m_axisTemp->setTitleText("Temp (degC)");
    m_axisX2  ->setLabelFormat("%.0f");
    m_axisTemp->setLabelFormat("%.1f");
    m_axisX2  ->setRange(0, VIEW_WINDOW);
    m_axisTemp->setRange(20, 35);

    tempChart->addAxis(m_axisX2,   Qt::AlignBottom);
    tempChart->addAxis(m_axisTemp, Qt::AlignLeft);
    m_tempSeries->attachAxis(m_axisX2);
    m_tempSeries->attachAxis(m_axisTemp);

    m_tempView = new ScrollableChartView(tempChart);
    m_tempView->setXAxis(m_axisX2);
    m_tempView->setMinimumHeight(250);

    // 两图联动: X轴拖动/缩放同步
    connect(m_pressView, &ScrollableChartView::xRangeChanged,
            m_tempView,  &ScrollableChartView::setXRange);
    connect(m_tempView,  &ScrollableChartView::xRangeChanged,
            m_pressView, &ScrollableChartView::setXRange);

    // 两图联动: Follow状态同步
    connect(m_pressView, &ScrollableChartView::followingChanged,
            m_tempView,  &ScrollableChartView::setFollowing);
    connect(m_tempView,  &ScrollableChartView::followingChanged,
            m_pressView, &ScrollableChartView::setFollowing);

    // Follow状态变化 → 更新按钮外观
    connect(m_pressView, &ScrollableChartView::followingChanged,
            this, &MainWindow::updateFollowButton);

    auto *chartW = new QWidget;
    auto *chartH = new QHBoxLayout(chartW);
    chartH->addWidget(m_pressView);
    chartH->addWidget(m_tempView);

    // ══ 数据表格 ════════════════════════════════
    m_table = new QTableWidget(0, 5);
    m_table->setHorizontalHeaderLabels({"No.", "Time", "Press (Pa)", "Temp (degC)", "Note"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setFixedHeight(200);

    // ══ 主布局 ══════════════════════════════════
    auto *rightV = new QVBoxLayout;
    rightV->addWidget(chartW,  3);
    rightV->addWidget(m_table, 1);
    auto *rightW = new QWidget;
    rightW->setLayout(rightV);

    auto *mainH = new QHBoxLayout;
    mainH->addWidget(leftW);
    mainH->addWidget(rightW, 1);
    auto *central = new QWidget;
    central->setLayout(mainH);
    setCentralWidget(central);

    updateFollowButton(true);
    statusBar()->showMessage(
        "Not connected  |  Drag chart to pan history  |  Scroll wheel to zoom");
}

// ─────────────────────────────────────────────────
void MainWindow::refreshPorts()
{
    QString cur = m_cbPort->currentData().toString();
    m_cbPort->clear();
    for (const auto &p : QSerialPortInfo::availablePorts())
        m_cbPort->addItem(p.portName() + "  " + p.description(), p.portName());
    for (int i = 0; i < m_cbPort->count(); ++i)
        if (m_cbPort->itemData(i).toString() == cur)
        { m_cbPort->setCurrentIndex(i); break; }
}

void MainWindow::onConnectClicked()
{
    if (m_serial->isOpen()) {
        onStopClicked();
        m_serial->close();
        setConnected(false);
        statusBar()->showMessage("Disconnected");
        return;
    }
    QString port = m_cbPort->currentData().toString();
    if (port.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a serial port");
        return;
    }
    m_serial->setPortName(port);
    m_serial->setBaudRate(m_cbBaud->currentText().toInt());
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, "Error",
                              "Failed to open port:\n" + m_serial->errorString());
        return;
    }
    setConnected(true);
    m_elapsed.start();
    statusBar()->showMessage("Connected: " + port);
    sendFrame(CMD_GET_PRODUCT_TYPE, QByteArray(1, 0x01));
}

void MainWindow::onSerialDataReady()
{
    m_parser->feed(m_serial->readAll());
}

void MainWindow::onFrameReady(SensorFrame frame)
{
    // 传感器型号枚举映射（与STM32 SensorProductType_t一致）
    static const QMap<int,QString> names = {
                                             {0,"SPL16_006"},{1,"SPL07_003"},{2,"SPL06_001"},
                                             {3,"SPL16_008"},{4,"SPL17_006"},{5,"SPL15_002"},
                                             {6,"SPL16_005"},{7,"SPA07_005"},{8,"SPD17_001"},
                                             };

    switch (frame.cmd) {

    // ── 0x01: 产品类型 ───────────────────────────
    case CMD_GET_PRODUCT_TYPE: {
        quint8 id = frame.data.isEmpty() ? 0xFF : (quint8)frame.data[0];
        m_lblType->setText("Type: " + names.value(id, "UNKNOWN"));
        statusBar()->showMessage("Sensor detected: " + names.value(id, "UNKNOWN"));
    } break;

    // ── 0x07: 气压+温度连续数据帧（14字节）───────
    case CMD_READ_PRESS_TEMP: {
        if (frame.data.size() < 6) break;

        // [0..3] 压力 U32 小端 ÷100 = Pa
        quint32 pr = 0;
        memcpy(&pr, frame.data.constData(), 4);
        double press = pr / 100.0;

        // [4..5] 温度 S16 小端 ÷100 = degC
        qint16 tr = 0;
        memcpy(&tr, frame.data.constData() + 4, 2);
        double temp = tr / 100.0;

        m_lblPress->setText(QString("Pressure: %1 Pa").arg(press, 0, 'f', 2));
        m_lblTemp ->setText(QString("Temp: %1 C").arg(temp,  0, 'f', 2));

        // [6..8] 气压RAW 3字节小端有符号 + [9]状态寄存器 + [10..13] 温度RAW S32小端
        if (frame.data.size() >= 14) {
            qint32 pressRaw = 0;
            memcpy(&pressRaw, frame.data.constData() + 6, 3);
            if (pressRaw & 0x800000) pressRaw |= 0xFF000000; // 24→32位符号扩展

            quint8 statusReg = static_cast<quint8>(frame.data[9]);

            qint32 tempRaw = 0;
            memcpy(&tempRaw, frame.data.constData() + 10, 4);

            m_lblPressRaw->setText(QString("Press RAW: %1").arg(pressRaw));
            m_lblTempRaw ->setText(QString("Temp RAW: %1").arg(tempRaw));
            m_lblStatus  ->setText(
                QString("Status REG: 0x%1")
                    .arg(QString::number(statusReg, 16).toUpper().rightJustified(2, '0')));
        }

        // ── 图表数据处理 ─────────────────────────
        double t = m_elapsed.elapsed() / 1000.0;
        m_dataMaxT = t;
        m_pressView->setDataMaxTime(t);
        m_tempView ->setDataMaxTime(t);

        m_pressSeries->append(t, press);
        m_tempSeries ->append(t, temp);

        // 移除超出10分钟窗口的旧数据
        const double oldest = t - VIEW_WINDOW;
        while (m_pressSeries->count() > 0 && m_pressSeries->at(0).x() < oldest) {
            m_pressSeries->remove(0);
            m_tempSeries ->remove(0);
        }

        // 更新Y轴范围（全局追踪）
        m_pMin = qMin(m_pMin, press); m_pMax = qMax(m_pMax, press);
        m_tMin = qMin(m_tMin, temp);  m_tMax = qMax(m_tMax, temp);
        m_axisPress->setRange(m_pMin - 20.0, m_pMax + 20.0);
        m_axisTemp ->setRange(m_tMin -  1.0, m_tMax +  1.0);

        // 跟随模式：自动滚动X轴到最新数据
        if (m_pressView->isFollowing()) {
            const double viewEnd   = t + 2.0;
            const double viewStart = qMax(0.0, viewEnd - VIEW_WINDOW);
            m_axisX1->setRange(viewStart, viewEnd);
            m_axisX2->setRange(viewStart, viewEnd);
        }

        // 每10帧写入一行表格
        if (++m_cnt % 10 == 0)
            addTableRow(QString::number(press, 'f', 2),
                        QString::number(temp,  'f', 2), "continuous");
    } break;

    // ── 0x06: 读寄存器回复 ───────────────────────
    case CMD_READ_REGISTER: {
        quint8 v = frame.data.isEmpty() ? 0 : (quint8)frame.data[0];
        const QString hexStr = QString::number(v, 16).toUpper().rightJustified(2, '0');
        const QString s = QString("Reg 0x%1 = 0x%2 (%3)")
                              .arg(m_edAddr->text().toUpper())
                              .arg(hexStr)
                              .arg(v);
        statusBar()->showMessage(s);
        m_edVal->setText(hexStr);
        addTableRow("--", "--", s);
    } break;

    // ── 0x05: 写寄存器回复 ───────────────────────
    case CMD_WRITE_REGISTER: {
        bool ok = !frame.data.isEmpty() && (quint8)frame.data[0] == 0x01;
        statusBar()->showMessage(ok ? "Write register OK" : "Write register FAILED");
    } break;

    // ── 0x0B: 采样配置回复 ───────────────────────
    case CMD_SET_OVERSAMPLING: {
        // STM32回复: 0x01=成功, 0x00=失败
        bool ok = !frame.data.isEmpty() && (quint8)frame.data[0] == 0x01;
        statusBar()->showMessage(ok ? "Sampling config applied OK"
                                    : "Sampling config FAILED");
    } break;

    default: break;
    }
}

// ─────────────────────────────────────────────────
void MainWindow::onStartClicked()
{
    m_pMin = m_tMin = 1e9;
    m_pMax = m_tMax = -1e9;
    m_cnt = 0;
    m_dataMaxT = VIEW_WINDOW;
    m_elapsed.restart();
    m_pressSeries->clear();
    m_tempSeries ->clear();
    m_axisX1->setRange(0, VIEW_WINDOW);
    m_axisX2->setRange(0, VIEW_WINDOW);
    m_pressView->setFollowing(true);
    m_tempView ->setFollowing(true);
    updateFollowButton(true);
    sendFrame(CMD_READ_PRESS_TEMP, QByteArray(1, 0x01));
    m_btnStart->setEnabled(false);
    m_btnStop ->setEnabled(true);
    statusBar()->showMessage(
        "Continuous read...  |  Drag to view history  |  Scroll to zoom  |  Last 10 min");
}

void MainWindow::onStopClicked()
{
    sendFrame(CMD_READ_PRESS_TEMP, QByteArray(1, 0x00));
    m_btnStart->setEnabled(true);
    m_btnStop ->setEnabled(false);
    statusBar()->showMessage("Stopped");
}

void MainWindow::onFollowClicked()
{
    m_pressView->setFollowing(true);
    m_tempView ->setFollowing(true);
    updateFollowButton(true);

    const double viewEnd   = m_dataMaxT + 2.0;
    const double viewStart = qMax(0.0, viewEnd - VIEW_WINDOW);
    m_axisX1->setRange(viewStart, viewEnd);
    m_axisX2->setRange(viewStart, viewEnd);
    statusBar()->showMessage("Returned to live view");
}

void MainWindow::onApplyOSRClicked()
{
    // 构造CMD_SET_OVERSAMPLING帧
    // data[0]=press_rate(ODR索引0-7)  data[1]=press_osr(索引0-7)
    // data[2]=temp_rate(ODR索引0-6)   data[3]=temp_osr(索引0-7)
    const quint8 pressRate = static_cast<quint8>(m_cbPressODR->currentIndex());
    const quint8 pressOSR  = static_cast<quint8>(m_cbPressOSR->currentIndex());
    const quint8 tempRate  = static_cast<quint8>(m_cbTempODR->currentIndex());
    const quint8 tempOSR   = static_cast<quint8>(m_cbTempOSR->currentIndex());

    QByteArray d;
    d.append(static_cast<char>(pressRate));
    d.append(static_cast<char>(pressOSR));
    d.append(static_cast<char>(tempRate));
    d.append(static_cast<char>(tempOSR));
    sendFrame(CMD_SET_OVERSAMPLING, d);

    statusBar()->showMessage(
        QString("Applying: P-ODR=%1 P-OSR=%2 T-ODR=%3 T-OSR=%4 ...")
            .arg(pressRate).arg(pressOSR).arg(tempRate).arg(tempOSR));
}

void MainWindow::onReadRegClicked()
{
    bool ok;
    quint8 addr = static_cast<quint8>(m_edAddr->text().toUInt(&ok, 16));
    if (!ok) { statusBar()->showMessage("Invalid address format"); return; }
    sendFrame(CMD_READ_REGISTER, QByteArray(1, static_cast<char>(addr)));
}

void MainWindow::onWriteRegClicked()
{
    bool ok1, ok2;
    quint8 addr = static_cast<quint8>(m_edAddr->text().toUInt(&ok1, 16));
    quint8 val  = static_cast<quint8>(m_edVal ->text().toUInt(&ok2, 16));
    if (!ok1 || !ok2) { statusBar()->showMessage("Invalid format"); return; }
    QByteArray d;
    d.append(static_cast<char>(addr));
    d.append(static_cast<char>(val));
    sendFrame(CMD_WRITE_REGISTER, d);
}

void MainWindow::onSaveClicked()
{
    if (m_table->rowCount() == 0) {
        QMessageBox::information(this, "Info", "No data to save");
        return;
    }
    QString path = QFileDialog::getSaveFileName(
        this, "Save Data", "sensor_data.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Failed to save file");
        return;
    }
    QTextStream out(&f);
    out << "No.,Time,Press(Pa),Temp(degC),Note\n";
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QStringList row;
        for (int c = 0; c < 5; ++c)
            row << (m_table->item(r, c) ? m_table->item(r, c)->text() : "");
        out << row.join(",") << "\n";
    }
    const int n = m_table->rowCount();
    statusBar()->showMessage(QString("Saved %1 rows").arg(n));
    QMessageBox::information(this, "Saved",
                             QString("Saved %1 rows to:\n%2").arg(n).arg(path));
}

void MainWindow::onClearClicked()
{
    m_pressSeries->clear();
    m_tempSeries ->clear();
    m_table->setRowCount(0);
    m_pMin = m_tMin = 1e9;
    m_pMax = m_tMax = -1e9;
    m_cnt = 0;
    m_dataMaxT = VIEW_WINDOW;
    m_elapsed.restart();
    m_axisX1->setRange(0, VIEW_WINDOW);
    m_axisX2->setRange(0, VIEW_WINDOW);
    m_pressView->setFollowing(true);
    m_tempView ->setFollowing(true);
    updateFollowButton(true);
    statusBar()->showMessage("Cleared");
}

// ─────────────────────────────────────────────────
void MainWindow::sendFrame(quint8 cmd, const QByteArray &data)
{
    if (!m_serial->isOpen()) return;
    m_serial->write(buildFrame(cmd, data));
}

void MainWindow::setConnected(bool on)
{
    m_btnConn    ->setText(on ? "Disconnect" : "Connect");
    m_btnStart   ->setEnabled(on);
    m_btnStop    ->setEnabled(false);
    m_btnApplyOSR->setEnabled(on);   // 连接后才能发配置
}

void MainWindow::addTableRow(const QString &press, const QString &temp, const QString &note)
{
    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
    m_table->setItem(row, 1, new QTableWidgetItem(
                                 QDateTime::currentDateTime().toString("hh:mm:ss")));
    m_table->setItem(row, 2, new QTableWidgetItem(press));
    m_table->setItem(row, 3, new QTableWidgetItem(temp));
    m_table->setItem(row, 4, new QTableWidgetItem(note));
    m_table->scrollToBottom();
}

void MainWindow::updateFollowButton(bool following)
{
    // 跟随模式=绿色, 已暂停=橙色
    if (following) {
        m_btnFollow->setText("Follow Live");
        m_btnFollow->setStyleSheet("color:#2e7d32;font-weight:bold;");
    } else {
        m_btnFollow->setText("Go to Latest");
        m_btnFollow->setStyleSheet("color:#e65100;font-weight:bold;");
    }
}
